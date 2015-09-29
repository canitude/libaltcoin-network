/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_WALLET_BITCOIN_URI_HPP
#define LIBBITCOIN_WALLET_BITCOIN_URI_HPP

#include <cstdint>
#include <map>
#include <string>
#include <boost/optional.hpp>
#include <bitcoin/bitcoin/define.hpp>
#include <bitcoin/bitcoin/wallet/payment_address.hpp>
#include <bitcoin/bitcoin/wallet/stealth_address.hpp>
#include <bitcoin/bitcoin/wallet/uri_reader.hpp>

namespace libbitcoin {
namespace wallet {

/**
 * A bitcoin URI corresponding to BIP 21 and BIP 72.
 */
class BC_API bitcoin_uri
  : public uri_reader
{
public:
    bitcoin_uri();
    bitcoin_uri(const bitcoin_uri& other);
    bitcoin_uri(const std::string& uri, bool strict=true);

    /// Test for validity following a parse.
    operator const bool() const;

    /// Get the serialize URI representation.
    std::string encoded() const;

    /// Property getters.
    uint64_t amount() const;
    std::string label() const;
    std::string message() const;
    std::string r() const;
    std::string address() const;
    payment_address payment() const;
    stealth_address stealth() const;
    std::string parameter(const std::string& key) const;

    /// Property setters.
    void set_amount(uint64_t satoshis);
    void set_label(const std::string& label);
    void set_message(const std::string& message);
    void set_r(const std::string& r);
    void set_address(const payment_address& payment);
    void set_address(const stealth_address& stealth);

    /// uri_reader implementation.
    void set_strict(bool strict);
    bool set_scheme(const std::string& scheme);
    bool set_authority(const std::string& authority);
    bool set_path(const std::string& path);
    bool set_fragment(const std::string& fragment);
    bool set_parameter(const std::string& key, const std::string& value);

private:
    bool set_address(const std::string& address);
    bool set_amount(const std::string& satoshis);

    bool strict_;
    std::string scheme_;
    std::string address_;
    std::map<std::string, std::string> query_;
};

} // namespace wallet
} // namespace libbitcoin

#endif
