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
#include <altcoin/network/protocols/protocol_ping_31402.hpp>

#include <functional>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/protocols/protocol_timer.hpp>

namespace libbitcoin {
namespace network {

#define NAME "ping"
#define CLASS protocol_ping_31402

using namespace bc::message;
using namespace std::placeholders;

template<class MessageSubscriber>
protocol_ping_31402<MessageSubscriber>::protocol_ping_31402(p2p<MessageSubscriber>& network, typename channel<MessageSubscriber>::ptr channel)
  : protocol_timer<MessageSubscriber>(network, channel, true, NAME),
    settings_(network.network_settings()),
    CONSTRUCT_TRACK(protocol_ping_31402<MessageSubscriber>)
{
}

template<class MessageSubscriber>
void protocol_ping_31402<MessageSubscriber>::start()
{
    protocol_timer<MessageSubscriber>::start(settings_.channel_heartbeat(), BIND1(send_ping, _1));

    SUBSCRIBE2(ping, handle_receive_ping, _1, _2);

    // Send initial ping message by simulating first heartbeat.
    this->set_event(error::success);
}

// This is fired by the callback (i.e. base timer and stop handler).
template<class MessageSubscriber>
void protocol_ping_31402<MessageSubscriber>::send_ping(const code& ec)
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

    SEND2(ping{}, handle_send, _1, ping::command);
}

template<class MessageSubscriber>
bool protocol_ping_31402<MessageSubscriber>::handle_receive_ping(const code& ec,
    ping_const_ptr message)
{
    if (this->stopped(ec))
        return false;

    // RESUBSCRIBE
    return true;
}

template class protocol_ping_31402<message_subscriber>;

} // namespace network
} // namespace libbitcoin
