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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_PING_31402_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_PING_31402_HPP

#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/protocols/protocol_timer.hpp>
#include <altcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

template <class MessageSubscriber> class p2p;

/**
 * Ping-pong protocol.
 * Attach this to a channel immediately following handshake completion.
 */
template <class MessageSubscriber>
class BCT_API protocol_ping_31402
  : public protocol_timer<MessageSubscriber>, track<protocol_ping_31402<MessageSubscriber>>
{
public:
    typedef std::shared_ptr<protocol_ping_31402<MessageSubscriber>> ptr;

    /**
     * Construct a ping protocol instance.
     * @param[in]  network   The network interface.
     * @param[in]  channel   The channel on which to start the protocol.
     */
    protocol_ping_31402(p2p<MessageSubscriber>& network, typename channel<MessageSubscriber>::ptr channel);

    /**
     * Start the protocol.
     */
    virtual void start();

protected:
    virtual void send_ping(const code& ec);

    virtual bool handle_receive_ping(const code& ec, ping_const_ptr message);

    const settings& settings_;
};

} // namespace network
} // namespace libbitcoin

#endif
