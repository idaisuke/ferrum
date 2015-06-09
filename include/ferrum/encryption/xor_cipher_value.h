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

#ifndef __ferrum__xor_cipher_value__
#define __ferrum__xor_cipher_value__

#include <cstdint>
#include <limits>
#include <random>
#include <type_traits>

#include "cipher_value.h"

namespace fe {
    template <class T>
    class xor_cipher;

    /**
     *  The Alias of cipher_value using the xor algorithm.
     *
     *  ~~~~~~~~~~
     *  fe::xor_cipher_value<int> encrypted_value;
     *
     *  // encrypt
     *  encrypted_value = 12345;
     *
     *  // decrypt
     *  int unencrypted_value = encrypted_value;
     *  ~~~~~~~~~~
     *
     *  @tparam T The type of the unencrypted value.
     */
    template <class T>
    using xor_cipher_value = cipher_value<T, xor_cipher>;

    template <class T, class>
    class basic_int_xor_cipher;

    template <class T, class IntType, class>
    class basic_real_xor_cipher;

    class xor_cipher_value_random_engine_holder {
    private:
        xor_cipher_value_random_engine_holder() = delete;

        static std::random_device::result_type get_seed() {
            std::random_device gen;
            return gen();
        }

        static std::mt19937 &get() {
            static std::mt19937 engine(get_seed());
            return engine;
        }

        template <class T, class>
        friend class basic_int_xor_cipher;

        template <class T, class IntType, class>
        friend class basic_real_xor_cipher;
    };

    template <class T, class = typename std::enable_if<std::is_integral<T>::value, void>::type>
    class basic_int_xor_cipher {
    public:
        using value_type = T;
        using encrypted_type = value_type;

        /**
         *  Default constructor.
         */
        basic_int_xor_cipher() {
            init();
        }

        /**
         *  Copy constructor.
         */
        basic_int_xor_cipher(const basic_int_xor_cipher &other) {
            init();
        }

        /**
         *  Move constructor.
         */
        basic_int_xor_cipher(basic_int_xor_cipher &&other) noexcept = default;

        /**
         *  Copy assignment operator.
         */
        basic_int_xor_cipher &operator=(const basic_int_xor_cipher &other) {
            init();
            return *this;
        }

        /**
         *  Move assignment operator.
         */
        basic_int_xor_cipher &operator=(basic_int_xor_cipher &&other) noexcept = default;

        encrypted_type encrypt(const value_type &unencrypted) const {
            return (unencrypted ^ _key) + _salt;
        }

        value_type decrypt(const encrypted_type &encrypted) const {
            return (encrypted - _salt) ^ _key;
        }

    private:
        static constexpr encrypted_type LIMIT_MAX = std::numeric_limits<encrypted_type>::max();
        static constexpr encrypted_type LIMIT_MIN = std::numeric_limits<encrypted_type>::min();

        void init() {
            std::uniform_int_distribution<encrypted_type> dist(LIMIT_MIN, LIMIT_MAX);
            _key = dist(xor_cipher_value_random_engine_holder::get());
            _salt = dist(xor_cipher_value_random_engine_holder::get());
        }

        encrypted_type _key;
        encrypted_type _salt;
    };

    template <class T, class IntType, class = typename std::enable_if<std::is_floating_point<T>::value, void>::type>
    class basic_real_xor_cipher {
    public:
        using value_type = T;
        using encrypted_type = IntType;

        /**
         *  Default constructor.
         */
        basic_real_xor_cipher() {
            init();
        }

        /**
         *  Copy constructor.
         */
        basic_real_xor_cipher(const basic_real_xor_cipher &other) {
            init();
        }

        /**
         *  Move constructor.
         */
        basic_real_xor_cipher(basic_real_xor_cipher &&other) noexcept = default;

        /**
         *  Copy assignment operator.
         */
        basic_real_xor_cipher &operator=(const basic_real_xor_cipher &other) {
            init();
            return *this;
        }

        /**
         *  Move assignment operator.
         */
        basic_real_xor_cipher &operator=(basic_real_xor_cipher &&other) noexcept = default;

        encrypted_type encrypt(const value_type &unencrypted) const {
            converter converter;
            converter.real_value = unencrypted;
            converter.int_value = (converter.int_value ^ _key) + _salt;
            return converter.int_value;
        }

        value_type decrypt(const encrypted_type &encrypted) const {
            converter converter;
            converter.int_value = encrypted;
            converter.int_value = (converter.int_value - _salt) ^ _key;
            return converter.real_value;
        }

    private:
        static constexpr encrypted_type LIMIT_MAX = std::numeric_limits<encrypted_type>::max();
        static constexpr encrypted_type LIMIT_MIN = std::numeric_limits<encrypted_type>::min();

        void init() {
            std::uniform_int_distribution<encrypted_type> dist(LIMIT_MIN, LIMIT_MAX);
            _key = dist(xor_cipher_value_random_engine_holder::get());
            _salt = dist(xor_cipher_value_random_engine_holder::get());
        }

        union converter {
            value_type real_value;
            encrypted_type int_value;
        };

        encrypted_type _key;
        encrypted_type _salt;
    };

    template <class T>
    class xor_cipher : public basic_int_xor_cipher<T> {};

#if __SIZEOF_FLOAT__ < 2
    template <>
    class xor_cipher<float> : public basic_real_xor_cipher<float, std::int_fast8_t> {};
#elif __SIZEOF_FLOAT__ < 4
    template <>
    class xor_cipher<float> : public basic_real_xor_cipher<float, std::int_fast16_t> {};
#elif __SIZEOF_FLOAT__ < 8
    template <>
    class xor_cipher<float> : public basic_real_xor_cipher<float, std::int_fast32_t> {};
#else
    template <>
    class xor_cipher<float> : public basic_real_xor_cipher<float, std::int_fast64_t> {};
#endif

#if __SIZEOF_DOUBLE__ < 2
    template <>
    class xor_cipher<double> : public basic_real_xor_cipher<double, std::int_fast8_t> {};
#elif __SIZEOF_DOUBLE__ < 4
    template <>
    class xor_cipher<double> : public basic_real_xor_cipher<double, std::int_fast16_t> {};
#elif __SIZEOF_DOUBLE__ < 8
    template <>
    class xor_cipher<double> : public basic_real_xor_cipher<double, std::int_fast32_t> {};
#else
    template <>
    class xor_cipher<double> : public basic_real_xor_cipher<double, std::int_fast64_t> {};
#endif

#if __SIZEOF_LONG_DOUBLE__ < 2
    template <>
    class xor_cipher<long double> : public basic_real_xor_cipher<long double, std::int_fast8_t> {};
#elif __SIZEOF_LONG_DOUBLE__ < 4
    template <>
    class xor_cipher<long double> : public basic_real_xor_cipher<long double, std::int_fast16_t> {};
#elif __SIZEOF_LONG_DOUBLE__ < 8
    template <>
    class xor_cipher<long double> : public basic_real_xor_cipher<long double, std::int_fast32_t> {};
#else
    template <>
    class xor_cipher<long double> : public basic_real_xor_cipher<long double, std::int_fast64_t> {};
#endif
}

#endif /* defined(__ferrum__xor_cipher_value__) */
