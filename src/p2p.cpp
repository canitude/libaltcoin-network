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
#include <altcoin/network/p2p.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/hosts.hpp>
#include <altcoin/network/protocols/protocol_address_31402.hpp>
#include <altcoin/network/protocols/protocol_ping_31402.hpp>
#include <altcoin/network/protocols/protocol_ping_60001.hpp>
#include <altcoin/network/protocols/protocol_seed_31402.hpp>
#include <altcoin/network/protocols/protocol_version_31402.hpp>
#include <altcoin/network/protocols/protocol_version_70002.hpp>
#include <altcoin/network/sessions/session_inbound.hpp>
#include <altcoin/network/sessions/session_manual.hpp>
#include <altcoin/network/sessions/session_outbound.hpp>
#include <altcoin/network/sessions/session_seed.hpp>
#include <altcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "p2p"

using namespace bc::config;
using namespace std::placeholders;

// This can be exceeded due to manual connection calls and race conditions.
inline size_t nominal_connecting(const settings& settings)
{
    return settings.peers.size() + settings.connect_batch_size *
        settings.outbound_connections;
}

// This can be exceeded due to manual connection calls and race conditions.
inline size_t nominal_connected(const settings& settings)
{
    return settings.peers.size() + settings.outbound_connections +
        settings.inbound_connections;
}

template<class MessageSubscriber>
p2p<MessageSubscriber>::p2p(const settings& settings)
  : settings_(settings),
    stopped_(true),
    top_block_({ null_hash, 0 }),
    hosts_(settings_),
    pending_connect_(nominal_connecting(settings_)),
    pending_handshake_(nominal_connected(settings_)),
    pending_close_(nominal_connected(settings_)),
    stop_subscriber_(std::make_shared<stop_subscriber>(threadpool_,
        NAME "_stop_sub")),
    channel_subscriber_(std::make_shared<channel_subscriber>(threadpool_,
        NAME "_sub"))
{
}

// This allows for shutdown based on destruct without need to call stop.
template<class MessageSubscriber>
p2p<MessageSubscriber>::~p2p()
{
    p2p<MessageSubscriber>::close();
}

// Start sequence.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void p2p<MessageSubscriber>::start(result_handler handler)
{
    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    threadpool_.join();
    threadpool_.spawn(thread_default(settings_.threads),
        thread_priority::normal);

    stopped_ = false;
    stop_subscriber_->start();
    channel_subscriber_->start();

    // This instance is retained by stop handler and member reference.
    manual_.store(attach_manual_session());

    // This is invoked on a new thread.
    manual_.load()->start(
        std::bind(&p2p<MessageSubscriber>::handle_manual_started,
            this, _1, handler));
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::handle_manual_started(const code& ec, result_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    if (ec)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Error starting manual session: " << ec.message();
        handler(ec);
        return;
    }

    handle_hosts_loaded(hosts_.start(), handler);
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::handle_hosts_loaded(const code& ec, result_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    if (ec)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Error loading host addresses: " << ec.message();
        handler(ec);
        return;
    }

    // The instance is retained by the stop handler (until shutdown).
    const auto seed = attach_seed_session();

    // This is invoked on a new thread.
    seed->start(
        std::bind(&p2p<MessageSubscriber>::handle_started,
            this, _1, handler));
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::handle_started(const code& ec, result_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    if (ec)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Error seeding host addresses: " << ec.message();
        handler(ec);
        return;
    }

    // There is no way to guarantee subscription before handler execution.
    // So currently subscription for seed node connections is not supported.
    // Subscription after this return will capture connections established via
    // subsequent "run" and "connect" calls, and will clear on close/destruct.

    // This is the end of the start sequence.
    handler(error::success);
}

