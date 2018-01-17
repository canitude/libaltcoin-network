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
#include <altcoin/network/sessions/session_outbound.hpp>

#include <cstddef>
#include <functional>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/protocols/protocol_address_31402.hpp>
#include <altcoin/network/protocols/protocol_ping_31402.hpp>
#include <altcoin/network/protocols/protocol_ping_60001.hpp>
#include <altcoin/network/protocols/protocol_reject_70002.hpp>
#include <altcoin/network/protocols/protocol_version_31402.hpp>
#include <altcoin/network/protocols/protocol_version_70002.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_outbound<MessageSubscriber>

using namespace std::placeholders;

template<class MessageSubscriber>
session_outbound<MessageSubscriber>::session_outbound(p2p<MessageSubscriber>& network, bool notify_on_connect)
  : session_batch<MessageSubscriber>(network, notify_on_connect),
    CONSTRUCT_TRACK(session_outbound<MessageSubscriber>)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void session_outbound<MessageSubscriber>::start(result_handler handler)
{
    if (this->settings_.outbound_connections == 0)
    {
        LOG_INFO(LOG_NETWORK)
            << "Not configured for generating outbound connections.";
        handler(error::success);
        return;
    }

    LOG_INFO(LOG_NETWORK)
        << "Starting outbound session.";

    session<MessageSubscriber>::start(CONCURRENT_DELEGATE2(handle_started, _1, handler));
}

template<class MessageSubscriber>
void session_outbound<MessageSubscriber>::handle_started(const code& ec, result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    for (size_t peer = 0; peer < this->settings_.outbound_connections; ++peer)
        new_connection(error::success);

    // This is the end of the start sequence.
    handler(error::success);
}

// Connnect cycle.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void session_outbound<MessageSubscriber>::new_connection(const code&)
{
    if (this->stopped())
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Suspended outbound connection.";
        return;
    }

    session_batch<MessageSubscriber>::connect(BIND2(handle_connect, _1, _2));
}

template<class MessageSubscriber>
void session_outbound<MessageSubscriber>::handle_connect(const code& ec, typename channel<MessageSubscriber>::ptr channel)
{
    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure connecting outbound: " << ec.message();

        // Retry with conditional delay, regardless of error.
        this->dispatch_delayed(this->cycle_delay(ec), BIND1(new_connection, _1));
        return;
    }

    this->register_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND2(handle_channel_stop, _1, channel));
}

template<class MessageSubscriber>
void session_outbound<MessageSubscriber>::handle_channel_start(const code& ec,
    typename channel<MessageSubscriber>::ptr channel)
{
    // The start failure is also caught by handle_channel_stop.
    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Outbound channel failed to start ["
            << channel->authority() << "] " << ec.message();
        return;
    }

    LOG_INFO(LOG_NETWORK)
        << "Connected outbound channel [" << channel->authority() << "] ("
        << this->connection_count() << ")";

    attach_protocols(channel);
};

template<class MessageSubscriber>
void session_outbound<MessageSubscriber>::attach_protocols(typename channel<MessageSubscriber>::ptr channel)
{
    const auto version = channel->negotiated_version();

    if (version >= message::version::level::bip31)
        this->template attach<protocol_ping_60001<MessageSubscriber>>(channel)->start();
    else
        this->template attach<protocol_ping_31402<MessageSubscriber>>(channel)->start();

    if (version >= message::version::level::bip61)
        this->template attach<protocol_reject_70002<MessageSubscriber>>(channel)->start();

    this->template attach<protocol_address_31402<MessageSubscriber>>(channel)->start();
}

template<class MessageSubscriber>
void session_outbound<MessageSubscriber>::attach_handshake_protocols(typename channel<MessageSubscriber>::ptr channel,
    result_handler handle_started)
{
    using serve = message::version::service;
    const auto relay = this->settings_.relay_transactions;
    const auto own_version = this->settings_.protocol_maximum;
    const auto own_services = this->settings_.services;
    const auto invalid_services = this->settings_.invalid_services;
    const auto minimum_version = this->settings_.protocol_minimum;

    // Require peer to serve network (and witness if configured on self).
    const auto minimum_services = (own_services & serve::node_witness) |
        serve::node_network;

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

template<class MessageSubscriber>
void session_outbound<MessageSubscriber>::handle_channel_stop(const code& ec,
    typename channel<MessageSubscriber>::ptr channel)
{
    LOG_DEBUG(LOG_NETWORK)
        << "Outbound channel stopped [" << channel->authority() << "] "
        << ec.message();

    new_connection(error::success);
}

// Channel start sequence.
// ----------------------------------------------------------------------------
// Pend outgoing connections so we can detect connection to self.

template<class MessageSubscriber>
void session_outbound<MessageSubscriber>::start_channel(typename channel<MessageSubscriber>::ptr channel,
    result_handler handle_started)
{
    const result_handler unpend_handler =
        BIND3(do_unpend, _1, channel, handle_started);

    const auto ec = this->pend(channel);

    if (ec)
    {
        unpend_handler(ec);
        return;
    }

    session<MessageSubscriber>::start_channel(channel, unpend_handler);
}

template<class MessageSubscriber>
void session_outbound<MessageSubscriber>::do_unpend(const code& ec, typename channel<MessageSubscriber>::ptr channel,
    result_handler handle_started)
{
    this->unpend(channel);
    handle_started(ec);
}

template class session_outbound<message_subscriber>;

} // namespace network
} // namespace libbitcoin
