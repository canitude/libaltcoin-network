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
#include <altcoin/network/protocols/protocol_timer.hpp>

#include <functional>
#include <memory>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/protocols/protocol_events.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_timer
using namespace std::placeholders;

template<class MessageSubscriber>
protocol_timer<MessageSubscriber>::protocol_timer(p2p<MessageSubscriber>& network, typename channel<MessageSubscriber>::ptr channel,
    bool perpetual, const std::string& name)
  : protocol_events<MessageSubscriber>(network, channel, name),
    perpetual_(perpetual)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

// protected:
template<class MessageSubscriber>
void protocol_timer<MessageSubscriber>::start(const asio::duration& timeout,
    event_handler handle_event)
{
    // The deadline timer is thread safe.
    timer_ = std::make_shared<deadline>(this->pool(), timeout);
    protocol_events<MessageSubscriber>::start(BIND2(handle_notify, _1, handle_event));
    reset_timer();
}

template<class MessageSubscriber>
void protocol_timer<MessageSubscriber>::handle_notify(const code& ec, event_handler handler)
{
    if (ec == error::channel_stopped)
        timer_->stop();

    handler(ec);
}

// Timer.
// ----------------------------------------------------------------------------

// protected:
template<class MessageSubscriber>
void protocol_timer<MessageSubscriber>::reset_timer()
{
    if (this->stopped())
        return;

    timer_->start(BIND1(handle_timer, _1));
}

template<class MessageSubscriber>
void protocol_timer<MessageSubscriber>::handle_timer(const code& ec)
{
    if (this->stopped())
        return;

    LOG_DEBUG(LOG_NETWORK)
        << "Fired protocol_" << this->name() << " timer on [" << this->authority() << "] "
        << ec.message();

    // The handler completes before the timer is reset.
    this->set_event(error::channel_timeout);

    // A perpetual timer resets itself until the channel is stopped.
    if (perpetual_)
        reset_timer();
}

template class protocol_timer<message_subscriber>;

} // namespace network
} // namespace libbitcoin
