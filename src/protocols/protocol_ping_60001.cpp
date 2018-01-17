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
#include <altcoin/network/protocols/protocol_ping_60001.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/protocols/protocol_ping_31402.hpp>
#include <altcoin/network/protocols/protocol_timer.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_ping_60001<MessageSubscriber>

using namespace bc::message;
using namespace std::placeholders;

template<class MessageSubscriber>
protocol_ping_60001<MessageSubscriber>::protocol_ping_60001(p2p<MessageSubscriber>& network, typename channel<MessageSubscriber>::ptr channel)
  : protocol_ping_31402<MessageSubscriber>(network, channel),
    pending_(false),
    CONSTRUCT_TRACK(protocol_ping_60001<MessageSubscriber>)
{
}

// This is fired by the callback (i.e. base timer and stop handler).
template<class MessageSubscriber>
void protocol_ping_60001<MessageSubscriber>::send_ping(const code& ec)
{
    if (this->stopped(ec))
        return;

    if (ec && ec != error::channel_timeout)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure in ping timer for [" << this->authority() << "] "
            << ec.message();
        this->stop(ec);
        return;
    }

    if (pending_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Ping latency limit exceeded [" << this->authority() << "]";
        this->stop(error::channel_timeout);
        return;
    }

    pending_ = true;
    const auto nonce = pseudo_random();
    SUBSCRIBE3(pong, handle_receive_pong, _1, _2, nonce);
    SEND2(ping{ nonce }, handle_send_ping, _1, ping::command);
}

template<class MessageSubscriber>
void protocol_ping_60001<MessageSubscriber>::handle_send_ping(const code& ec, const std::string&)
{
    if (this->stopped(ec))
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure sending ping to [" << this->authority() << "] "
            << ec.message();
        this->stop(ec);
        return;
    }
}

template<class MessageSubscriber>
bool protocol_ping_60001<MessageSubscriber>::handle_receive_ping(const code& ec,
    ping_const_ptr message)
{
    if (this->stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure getting ping from [" << this->authority() << "] "
            << ec.message();
        this->stop(ec);
        return false;
    }

    SEND2(pong{ message->nonce() }, handle_send, _1, pong::command);
    return true;
}

template<class MessageSubscriber>
bool protocol_ping_60001<MessageSubscriber>::handle_receive_pong(const code& ec,
    pong_const_ptr message, uint64_t nonce)
{
    if (this->stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure getting pong from [" << this->authority() << "] "
            << ec.message();
        this->stop(ec);
        return false;
    }

    pending_ = false;

    if (message->nonce() != nonce)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid pong nonce from [" << this->authority() << "]";
        this->stop(error::bad_stream);
        return false;
    }

    return false;
}

template class protocol_ping_60001<message_subscriber>;

} // namespace network
} // namespace libbitcoin
