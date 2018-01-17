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
#ifndef LIBBITCOIN_NETWORK_SESSION_OUTBOUND_HPP
#define LIBBITCOIN_NETWORK_SESSION_OUTBOUND_HPP

#include <cstddef>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/sessions/session_batch.hpp>
#include <altcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

template <class MessageSubscriber> class p2p;

/// Outbound connections session, thread safe.
template <class MessageSubscriber>
class BCT_API session_outbound
  : public session_batch<MessageSubscriber>, track<session_outbound<MessageSubscriber>>
{
public:
    typedef std::shared_ptr<session_outbound<MessageSubscriber>> ptr;
    typedef typename session<MessageSubscriber>::result_handler result_handler;

    /// Construct an instance.
    session_outbound(p2p<MessageSubscriber>& network, bool notify_on_connect);

    /// Start the session.
    void start(result_handler handler) override;

protected:
    /// Overridden to implement pending outbound channels.
    void start_channel(typename channel<MessageSubscriber>::ptr channel,
        result_handler handle_started) override;

    /// Overridden to attach minimum service level for witness support.
    virtual void attach_handshake_protocols(typename channel<MessageSubscriber>::ptr channel,
        result_handler handle_started) override;

    /// Override to attach specialized protocols upon channel start.
    virtual void attach_protocols(typename channel<MessageSubscriber>::ptr channel);

private:
    void new_connection(const code&);

    void handle_started(const code& ec, result_handler handler);
    void handle_connect(const code& ec, typename channel<MessageSubscriber>::ptr channel);

    void do_unpend(const code& ec, typename channel<MessageSubscriber>::ptr channel,
        result_handler handle_started);

    void handle_channel_stop(const code& ec, typename channel<MessageSubscriber>::ptr channel);
    void handle_channel_start(const code& ec, typename channel<MessageSubscriber>::ptr channel);
};

} // namespace network
} // namespace libbitcoin

#endif
