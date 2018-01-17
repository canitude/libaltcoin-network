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
#ifndef LIBBITCOIN_NETWORK_SESSION_BATCH_HPP
#define LIBBITCOIN_NETWORK_SESSION_BATCH_HPP

#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/connector.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/sessions/session.hpp>
#include <altcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

template <class MessageSubscriber> class p2p;

/// Intermediate base class for adding batch connect sequence.
template <class MessageSubscriber>
class BCT_API session_batch
  : public session<MessageSubscriber>
{
protected:
    typedef typename session<MessageSubscriber>::channel_handler channel_handler;
    typedef typename session<MessageSubscriber>::authority authority;

    /// Construct an instance.
    session_batch(p2p<MessageSubscriber>& network, bool notify_on_connect);

    /// Create a channel from the configured number of concurrent attempts.
    virtual void connect(channel_handler handler);

    // Connect sequence
    virtual void new_connect(channel_handler handler);

    void start_connect(const code& ec, const authority& host,
        channel_handler handler);

private:
    void handle_connect(const code& ec, typename channel<MessageSubscriber>::ptr channel,
        typename connector<MessageSubscriber>::ptr connector, channel_handler handler);

    const size_t batch_size_;
};

} // namespace network
} // namespace libbitcoin

#endif
