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
#include <altcoin/network/sessions/session_batch.hpp>

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/connector.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_batch<MessageSubscriber>
#define NAME "session_batch"

using namespace bc::config;
using namespace bc::message;
using namespace std::placeholders;

template<class MessageSubscriber>
session_batch<MessageSubscriber>::session_batch(p2p<MessageSubscriber>& network, bool notify_on_connect)
  : session<MessageSubscriber>(network, notify_on_connect),
    batch_size_(std::max(this->settings_.connect_batch_size, 1u))
{
}

// Connect sequence.
// ----------------------------------------------------------------------------

// protected:
template<class MessageSubscriber>
void session_batch<MessageSubscriber>::connect(channel_handler handler)
{
    const auto join_handler = synchronize(handler, batch_size_, NAME "_join",
        synchronizer_terminate::on_success);

    for (size_t host = 0; host < batch_size_; ++host)
        new_connect(join_handler);
}

template<class MessageSubscriber>
void session_batch<MessageSubscriber>::new_connect(channel_handler handler)
{
    if (this->stopped())
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Suspended batch connection.";
        handler(error::channel_stopped, nullptr);
        return;
    }

    network_address address;
    const auto ec = this->fetch_address(address);
    start_connect(ec, address, handler);
}

template<class MessageSubscriber>
void session_batch<MessageSubscriber>::start_connect(const code& ec, const authority& host,
    channel_handler handler)
{
    if (this->stopped(ec))
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Batch session stopped while starting.";
        handler(error::service_stopped, nullptr);
        return;
    }

    // This termination prevents a tight loop in the empty address pool case.
    if (ec)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Failure fetching new address: " << ec.message();
        handler(ec, nullptr);
        return;
    }

    // This creates a tight loop in the case of a small address pool.
    if (this->blacklisted(host))
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Fetched blacklisted address [" << host << "] ";
        handler(error::address_blocked, nullptr);
        return;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Connecting to [" << host << "]";

    const auto connector = this->create_connector();
    this->pend(connector);

    // CONNECT
    connector->connect(host,
        BIND4(handle_connect, _1, _2, connector, handler));
}

template<class MessageSubscriber>
void session_batch<MessageSubscriber>::handle_connect(const code& ec, typename channel<MessageSubscriber>::ptr channel,
    typename connector<MessageSubscriber>::ptr connector, channel_handler handler)
{
    this->unpend(connector);

    if (ec)
    {
        handler(ec, nullptr);
        return;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Connected to [" << channel->authority() << "]";

    // This is the end of the connect sequence.
    handler(error::success, channel);
}

template class session_batch<message_subscriber>;

} // namespace network
} // namespace libbitcoin
