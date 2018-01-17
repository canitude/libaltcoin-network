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
#include <altcoin/network/protocols/protocol_reject_70002.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/p2p.hpp>
#include <altcoin/network/protocols/protocol_events.hpp>

namespace libbitcoin {
namespace network {

#define NAME "reject"
#define CLASS protocol_reject_70002<MessageSubscriber>

using namespace bc::message;
using namespace std::placeholders;

template<class MessageSubscriber>
protocol_reject_70002<MessageSubscriber>::protocol_reject_70002(p2p<MessageSubscriber>& network,
    typename channel<MessageSubscriber>::ptr channel)
  : protocol_events<MessageSubscriber>(network, channel, NAME),
    CONSTRUCT_TRACK(protocol_reject_70002<MessageSubscriber>)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void protocol_reject_70002<MessageSubscriber>::start()
{
    protocol_events<MessageSubscriber>::start();

    SUBSCRIBE2(reject, handle_receive_reject, _1, _2);
}

// Protocol.
// ----------------------------------------------------------------------------

// TODO: mitigate log fill DOS.
template<class MessageSubscriber>
bool protocol_reject_70002<MessageSubscriber>::handle_receive_reject(const code& ec,
    reject_const_ptr reject)
{
    if (this->stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving reject from [" << this->authority() << "] "
            << ec.message();
        this->stop(error::channel_stopped);
        return false;
    }

    const auto& message = reject->message();

    // Handle these in the version protocol.
    if (message == version::command)
        return true;

    std::string hash;
    if (message == block::command || message == transaction::command)
        hash = " [" + encode_hash(reject->data()) + "].";

    const auto code = reject->code();
    LOG_DEBUG(LOG_NETWORK)
        << "Received " << message << " reject (" << static_cast<uint16_t>(code)
        << ") from [" << this->authority() << "] '" << reject->reason()
        << "'" << hash;
    return true;
}

template class protocol_reject_70002<message_subscriber>;

} // namespace network
} // namespace libbitcoin
