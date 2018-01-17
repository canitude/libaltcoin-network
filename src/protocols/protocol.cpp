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
#include <altcoin/network/protocols/protocol.hpp>

#include <cstdint>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/p2p.hpp>

namespace libbitcoin {
namespace network {

#define NAME "protocol"

template<class MessageSubscriber>
protocol<MessageSubscriber>::protocol(p2p<MessageSubscriber>& network, typename channel<MessageSubscriber>::ptr channel, const std::string& name)
  : pool_(network.thread_pool()),
    dispatch_(network.thread_pool(), NAME),
    channel_(channel),
    name_(name)
{
}

template<class MessageSubscriber>
config::authority protocol<MessageSubscriber>::authority() const
{
    return channel_->authority();
}

template<class MessageSubscriber>
const std::string& protocol<MessageSubscriber>::name() const
{
    return name_;
}

template<class MessageSubscriber>
uint64_t protocol<MessageSubscriber>::nonce() const
{
    return channel_->nonce();
}

template<class MessageSubscriber>
version_const_ptr protocol<MessageSubscriber>::peer_version() const
{
    return channel_->peer_version();
}

template<class MessageSubscriber>
void protocol<MessageSubscriber>::set_peer_version(version_const_ptr value)
{
    channel_->set_peer_version(value);
}

template<class MessageSubscriber>
uint32_t protocol<MessageSubscriber>::negotiated_version() const
{
    return channel_->negotiated_version();
}

template<class MessageSubscriber>
void protocol<MessageSubscriber>::set_negotiated_version(uint32_t value)
{
    channel_->set_negotiated_version(value);
}

template<class MessageSubscriber>
threadpool& protocol<MessageSubscriber>::pool()
{
    return pool_;
}

// Stop the channel.
template<class MessageSubscriber>
void protocol<MessageSubscriber>::stop(const code& ec)
{
    channel_->stop(ec);
}

// protected
template<class MessageSubscriber>
void protocol<MessageSubscriber>::handle_send(const code& ec, const std::string& command)
{
    // Send and receive failures are logged by the proxy.
    // This provides a convenient location for override if desired.
}

template class protocol<message_subscriber>;

} // namespace network
} // namespace libbitcoin
