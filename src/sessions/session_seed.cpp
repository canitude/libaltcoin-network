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
#include <altcoin/network/sessions/session_seed.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/protocols/protocol_ping_31402.hpp>
#include <altcoin/network/protocols/protocol_ping_60001.hpp>
#include <altcoin/network/protocols/protocol_reject_70002.hpp>
#include <altcoin/network/protocols/protocol_seed_31402.hpp>
#include <altcoin/network/protocols/protocol_version_31402.hpp>
#include <altcoin/network/protocols/protocol_version_70002.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_seed<MessageSubscriber>
#define NAME "session_seed"

/// If seeding occurs it must generate an increase of 100 hosts or will fail.
static const size_t minimum_host_increase = 100;

using namespace std::placeholders;

template<class MessageSubscriber>
session_seed<MessageSubscriber>::session_seed(p2p<MessageSubscriber>& network)
  : session<MessageSubscriber>(network, false),
    CONSTRUCT_TRACK(session_seed<MessageSubscriber>)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void session_seed<MessageSubscriber>::start(result_handler handler)
{
    if (this->settings_.host_pool_capacity == 0)
    {
        LOG_INFO(LOG_NETWORK)
            << "Not configured to populate an address pool.";
        handler(error::success);
        return;
    }

    session<MessageSubscriber>::start(CONCURRENT_DELEGATE2(handle_started, _1, handler));
}

template<class MessageSubscriber>
void session_seed<MessageSubscriber>::handle_started(const code& ec, result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    const auto start_size = this->address_count();

    if (start_size != 0)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Seeding is not required because there are "
            << start_size << " cached addresses.";
        handler(error::success);
        return;
    }

    if (this->settings_.seeds.empty())
    {
        LOG_ERROR(LOG_NETWORK)
            << "Seeding is required but no seeds are configured.";
        handler(error::operation_failed);
        return;
    }

    // This is NOT technically the end of the start sequence, since the handler
    // is not invoked until seeding operations are complete.
    start_seeding(start_size, handler);
}

template<class MessageSubscriber>
void session_seed<MessageSubscriber>::attach_handshake_protocols(typename channel<MessageSubscriber>::ptr channel,
    result_handler handle_started)
{
    // Don't use configured services or relay for seeding.
    const auto relay = false;
    const auto own_version = this->settings_.protocol_maximum;
    const auto own_services = message::version::service::none;
    const auto invalid_services = this->settings_.invalid_services;
    const auto minimum_version = this->settings_.protocol_minimum;
    const auto minimum_services = message::version::service::none;

    // Reject messages are not handled until bip61 (70002).
    // The negotiated_version is initialized to the configured maximum.
    if (channel->negotiated_version() >= message::version::level::bip61)
        this->template attach<protocol_version_70002<MessageSubscriber>>(channel, own_version, own_services,
            invalid_services, minimum_version, minimum_services, relay)
            ->start(handle_started);
    else
        this->template attach<protocol_version_31402<MessageSubscriber>>(channel, own_version, own_services,
            invalid_services, minimum_version, minimum_services)
            ->start(handle_started);
}

// Seed sequence.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void session_seed<MessageSubscriber>::start_seeding(size_t start_size, result_handler handler)
{
    const auto complete = BIND2(handle_complete, start_size, handler);

    const auto join_handler = synchronize(complete, this->settings_.seeds.size(),
        NAME, synchronizer_terminate::on_count);

    // We don't use parallel here because connect is itself asynchronous.
    for (const auto& seed: this->settings_.seeds)
        start_seed(seed, join_handler);
}

template<class MessageSubscriber>
void session_seed<MessageSubscriber>::start_seed(const config::endpoint& seed,
    result_handler handler)
{
    if (this->stopped())
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Suspended seed connection";
        handler(error::channel_stopped);
        return;
    }

    LOG_INFO(LOG_NETWORK)
        << "Contacting seed [" << seed << "]";

    const auto connector = this->create_connector();
    this->pend(connector);

    // OUTBOUND CONNECT
    connector->connect(seed,
        BIND5(handle_connect, _1, _2, seed, connector, handler));
}

template<class MessageSubscriber>
void session_seed<MessageSubscriber>::handle_connect(const code& ec, typename channel<MessageSubscriber>::ptr channel,
    const config::endpoint& seed, typename connector<MessageSubscriber>::ptr connector,
    result_handler handler)
{
    this->unpend(connector);

    if (ec)
    {
        LOG_INFO(LOG_NETWORK)
            << "Failure contacting seed [" << seed << "] " << ec.message();
        handler(ec);
        return;
    }

    if (this->blacklisted(channel->authority()))
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Seed [" << seed << "] on blacklisted address ["
            << channel->authority() << "]";
        handler(error::address_blocked);
        return;
    }

    LOG_INFO(LOG_NETWORK)
        << "Connected seed [" << seed << "] as " << channel->authority();

    this->register_channel(channel,
        BIND3(handle_channel_start, _1, channel, handler),
        BIND1(handle_channel_stop, _1));
}

template<class MessageSubscriber>
void session_seed<MessageSubscriber>::handle_channel_start(const code& ec, typename channel<MessageSubscriber>::ptr channel,
    result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    attach_protocols(channel, handler);
};

template<class MessageSubscriber>
void session_seed<MessageSubscriber>::attach_protocols(typename channel<MessageSubscriber>::ptr channel,
    result_handler handler)
{
    const auto version = channel->negotiated_version();

    if (version >= message::version::level::bip31)
        this->template attach<protocol_ping_60001<MessageSubscriber>>(channel)->start();
    else
        this->template attach<protocol_ping_31402<MessageSubscriber>>(channel)->start();

    if (version >= message::version::level::bip61)
        this->template attach<protocol_reject_70002<MessageSubscriber>>(channel)->start();

    this->template attach<protocol_seed_31402<MessageSubscriber>>(channel)->start(handler);
}

template<class MessageSubscriber>
void session_seed<MessageSubscriber>::handle_channel_stop(const code& ec)
{
    LOG_DEBUG(LOG_NETWORK)
        << "Seed channel stopped: " << ec.message();
}

// This accepts no error code because individual seed errors are suppressed.
template<class MessageSubscriber>
void session_seed<MessageSubscriber>::handle_complete(size_t start_size, result_handler handler)
{
    // We succeed only if there is a host count increase of at least 100.
    const auto increase = this->address_count() >=
        ceiling_add(start_size, minimum_host_increase);

    // This is the end of the seed sequence.
    handler(increase ? error::success : error::peer_throttling);
}

template class session_seed<message_subscriber>;

} // namespace network
} // namespace libbitcoin
