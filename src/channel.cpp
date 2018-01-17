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
#include <altcoin/network/channel.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/proxy.hpp>
#include <altcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::message;
using namespace std::placeholders;

// Factory for deadline timer pointer construction.
static deadline::ptr alarm(threadpool& pool, const asio::duration& duration)
{
    return std::make_shared<deadline>(pool, pseudo_randomize(duration));
}

template<class MessageSubscriber>
channel<MessageSubscriber>::channel(threadpool& pool, socket::ptr socket,
    const settings& settings)
  : proxy<MessageSubscriber>(pool, socket, settings),
    notify_(false),
    nonce_(0),
    expiration_(alarm(pool, settings.channel_expiration())),
    inactivity_(alarm(pool, settings.channel_inactivity())),
    CONSTRUCT_TRACK(channel<MessageSubscriber>)
{
}

// Talk sequence.
// ----------------------------------------------------------------------------

// public:
template<class MessageSubscriber>
void channel<MessageSubscriber>::start(result_handler handler)
{
    proxy<MessageSubscriber>::start(
        std::bind(&channel<MessageSubscriber>::do_start,
            channel<MessageSubscriber>::template shared_from_base<channel<MessageSubscriber>>(), _1, handler));
}

// Don't start the timers until the socket is enabled.
template<class MessageSubscriber>
void channel<MessageSubscriber>::do_start(const code& ec, result_handler handler)
{
    start_expiration();
    start_inactivity();
    handler(error::success);
}

// Properties.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
bool channel<MessageSubscriber>::notify() const
{
    return notify_;
}

template<class MessageSubscriber>
void channel<MessageSubscriber>::set_notify(bool value)
{
    notify_ = value;
}

template<class MessageSubscriber>
uint64_t channel<MessageSubscriber>::nonce() const
{
    return nonce_;
}

template<class MessageSubscriber>
void channel<MessageSubscriber>::set_nonce(uint64_t value)
{
    nonce_.store(value);
}

template<class MessageSubscriber>
version_const_ptr channel<MessageSubscriber>::peer_version() const
{
    const auto version = peer_version_.load();
    BITCOIN_ASSERT_MSG(version, "Read peer version before set.");
    return version;
}

template<class MessageSubscriber>
void channel<MessageSubscriber>::set_peer_version(version_const_ptr value)
{
    peer_version_.store(value);
}

// Proxy pure virtual protected and ordered handlers.
// ----------------------------------------------------------------------------

// It is possible that this may be called multiple times.
template<class MessageSubscriber>
void channel<MessageSubscriber>::handle_stopping()
{
    expiration_->stop();
    inactivity_->stop();
}

template<class MessageSubscriber>
void channel<MessageSubscriber>::signal_activity()
{
    start_inactivity();
}

template<class MessageSubscriber>
bool channel<MessageSubscriber>::stopped(const code& ec) const
{
    return proxy<MessageSubscriber>::stopped() || ec == error::channel_stopped ||
        ec == error::service_stopped;
}

// Timers (these are inherent races, requiring stranding by stop only).
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void channel<MessageSubscriber>::start_expiration()
{
    if (proxy<MessageSubscriber>::stopped())
        return;

    expiration_->start(
        std::bind(&channel<MessageSubscriber>::handle_expiration,
            channel<MessageSubscriber>::template shared_from_base<channel<MessageSubscriber>>(), _1));
}

template<class MessageSubscriber>
void channel<MessageSubscriber>::handle_expiration(const code& ec)
{
    if (stopped(ec))
        return;

    LOG_DEBUG(LOG_NETWORK)
        << "Channel lifetime expired [" << this->authority() << "]";

    this->stop(error::channel_timeout);
}

template<class MessageSubscriber>
void channel<MessageSubscriber>::start_inactivity()
{
    if (proxy<MessageSubscriber>::stopped())
        return;

    inactivity_->start(
        std::bind(&channel<MessageSubscriber>::handle_inactivity,
            channel<MessageSubscriber>::template shared_from_base<channel<MessageSubscriber>>(), _1));
}

template<class MessageSubscriber>
void channel<MessageSubscriber>::handle_inactivity(const code& ec)
{
    if (stopped(ec))
        return;

    LOG_DEBUG(LOG_NETWORK)
        << "Channel inactivity timeout [" << this->authority() << "]";

    this->stop(error::channel_timeout);
}

template class channel<message_subscriber>;

} // namespace network
} // namespace libbitcoin
