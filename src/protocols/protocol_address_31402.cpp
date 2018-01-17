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
#include <altcoin/network/protocols/protocol_address_31402.hpp>

#include <ctime>
#include <functional>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/protocols/protocol.hpp>
#include <altcoin/network/protocols/protocol_events.hpp>

namespace libbitcoin {
namespace network {

#define NAME "address"
#define CLASS protocol_address_31402<MessageSubscriber>

using namespace bc::message;
using namespace std::placeholders;

static message::address configured_self(const network::settings& settings)
{
    if (settings.self.port() == 0)
        return address{};

    network_address netaddr = settings.self.to_network_address();
    netaddr.set_timestamp(static_cast<uint32_t>(time(nullptr)));
    netaddr.set_services(settings.services);

    network_address::list l;
    l.push_back(netaddr);

    return message::address(l);
}

template<class MessageSubscriber>
protocol_address_31402<MessageSubscriber>::protocol_address_31402(p2p<MessageSubscriber>& network,
    typename channel<MessageSubscriber>::ptr channel)
  : protocol_events<MessageSubscriber>(network, channel, NAME),
    network_(network),
    self_(configured_self(network_.network_settings())),
    CONSTRUCT_TRACK(protocol_address_31402<MessageSubscriber>)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

class ZZZ { };

template<class MessageSubscriber>
void protocol_address_31402<MessageSubscriber>::start()
{
    const auto& settings = network_.network_settings();

    // Must have a handler to capture a shared self pointer in stop subscriber.
    protocol_events<MessageSubscriber>::start(BIND1(handle_stop, _1));

    if (!self_.addresses().empty())
    {
        SEND2(self_, handle_send, _1, self_.command);
    }

    // If we can't store addresses we don't ask for or handle them.
    if (settings.host_pool_capacity == 0)
        return;

    SUBSCRIBE2(address, handle_receive_address, _1, _2);
    SUBSCRIBE2(get_address, handle_receive_get_address, _1, _2);
    SEND2(get_address{}, handle_send, _1, get_address::command);
}

// Protocol.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
bool protocol_address_31402<MessageSubscriber>::handle_receive_address(const code& ec,
    address_const_ptr message)
{
    if (this->stopped(ec))
        return false;

    LOG_DEBUG(LOG_NETWORK)
        << "Storing addresses from [" << this->authority() << "] ("
        << message->addresses().size() << ")";

    // TODO: manage timestamps (active channels are connected < 3 hours ago).
    network_.store(message->addresses(), BIND1(handle_store_addresses, _1));

    // RESUBSCRIBE
    return true;
}

template<class MessageSubscriber>
bool protocol_address_31402<MessageSubscriber>::handle_receive_get_address(const code& ec,
    get_address_const_ptr message)
{
    if (this->stopped(ec))
        return false;

    // TODO: allowing repeated queries can allow a channel to map our history.
    // TODO: pull active hosts from host cache (currently just resending self).
    // TODO: need to distort for privacy, don't send currently-connected peers.
    // TODO: response size limit is max_address (1000).

    if (self_.addresses().empty())
        return false;

    LOG_DEBUG(LOG_NETWORK)
        << "Sending addresses to [" << this->authority() << "] ("
        << self_.addresses().size() << ")";

    SEND2(self_, handle_send, _1, self_.command);

    // RESUBSCRIBE
    return true;
}

template<class MessageSubscriber>
void protocol_address_31402<MessageSubscriber>::handle_store_addresses(const code& ec)
{
    if (this->stopped(ec))
        return;

    if (ec)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Failure storing addresses from [" << this->authority() << "] "
            << ec.message();
        this->stop(ec);
    }
}

template<class MessageSubscriber>
void protocol_address_31402<MessageSubscriber>::handle_stop(const code&)
{
    // None of the other bc::network protocols log their stop.
    ////LOG_DEBUG(LOG_NETWORK)
    ////    << "Stopped address protocol for [" << authority() << "].";
}

template class protocol_address_31402<message_subscriber>;

} // namespace network
} // namespace libbitcoin
