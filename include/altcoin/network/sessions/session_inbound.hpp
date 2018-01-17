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
#ifndef LIBBITCOIN_NETWORK_SESSION_INBOUND_HPP
#define LIBBITCOIN_NETWORK_SESSION_INBOUND_HPP

#include <cstddef>
#include <memory>
#include <vector>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/acceptor.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/sessions/session.hpp>
#include <altcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

template <class MessageSubscriber> class p2p;

/// Inbound connections session, thread safe.
template <class MessageSubscriber>
class BCT_API session_inbound
  : public session<MessageSubscriber>, track<session_inbound<MessageSubscriber>>
{
public:
    typedef std::shared_ptr<session_inbound<MessageSubscriber>> ptr;
    typedef typename session<MessageSubscriber>::result_handler result_handler;

    /// Construct an instance.
    session_inbound(p2p<MessageSubscriber>& network, bool notify_on_connect);

    /// Start the session.
    void start(result_handler handler) override;

protected:
    /// Overridden to implement pending test for inbound channels.
    void handshake_complete(typename channel<MessageSubscriber>::ptr channel,
        result_handler handle_started) override;

    /// Override to attach specialized protocols upon channel start.
    virtual void attach_protocols(typename channel<MessageSubscriber>::ptr channel);

private:
    void start_accept(const code& ec);

    void handle_stop(const code& ec);
    void handle_started(const code& ec, result_handler handler);
    void handle_accept(const code& ec, typename channel<MessageSubscriber>::ptr channel);

    void handle_channel_start(const code& ec, typename channel<MessageSubscriber>::ptr channel);
    void handle_channel_stop(const code& ec);

    // These are thread safe.
    typename acceptor<MessageSubscriber>::ptr acceptor_;
    const size_t connection_limit_;
};

} // namespace network
} // namespace libbitcoin

#endif
