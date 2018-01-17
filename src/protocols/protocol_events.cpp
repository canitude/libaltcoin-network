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
#include <altcoin/network/protocols/protocol_events.hpp>

#include <functional>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/protocols/protocol.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_events<MessageSubscriber>

using namespace std::placeholders;

template<class MessageSubscriber>
protocol_events<MessageSubscriber>::protocol_events(p2p<MessageSubscriber>& network, typename channel<MessageSubscriber>::ptr channel,
    const std::string& name)
  : protocol<MessageSubscriber>(network, channel, name)
{
}

// Properties.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
bool protocol_events<MessageSubscriber>::stopped() const
{
    // Used for context-free stop testing.
    return !handler_.load();
}

template<class MessageSubscriber>
bool protocol_events<MessageSubscriber>::stopped(const code& ec) const
{
    // The service stop code may also make its way into protocol handlers.
    return stopped() || ec == error::channel_stopped ||
        ec == error::service_stopped;
}

// Start.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void protocol_events<MessageSubscriber>::start()
{
    const auto nop = [](const code&){};
    start(nop);
}

template<class MessageSubscriber>
void protocol_events<MessageSubscriber>::start(event_handler handler)
{
    handler_.store(handler);
    SUBSCRIBE_STOP1(handle_stopped, _1);
}

// Stop.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void protocol_events<MessageSubscriber>::handle_stopped(const code& ec)
{
    if (!stopped(ec))
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Stop protocol_" << this->name() << " on [" << this->authority() << "] "
            << ec.message();
    }

    // Event handlers can depend on this code for channel stop.
    set_event(error::channel_stopped);
}

// Set Event.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void protocol_events<MessageSubscriber>::set_event(const code& ec)
{
    // If already stopped.
    auto handler = handler_.load();
    if (!handler)
        return;

    // If stopping but not yet cleared, clear event handler now.
    if (stopped(ec))
        handler_.store(nullptr);

    // Invoke event handler.
    handler(ec);
}

template class protocol_events<message_subscriber>;

} // namespace network
} // namespace libbitcoin
