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
#include <altcoin/network/protocols/protocol_version_70002.hpp>

#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/protocols/protocol_version_31402.hpp>
#include <altcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "version"
#define CLASS protocol_version_70002<MessageSubscriber>

using namespace bc::message;
using namespace std::placeholders;

static const std::string insufficient_version = "insufficient-version";
static const std::string insufficient_services = "insufficient-services";

// TODO: set explicitly on inbound (none or new config) and self on outbound.
// Configured services was our but we found that most incoming connections are
// set to zero, so that is currently the default (see below).
template<class MessageSubscriber>
protocol_version_70002<MessageSubscriber>::protocol_version_70002(p2p<MessageSubscriber>& network,
    typename channel<MessageSubscriber>::ptr channel)
  : protocol_version_70002(network, channel,
        network.network_settings().protocol_maximum,
        network.network_settings().services,
        network.network_settings().invalid_services,
        network.network_settings().protocol_minimum,
        version::service::none,
        network.network_settings().relay_transactions)
{
}

template<class MessageSubscriber>
protocol_version_70002<MessageSubscriber>::protocol_version_70002(p2p<MessageSubscriber>& network,
    typename channel<MessageSubscriber>::ptr channel, uint32_t own_version, uint64_t own_services,
    uint64_t invalid_services, uint32_t minimum_version,
    uint64_t minimum_services, bool relay)
  : protocol_version_31402<MessageSubscriber>(network, channel, own_version, own_services,
        invalid_services, minimum_version, minimum_services),
    relay_(relay),
    CONSTRUCT_TRACK(protocol_version_70002<MessageSubscriber>)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void protocol_version_70002<MessageSubscriber>::start(event_handler handler)
{
    protocol_version_31402<MessageSubscriber>::start(handler);

    SUBSCRIBE2(reject, handle_receive_reject, _1, _2);
}

template<class MessageSubscriber>
message::version protocol_version_70002<MessageSubscriber>::version_factory() const
{
    auto version = protocol_version_31402<MessageSubscriber>::version_factory();

    // This is the only difference at protocol level 70001.
    version.set_relay(relay_);
    return version;
}

// Protocol.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
bool protocol_version_70002<MessageSubscriber>::sufficient_peer(version_const_ptr message)
{
    if (message->value() < this->minimum_version_)
    {
        const reject version_rejection
        {
            reject::reason_code::obsolete,
            version::command,
            insufficient_version
        };

        SEND2(version_rejection, handle_send, _1, reject::command);
    }
    else if ((message->services() & this->minimum_services_) != this->minimum_services_)
    {
        const reject obsolete_rejection
        {
            reject::reason_code::obsolete,
            version::command,
            insufficient_services
        };

        SEND2(obsolete_rejection, handle_send, _1, reject::command);
    }

    return protocol_version_31402<MessageSubscriber>::sufficient_peer(message);
}

template<class MessageSubscriber>
bool protocol_version_70002<MessageSubscriber>::handle_receive_reject(const code& ec,
    reject_const_ptr reject)
{
    if (this->stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving reject from [" << this->authority() << "] "
            << ec.message();
        this->set_event(error::channel_stopped);
        return false;
    }

    const auto& message = reject->message();

    // Handle these in the reject protocol.
    if (message != version::command)
        return true;

    const auto code = reject->code();

    // Client is an obsolete, unsupported version.
    if (code == reject::reason_code::obsolete)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Obsolete version reject from [" << this->authority() << "] '"
            << reject->reason() << "'";
        this->set_event(error::channel_stopped);
        return false;
    }

    // Duplicate version message received.
    if (code == reject::reason_code::duplicate)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Duplicate version reject from [" << this->authority() << "] '"
            << reject->reason() << "'";
        this->set_event(error::channel_stopped);
        return false;
    }

    return true;
}

template class protocol_version_70002<message_subscriber>;

} // namespace network
} // namespace libbitcoin