// Run sequence.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void p2p<MessageSubscriber>::run(result_handler handler)
{
    // Start node.peer persistent connections.
    for (const auto& peer: settings_.peers)
        connect(peer);

    // The instance is retained by the stop handler (until shutdown).
    const auto inbound = attach_inbound_session();

    // This is invoked on a new thread.
    inbound->start(
        std::bind(&p2p<MessageSubscriber>::handle_inbound_started,
            this, _1, handler));
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::handle_inbound_started(const code& ec, result_handler handler)
{
    if (ec)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Error starting inbound session: " << ec.message();
        handler(ec);
        return;
    }

    // The instance is retained by the stop handler (until shutdown).
    const auto outbound = attach_outbound_session();

    // This is invoked on a new thread.
    outbound->start(
        std::bind(&p2p<MessageSubscriber>::handle_running,
            this, _1, handler));
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::handle_running(const code& ec, result_handler handler)
{
    if (ec)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Error starting outbound session: " << ec.message();
        handler(ec);
        return;
    }

    // This is the end of the run sequence.
    handler(error::success);
}

// Specializations.
// ----------------------------------------------------------------------------
// Create derived sessions and override these to inject from derived p2p class.

template<class MessageSubscriber>
typename session_seed<MessageSubscriber>::ptr p2p<MessageSubscriber>::attach_seed_session()
{
    return attach<session_seed<MessageSubscriber>>();
}

template<class MessageSubscriber>
typename session_manual<MessageSubscriber>::ptr p2p<MessageSubscriber>::attach_manual_session()
{
    return attach<session_manual<MessageSubscriber>>(true);
}

template<class MessageSubscriber>
typename session_inbound<MessageSubscriber>::ptr p2p<MessageSubscriber>::attach_inbound_session()
{
    return attach<session_inbound<MessageSubscriber>>(true);
}

template<class MessageSubscriber>
typename session_outbound<MessageSubscriber>::ptr p2p<MessageSubscriber>::attach_outbound_session()
{
    return attach<session_outbound<MessageSubscriber>>(true);
}

// Shutdown.
// ----------------------------------------------------------------------------
// All shutdown actions must be queued by the end of the stop call.
// IOW queued shutdown operations must not enqueue additional work.

// This is not short-circuited by a stopped test because we need to ensure it
// completes at least once before returning. This requires a unique lock be
// taken around the entire section, which poses a deadlock risk. Instead this
// is thread safe and idempotent, allowing it to be unguarded.
template<class MessageSubscriber>
bool p2p<MessageSubscriber>::stop()
{
    // This is the only stop operation that can fail.
    const auto result = (hosts_.stop() == error::success);

    // Signal all current work to stop and free manual session.
    stopped_ = true;
    manual_.store({});

    // Prevent subscription after stop.
    stop_subscriber_->stop();
    stop_subscriber_->invoke(error::service_stopped);

    // Prevent subscription after stop.
    channel_subscriber_->stop();
    channel_subscriber_->invoke(error::service_stopped, {});

    // Stop creating new channels and stop those that exist (self-clearing).
    pending_connect_.stop(error::service_stopped);
    pending_handshake_.stop(error::service_stopped);
    pending_close_.stop(error::service_stopped);

    // Signal threadpool to stop accepting work now that subscribers are clear.
    threadpool_.shutdown();
    return result;
}

// This must be called from the thread that constructed this class (see join).
template<class MessageSubscriber>
bool p2p<MessageSubscriber>::close()
{
    // Signal current work to stop and threadpool to stop accepting new work.
    const auto result = p2p<MessageSubscriber>::stop();

    // Block on join of all threads in the threadpool.
    threadpool_.join();
    return result;
}

// Properties.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
const settings& p2p<MessageSubscriber>::network_settings() const
{
    return settings_;
}

