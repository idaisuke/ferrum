// The MIT License (MIT)
//
// Copyright (c) 2015 Daisuke Itabashi (https://github.com/idaisuke/ferrum)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __ferrum__cipher_value__
#define __ferrum__cipher_value__

#include <type_traits>
#include <utility>

namespace fe {

    /**
     *  Provides a value that is encrypted that can be used in the same way as the primitive type.
     *
     *  ~~~~~~~~~~
     *  fe::cipher_value<int, fe::xor_cipher<int>> encrypted_value;
     *
     *  // encrypt.
     *  encrypted_value = 12345;
     *
     *  // arithmetic operations are possible.
     *  auto result = encrypted_value * 3;
     *
     *  // decrypt
     *  int unencrypted_value = encrypted_value;
     *  ~~~~~~~~~~
     *
     *  @tparam T      The type of the unencrypted value.
     *  @tparam Cipher A cryptographic cipher for encryption and decryption.
     *
     *  @see fe::xor_cipher_value, fe::aes_cipher_value
     */
    template <class T, class Cipher>
    class cipher_value {
    public:
        /**
         *  The type of the unencrypted value.
         */
        using value_type = T;

        /**
         *  The type of the cipher.
         */
        using cipher_type = Cipher;

        /**
         *  The type of the encrypted value.
         */
        using encrypted_type = typename cipher_type::encrypted_type;

        /**
         *  Default constructor.
         */
        cipher_value() : _cipher(cipher_type()) {
        }

        explicit cipher_value(const cipher_type &cipher) : _cipher(cipher) {
        }

        cipher_value(const value_type &value, const cipher_type &cipher = cipher_type())
            : _cipher(cipher), _encrypted(_cipher.encrypt(value)) {
        }

        cipher_value(value_type &&value, const cipher_type &cipher = cipher_type())
            : _cipher(cipher), _encrypted(_cipher.encrypt(std::move(value))) {
        }

        /**
         *  Copy constructor.
         */
        cipher_value(const cipher_value &other)
            : _cipher(other._cipher), _encrypted(_cipher.encrypt(static_cast<value_type>(other))) {
        }

        cipher_value(const cipher_value &other, const cipher_type &cipher)
            : _cipher(cipher), _encrypted(_cipher.encrypt(static_cast<value_type>(other))) {
        }

        /**
         *  Move constructor.
         */
        cipher_value(cipher_value &&other) noexcept(std::is_nothrow_move_constructible<cipher_type>::value
                                                        &&std::is_nothrow_move_constructible<encrypted_type>::value)
            : _cipher(std::move(other._cipher)), _encrypted(std::move(other._encrypted)) {
        }

        cipher_value(cipher_value &&other, const cipher_type &cipher)
            : _cipher(cipher), _encrypted(_cipher.encrypt(static_cast<value_type>(std::move(other)))) {
        }

