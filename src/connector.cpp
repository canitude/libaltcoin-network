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
#include <altcoin/network/connector.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/proxy.hpp>
#include <altcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "connector"

using namespace bc::config;
using namespace std::placeholders;

template<class MessageSubscriber>
connector<MessageSubscriber>::connector(threadpool& pool, const settings& settings)
  : stopped_(false),
    pool_(pool),
    settings_(settings),
    dispatch_(pool, NAME),
    resolver_(pool.service()),
    CONSTRUCT_TRACK(connector)
{
}

template<class MessageSubscriber>
connector<MessageSubscriber>::~connector()
{
    BITCOIN_ASSERT_MSG(stopped(), "The connector was not stopped.");
}

template<class MessageSubscriber>
void connector<MessageSubscriber>::stop(const code&)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (!stopped())
    {
        mutex_.unlock_upgrade_and_lock();
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // This will asynchronously invoke the handler of the pending resolve.
        resolver_.cancel();

        if (timer_)
            timer_->stop();

        stopped_ = true;
        //---------------------------------------------------------------------
        mutex_.unlock();
        return;
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////
}

// private
template<class MessageSubscriber>
bool connector<MessageSubscriber>::stopped() const
{
    return stopped_;
}

template<class MessageSubscriber>
void connector<MessageSubscriber>::connect(const endpoint& endpoint, connect_handler handler)
{
    connect(endpoint.host(), endpoint.port(), handler);
}

template<class MessageSubscriber>
void connector<MessageSubscriber>::connect(const authority& authority, connect_handler handler)
{
    connect(authority.to_hostname(), authority.port(), handler);
}

template<class MessageSubscriber>
void connector<MessageSubscriber>::connect(const std::string& hostname, uint16_t port,
    connect_handler handler)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (stopped())
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        dispatch_.concurrent(handler, error::service_stopped, nullptr);
        return;
    }

    query_ = std::make_shared<asio::query>(hostname, std::to_string(port));

    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // async_resolve will not invoke the handler within this function.
    resolver_.async_resolve(*query_,
        std::bind(&connector<MessageSubscriber>::handle_resolve,
            this->shared_from_this(), _1, _2, handler));

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

template<class MessageSubscriber>
void connector<MessageSubscriber>::handle_resolve(const boost_code& ec, asio::iterator iterator,
    connect_handler handler)
{
    using namespace boost::asio;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_shared();

    if (stopped())
    {
        mutex_.unlock_shared();
        //---------------------------------------------------------------------
        dispatch_.concurrent(handler, error::service_stopped, nullptr);
        return;
    }

    if (ec)
    {
        mutex_.unlock_shared();
        //---------------------------------------------------------------------
        dispatch_.concurrent(handler, error::resolve_failed, nullptr);
        return;
    }

    const auto socket = std::make_shared<bc::socket>(pool_);
    timer_ = std::make_shared<deadline>(pool_, settings_.connect_timeout());

    // Manage the timer-connect race, returning upon first completion.
    const auto join_handler = synchronize(handler, 1, NAME,
        synchronizer_terminate::on_error);

    // timer.async_wait will not invoke the handler within this function.
    timer_->start(
        std::bind(&connector<MessageSubscriber>::handle_timer,
            this->shared_from_this(), _1, socket, join_handler));

    // async_connect will not invoke the handler within this function.
    // The bound delegate ensures handler completion before loss of scope.
    async_connect(socket->get(), iterator,
        std::bind(&connector<MessageSubscriber>::handle_connect,
            this->shared_from_this(), _1, _2, socket, join_handler));

    mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////
}

// private:
template<class MessageSubscriber>
void connector<MessageSubscriber>::handle_connect(const boost_code& ec, asio::iterator,
    socket::ptr socket, connect_handler handler)
{
    if (ec)
    {
        handler(error::boost_to_error_code(ec), nullptr);
        return;
    }

    // Ensure that channel is not passed as an r-value.
    const auto created = std::make_shared<channel<MessageSubscriber>>(pool_, socket, settings_);
    handler(error::success, created);
}

// private:
template<class MessageSubscriber>
void connector<MessageSubscriber>::handle_timer(const code& ec, socket::ptr socket,
    connect_handler handler)
{
    handler(ec ? ec : error::channel_timeout, nullptr);
}

template class connector<message_subscriber>;

} // namespace network
} // namespace libbitcoin
