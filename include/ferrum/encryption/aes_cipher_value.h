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

#ifndef __ferrum__aes_cipher_value__
#define __ferrum__aes_cipher_value__

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <limits>
#include <random>

#include <openssl/evp.h>

#include "cipher_value.h"

namespace fe {
    template <class T>
    class aes_cipher;

    /**
     *  The Alias of cipher_value using the aes_128_ecb algorithm.
     *  OpenSSL is required for this class.
     *
     *  ~~~~~~~~~~
     *  fe::aes_cipher_value<int> encrypted_value;
     *
     *  // encrypt.
     *  encrypted_value = 12345;
     *
     *  // decrypt
     *  int unencrypted_value = encrypted_value;
     *  ~~~~~~~~~~
     *
     *  @tparam T The type of the unencrypted value.
     */
    template <class T>
    using aes_cipher_value = cipher_value<T, aes_cipher<T>>;

    class aes_cipher_value_random_engine_holder {
    private:
        aes_cipher_value_random_engine_holder() = delete;

        static std::random_device::result_type get_seed() {
            std::random_device gen;
            return gen();
        }

        static std::mt19937 &get() {
            static std::mt19937 engine(get_seed());
            return engine;
        }

        template <class T>
        friend class aes_cipher;
    };

    template <class T>
    class aes_cipher {
    public:
        using value_type = T;
        using encrypted_type = std::array<unsigned char, 16>;

        /**
         *  Default constructor.
         */
        aes_cipher() {
            init();
        }

        /**
         *  Copy constructor.
         */
        aes_cipher(const aes_cipher &other) {
            init();
        }

        /**
         *  Move constructor.
         */
        aes_cipher(aes_cipher &&other) noexcept = default;

        /**
         *  Copy assignment operator.
         */
        aes_cipher &operator=(const aes_cipher &other) {
            init();
            return *this;
        }

        /**
         *  Move assignment operator.
         */
        aes_cipher &operator=(aes_cipher &&other) noexcept = default;

        encrypted_type encrypt(const value_type &unencrypted) const {
            EVP_CIPHER_CTX context;
            EVP_EncryptInit(&context, EVP_aes_128_ecb(), _key.data(), nullptr);

            converter in;
            in.value = unencrypted;

            encrypted_type out;

            int len;
            EVP_EncryptUpdate(&context, out.data(), &len, in.bytes.data(), static_cast<int>(in.bytes.size()));

            return out;
        }

        value_type decrypt(const encrypted_type &encrypted) const {
            EVP_CIPHER_CTX context;
            EVP_DecryptInit(&context, EVP_aes_128_ecb(), _key.data(), nullptr);

            converter out;

            int len;
            EVP_DecryptUpdate(&context, out.bytes.data(), &len, encrypted.data(), static_cast<int>(encrypted.size()));

            converter in;
            in.bytes = encrypted;

            return out.value;
        }

    private:
        void init() {
            std::uniform_int_distribution<unsigned char> dist(std::numeric_limits<unsigned char>::min(),
                                                              std::numeric_limits<unsigned char>::max());
            auto rand = std::bind(dist, std::ref(aes_cipher_value_random_engine_holder::get()));
            std::generate(_key.begin(), _key.end(), rand);
        }

        union converter {
            value_type value;
            encrypted_type bytes;
        };

        encrypted_type _key;
    };
}

#endif /* defined(__ferrum__aes_cipher_value__) */