        /**
         *  Copy assignment operator.
         */
        cipher_value &operator=(const cipher_value &other) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(other));
            return *this;
        }

        /**
         *  Move assignment operator.
         */
        cipher_value &operator=(cipher_value &&other) noexcept(std::is_nothrow_move_assignable<
            cipher_type>::value &&std::is_nothrow_move_assignable<encrypted_type>::value) {
            _cipher = std::move(other._cipher);
            _encrypted = std::move(other._encrypted);
            return *this;
        }

        cipher_value &operator=(const value_type &unencrypted) {
            _encrypted = _cipher.encrypt(unencrypted);
            return *this;
        }

        cipher_value &operator=(value_type &&unencrypted) {
            _encrypted = _cipher.encrypt(std::move(unencrypted));
            return *this;
        }

        cipher_value &operator+=(const cipher_value &other) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) + static_cast<value_type>(other));
            return *this;
        }

        cipher_value &operator+=(cipher_value &&other) {
            _encrypted =
                _cipher.encrypt(static_cast<value_type>(std::move(*this)) + static_cast<value_type>(std::move(other)));
            return *this;
        }

        cipher_value &operator+=(const value_type &unencrypted) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) + unencrypted);
            return *this;
        }

        cipher_value &operator+=(value_type &&unencrypted) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) + std::move(unencrypted));
            return *this;
        }

        cipher_value &operator-=(const cipher_value &other) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) - static_cast<value_type>(other));
            return *this;
        }

        cipher_value &operator-=(cipher_value &&other) {
            _encrypted =
                _cipher.encrypt(static_cast<value_type>(std::move(*this)) - static_cast<value_type>(std::move(other)));
            return *this;
        }

        cipher_value &operator-=(const value_type &unencrypted) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) - unencrypted);
            return *this;
        }

        cipher_value &operator-=(value_type &&unencrypted) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) - std::move(unencrypted));
            return *this;
        }

        cipher_value &operator*=(const cipher_value &other) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) * static_cast<value_type>(other));
            return *this;
        }

        cipher_value &operator*=(cipher_value &&other) {
            _encrypted =
                _cipher.encrypt(static_cast<value_type>(std::move(*this)) * static_cast<value_type>(std::move(other)));
            return *this;
        }

        cipher_value &operator*=(const value_type &unencrypted) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) * unencrypted);
            return *this;
        }

        cipher_value &operator*=(value_type &&unencrypted) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) * std::move(unencrypted));
            return *this;
        }

        cipher_value &operator/=(const cipher_value &other) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) / static_cast<value_type>(other));
            return *this;
        }

        cipher_value &operator/=(cipher_value &&other) {
            _encrypted =
                _cipher.encrypt(static_cast<value_type>(std::move(*this)) / static_cast<value_type>(std::move(other)));
            return *this;
        }

        cipher_value &operator/=(const value_type &unencrypted) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) / unencrypted);
            return *this;
        }

        cipher_value &operator/=(value_type &&unencrypted) {
            _encrypted = _cipher.encrypt(static_cast<value_type>(std::move(*this)) / std::move(unencrypted));
            return *this;
        }

        operator value_type() const & {
            return _cipher.decrypt(_encrypted);
        }

        operator value_type() && {
            return _cipher.decrypt(std::move(_encrypted));
        }

    private:
        cipher_type _cipher;
        encrypted_type _encrypted;
    };

    template <class T, class Cipher>
    cipher_value<T, Cipher> operator+(const cipher_value<T, Cipher> &lhs) {
        return lhs;
    }

    template <class T, class Cipher>
    cipher_value<T, Cipher> operator-(const cipher_value<T, Cipher> &lhs) {
        return -1 * lhs;
    }

    template <class T, class Cipher, class ArgType>
    auto operator+(const cipher_value<T, Cipher> &lhs, ArgType &&rhs) ->
        typename std::enable_if<std::is_convertible<ArgType, T>::value, cipher_value<T, Cipher>>::type {
        return static_cast<T>(lhs) + static_cast<T>(std::forward<ArgType>(rhs));
    }

    template <class T, class Cipher, class ArgType>
    auto operator+(ArgType &&lhs, const cipher_value<T, Cipher> &rhs) ->
        typename std::enable_if<std::is_convertible<ArgType, T>::value, cipher_value<T, Cipher>>::type {
        return static_cast<T>(std::forward<ArgType>(lhs)) + static_cast<T>(rhs);
    }

    template <class T, class Cipher, class ArgType>
    auto operator-(const cipher_value<T, Cipher> &lhs, ArgType &&rhs) ->
        typename std::enable_if<std::is_convertible<ArgType, T>::value, cipher_value<T, Cipher>>::type {
        return static_cast<T>(lhs) - static_cast<T>(std::forward<ArgType>(rhs));
    }

    template <class T, class Cipher, class ArgType>
    auto operator-(ArgType &&lhs, const cipher_value<T, Cipher> &rhs) ->
        typename std::enable_if<std::is_convertible<ArgType, T>::value, cipher_value<T, Cipher>>::type {
        return static_cast<T>(std::forward<ArgType>(lhs)) - static_cast<T>(rhs);
    }

    template <class T, class Cipher, class ArgType>
    auto operator*(const cipher_value<T, Cipher> &lhs, ArgType &&rhs) ->
        typename std::enable_if<std::is_convertible<ArgType, T>::value, cipher_value<T, Cipher>>::type {
        return static_cast<T>(lhs) * static_cast<T>(std::forward<ArgType>(rhs));
    }

    template <class T, class Cipher, class ArgType>
    auto operator*(ArgType &&lhs, const cipher_value<T, Cipher> &rhs) ->
        typename std::enable_if<std::is_convertible<ArgType, T>::value, cipher_value<T, Cipher>>::type {
        return static_cast<T>(std::forward<ArgType>(lhs)) * static_cast<T>(rhs);
    }

    template <class T, class Cipher, class ArgType>
    auto operator/(const cipher_value<T, Cipher> &lhs, ArgType &&rhs) ->
        typename std::enable_if<std::is_convertible<ArgType, T>::value, cipher_value<T, Cipher>>::type {
        return static_cast<T>(lhs) / static_cast<T>(std::forward<ArgType>(rhs));
    }

    template <class T, class Cipher, class ArgType>
    auto operator/(ArgType &&lhs, const cipher_value<T, Cipher> &rhs) ->
        typename std::enable_if<std::is_convertible<ArgType, T>::value, cipher_value<T, Cipher>>::type {
        return static_cast<T>(std::forward<ArgType>(lhs)) / static_cast<T>(rhs);
    }

    template <class T, class Cipher>
    bool operator==(const cipher_value<T, Cipher> &lhs, const cipher_value<T, Cipher> &rhs) {
        return static_cast<T>(lhs) == static_cast<T>(rhs);
    }

    template <class T, class Cipher>
    bool operator==(const cipher_value<T, Cipher> &lhs, const T &rhs) {
        return static_cast<T>(lhs) == rhs;
    }

    template <class T, class Cipher>
    bool operator==(const T &lhs, const cipher_value<T, Cipher> &rhs) {
        return lhs == static_cast<T>(rhs);
    }

    template <class T, class Cipher>
    bool operator!=(const cipher_value<T, Cipher> &lhs, const cipher_value<T, Cipher> &rhs) {
        return static_cast<T>(lhs) != static_cast<T>(rhs);
    }

    template <class T, class Cipher>
    bool operator!=(const cipher_value<T, Cipher> &lhs, const T &rhs) {
        return static_cast<T>(lhs) != rhs;
    }

    template <class T, class Cipher>
    bool operator!=(const T &lhs, const cipher_value<T, Cipher> &rhs) {
        return lhs != static_cast<T>(rhs);
    }

    template <class T, class Cipher, class CharT, class Traits>
    std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os,
                                                  const cipher_value<T, Cipher> &value) {
        return os << static_cast<T>(value);
    }

    template <class T, class Cipher, class CharT, class Traits>
    std::basic_istream<CharT, Traits> &operator>>(std::basic_istream<CharT, Traits> &is,
                                                  cipher_value<T, Cipher> &value) {
        T unencrypted;
        is >> unencrypted;
        value = unencrypted;
        return is;
    }
}

#endif /* defined(__ferrum__cipher_value__) */