template<class MessageSubscriber>
checkpoint p2p<MessageSubscriber>::top_block() const
{
    return top_block_.load();
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::set_top_block(checkpoint&& top)
{
    top_block_.store(std::move(top));
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::set_top_block(const checkpoint& top)
{
    top_block_.store(top);
}

template<class MessageSubscriber>
bool p2p<MessageSubscriber>::stopped() const
{
    return stopped_;
}

template<class MessageSubscriber>
threadpool& p2p<MessageSubscriber>::thread_pool()
{
    return threadpool_;
}

// Send.
// ----------------------------------------------------------------------------

// private
template<class MessageSubscriber>
void p2p<MessageSubscriber>::handle_send(const code& ec, typename channel<MessageSubscriber>::ptr channel,
    channel_handler handle_channel, result_handler handle_complete)
{
    handle_channel(ec, channel);
    handle_complete(ec);
}

// Subscriptions.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void p2p<MessageSubscriber>::subscribe_connection(connect_handler handler)
{
    channel_subscriber_->subscribe(handler, error::service_stopped, {});
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::subscribe_stop(result_handler handler)
{
    stop_subscriber_->subscribe(handler, error::service_stopped);
}

// Manual connections.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
void p2p<MessageSubscriber>::connect(const config::endpoint& peer)
{
    connect(peer.host(), peer.port());
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::connect(const std::string& hostname, uint16_t port)
{
    if (stopped())
        return;

    auto manual = manual_.load();

    // Connect is invoked on a new thread.
    if (manual)
        manual->connect(hostname, port);
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::connect(const std::string& hostname, uint16_t port,
    channel_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped, {});
        return;
    }

    auto manual = manual_.load();

    if (manual)
        manual->connect(hostname, port, handler);
}

// Hosts collection.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
size_t p2p<MessageSubscriber>::address_count() const
{
    return hosts_.count();
}

template<class MessageSubscriber>
code p2p<MessageSubscriber>::store(const address& address)
{
    return hosts_.store(address);
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::store(const address::list& addresses, result_handler handler)
{
    // Store is invoked on a new thread.
    hosts_.store(addresses, handler);
}

template<class MessageSubscriber>
code p2p<MessageSubscriber>::fetch_address(address& out_address) const
{
    return hosts_.fetch(out_address);
}

template<class MessageSubscriber>
code p2p<MessageSubscriber>::remove(const address& address)
{
    return hosts_.remove(address);
}

// Pending connect collection.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
code p2p<MessageSubscriber>::pend(typename connector<MessageSubscriber>::ptr connector)
{
    return pending_connect_.store(connector);
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::unpend(typename connector<MessageSubscriber>::ptr connector)
{
    connector->stop(error::success);
    pending_connect_.remove(connector);
}

// Pending handshake collection.
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
code p2p<MessageSubscriber>::pend(typename channel<MessageSubscriber>::ptr channel)
{
    return pending_handshake_.store(channel);
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::unpend(typename channel<MessageSubscriber>::ptr channel)
{
    pending_handshake_.remove(channel);
}

template<class MessageSubscriber>
bool p2p<MessageSubscriber>::pending(uint64_t version_nonce) const
{
    const auto match = [version_nonce](const typename channel<MessageSubscriber>::ptr& element)
    {
        return element->nonce() == version_nonce;
    };

    return pending_handshake_.exists(match);
}

// Pending close collection (open connections).
// ----------------------------------------------------------------------------

template<class MessageSubscriber>
size_t p2p<MessageSubscriber>::connection_count() const
{
    return pending_close_.size();
}

template<class MessageSubscriber>
bool p2p<MessageSubscriber>::connected(const address& address) const
{
    const auto match = [&address](const typename channel<MessageSubscriber>::ptr& element)
    {
        return element->authority() == address;
    };

    return pending_close_.exists(match);
}

template<class MessageSubscriber>
code p2p<MessageSubscriber>::store(typename channel<MessageSubscriber>::ptr channel)
{
    const auto address = channel->authority();
    const auto match = [&address](const typename libbitcoin::network::channel<MessageSubscriber>::ptr& element)
    {
        return element->authority() == address;
    };

    // May return error::address_in_use.
    const auto ec = pending_close_.store(channel, match);

    if (!ec && channel->notify())
        channel_subscriber_->relay(error::success, channel);

    return ec;
}

template<class MessageSubscriber>
void p2p<MessageSubscriber>::remove(typename channel<MessageSubscriber>::ptr channel)
{
    pending_close_.remove(channel);
}

template class p2p<message_subscriber>;

} // namespace network
} // namespace libbitcoin
