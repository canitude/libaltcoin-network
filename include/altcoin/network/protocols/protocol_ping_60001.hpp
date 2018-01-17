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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_PING_60001_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_PING_60001_HPP

#include <atomic>
#include <cstdint>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/protocols/protocol_ping_31402.hpp>
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
class BCT_API protocol_ping_60001
  : public protocol_ping_31402<MessageSubscriber>, track<protocol_ping_60001<MessageSubscriber>>
{
public:
    typedef std::shared_ptr<protocol_ping_60001<MessageSubscriber>> ptr;
    typedef typename protocol_ping_31402<MessageSubscriber>::event_handler event_handler;

    /**
     * Construct a ping protocol instance.
     * @param[in]  network   The network interface.
     * @param[in]  channel   The channel on which to start the protocol.
     */
    protocol_ping_60001(p2p<MessageSubscriber>& network, typename channel<MessageSubscriber>::ptr channel);

protected:
    void send_ping(const code& ec) override;

    void handle_send_ping(const code& ec, const std::string& command);
    bool handle_receive_ping(const code& ec, ping_const_ptr message) override;
    virtual bool handle_receive_pong(const code& ec, pong_const_ptr message,
        uint64_t nonce);

private:
    std::atomic<bool> pending_;
};

} // namespace network
} // namespace libbitcoin

#endif
