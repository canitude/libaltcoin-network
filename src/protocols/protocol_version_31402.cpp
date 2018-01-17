/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <altcoin/network/protocols/protocol_version_31402.hpp>

#include <cstdint>
#include <functional>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/protocols/protocol_timer.hpp>
#include <altcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "version"
#define CLASS protocol_version_31402<MessageSubscriber>

using namespace bc::message;
using namespace std::placeholders;

// TODO: set explicitly on inbound (none or new config) and self on outbound.
// Require the configured minimum and services by default.
// Configured min version is our own but we may require higer for some stuff.
// Configured services was our but we found that most incoming connections are
// set to zero, so that is currently the default (see below).
template<class MessageSubscriber>
protocol_version_31402<MessageSubscriber>::protocol_version_31402(p2p<MessageSubscriber>& network,
    typename channel<MessageSubscriber>::ptr channel)
  : protocol_version_31402(network, channel,
        network.network_settings().protocol_maximum,
        network.network_settings().services,
        network.network_settings().invalid_services,
        network.network_settings().protocol_minimum,
        version::service::none)
{
}

template<class MessageSubscriber>
protocol_version_31402<MessageSubscriber>::protocol_version_31402(p2p<MessageSubscriber>& network,
    typename channel<MessageSubscriber>::ptr channel, uint32_t own_version, uint64_t own_services,
    uint64_t invalid_services, uint32_t minimum_version,
    uint64_t minimum_services)
  : protocol_timer<MessageSubscriber>(network, channel, false, NAME),
    network_(network),
    own_version_(own_version),
    own_services_(own_services),
    invalid_services_(invalid_services),
    minimum_version_(minimum_version),
    minimum_services_(minimum_services),
    CONSTRUCT_TRACK(protocol_version_31402<MessageSubscriber>)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void protocol_version_31402<MessageSubscriber>::start(event_handler handler)
{
    const auto period = network_.network_settings().channel_handshake();

    const auto join_handler = synchronize(handler, 2, NAME,
        synchronizer_terminate::on_error);

    // The handler is invoked in the context of the last message receipt.
    protocol_timer<MessageSubscriber>::start(period, join_handler);

    SUBSCRIBE2(version, handle_receive_version, _1, _2);
    SUBSCRIBE2(verack, handle_receive_verack, _1, _2);
    SEND2(version_factory(), handle_send, _1, version::command);
}

template<class MessageSubscriber>
message::version protocol_version_31402<MessageSubscriber>::version_factory() const
{
    const auto& settings = network_.network_settings();
    const auto height = network_.top_block().height();
    BITCOIN_ASSERT_MSG(height <= max_uint32, "Time to upgrade the protocol.");

    message::version version;
    version.set_value(own_version_);
    version.set_services(own_services_);
    version.set_timestamp(static_cast<uint64_t>(zulu_time()));
    version.set_address_receiver(this->authority().to_network_address());
    version.set_address_sender(settings.self.to_network_address());
    version.set_nonce(this->nonce());
    version.set_user_agent(BC_USER_AGENT);
    version.set_start_height(static_cast<uint32_t>(height));

    // The peer's services cannot be reflected, so zero it.
    version.address_receiver().set_services(version::service::none);

    // We always match the services declared in our version.services.
    version.address_sender().set_services(own_services_);
    return version;
}

// Protocol.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
bool protocol_version_31402<MessageSubscriber>::handle_receive_version(const code& ec,
    version_const_ptr message)
{
    if (this->stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving version from [" << this->authority() << "] "
            << ec.message();
        this->set_event(ec);
        return false;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Peer [" << this->authority() << "] protocol version ("
        << message->value() << ") user agent: " << message->user_agent();

    // TODO: move these three checks to initialization.
    //-------------------------------------------------------------------------

    const auto& settings = network_.network_settings();

    if (settings.protocol_minimum < version::level::minimum)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, minimum below ("
            << version::level::minimum << ").";
        this->set_event(error::channel_stopped);
        return false;
    }

    if (settings.protocol_maximum > version::level::maximum)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, maximum above ("
            << version::level::maximum << ").";
        this->set_event(error::channel_stopped);
        return false;
    }

    if (settings.protocol_minimum > settings.protocol_maximum)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, "
            << "minimum exceeds maximum.";
        this->set_event(error::channel_stopped);
        return false;
    }

    //-------------------------------------------------------------------------

    if (!sufficient_peer(message))
    {
        this->set_event(error::channel_stopped);
        return false;
    }

    const auto version = std::min(message->value(), own_version_);
    this->set_negotiated_version(version);
    this->set_peer_version(message);

    LOG_DEBUG(LOG_NETWORK)
        << "Negotiated protocol version (" << version
        << ") for [" << this->authority() << "]";

    SEND2(verack(), handle_send, _1, verack::command);

    // 1 of 2
    this->set_event(error::success);
    return false;
}

template<class MessageSubscriber>
bool protocol_version_31402<MessageSubscriber>::sufficient_peer(version_const_ptr message)
{
    if ((message->services() & invalid_services_) != 0)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Invalid peer network services (" << message->services()
            << ") for [" << this->authority() << "]";
        return false;
    }

    if ((message->services() & minimum_services_) != minimum_services_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Insufficient peer network services (" << message->services()
            << ") for [" << this->authority() << "]";
        return false;
    }

    if (message->value() < minimum_version_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Insufficient peer protocol version (" << message->value()
            << ") for [" << this->authority() << "]";
        return false;
    }

    return true;
}

template<class MessageSubscriber>
bool protocol_version_31402<MessageSubscriber>::handle_receive_verack(const code& ec,
    verack_const_ptr)
{
    if (this->stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving verack from [" << this->authority() << "] "
            << ec.message();
        this->set_event(ec);
        return false;
    }

    // 2 of 2
    this->set_event(error::success);
    return false;
}

template class protocol_version_31402<message_subscriber>;

} // namespace network
} // namespace libbitcoin
