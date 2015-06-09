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

#ifndef __ferrum__copy_on_write_vector__
#define __ferrum__copy_on_write_vector__

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace fe {

    /**
     *  Represents a thread-safe sequence container for C++11.
     *  All iterators are unaffected by all operations in this class,
     *  because all mutative operations(such as push_bash) make a fresh copy of the underlying container.
     *
     *  ~~~~~~~~~~
     *  fe::copy_on_write_vector<int> vec = {2, 3, 5, 7, 11, 13};
     *
     *  // get a read-only snapshot.
     *  auto snapshot = vec.lock();
     *
     *  for (int elem : snapshot) {
     *      printf("elem = %d\n", elem);
     *
     *      // any mutative operations are possible during scanning of the iterator.
     *      vec.push_back(999);
     *  }
     *  ~~~~~~~~~~
     *
     *  @tparam T         The type of the elements
     *  @tparam Allocator An allocator that is used to acquire memory to store the elements
     */
    template <class T, class Allocator = std::allocator<T>>
    class copy_on_write_vector {
    public:
        /**
         *  The type of the elements.
         */
        using value_type = T;

        /**
         *  The type of the allocator.
         */
        using allocator_type = Allocator;

        /**
         *  The type of the underlying container.
         */
        using container_type = std::vector<value_type, allocator_type>;

        /**
         *  The unsigned integral type of the size of the underlying container.
         */
        using size_type = typename container_type::size_type;

        /**
         *  The signed integral type of difference of two iterators.
         */
        using difference_type = typename container_type::difference_type;

        /**
         *  The reference type of the elements.
         */
        using reference = typename container_type::reference;

        /**
         *  The const reference type of the elements.
         */
        using const_reference = typename container_type::const_reference;

        /**
         *  The pointer type of the elements.
         */
        using pointer = typename container_type::pointer;

        /**
         *  The const pointer type of the elements.
         */
        using const_pointer = typename container_type::const_pointer;

        class snapshot;

        /**
         *  Represents a constant random access iterator.
         */
        class const_random_access_iterator
            : public std::iterator<std::random_access_iterator_tag, value_type, difference_type, pointer, reference> {
        public:
            const_reference operator*() const {
                return *_itr;
            }

            const_reference operator->() const {
                return _itr.operator->();
            }

            const_reference operator[](size_type n) const {
                return _itr[n];
            }

            const_random_access_iterator &operator++() {
                ++_itr;
                return *this;
            }

            const_random_access_iterator operator++(int) {
                auto itr = _itr;
                ++(*this);
                return const_random_access_iterator(_shared_container, itr);
            }

            const_random_access_iterator &operator+=(difference_type distance) {
                _itr += distance;
                return *this;
            }

            const_random_access_iterator &operator+(difference_type distance) {
                auto itr = _itr + distance;
                return const_random_access_iterator(_shared_container, itr);
            }

            const_random_access_iterator &operator--() {
                --_itr;
                return *this;
            }

            const_random_access_iterator operator--(int) {
                auto itr = _itr;
                --(*this);
                return const_iterator(_shared_container, itr);
            }

            const_random_access_iterator &operator-=(difference_type distance) {
                _itr -= distance;
                return *this;
            }

            const_random_access_iterator &operator-(difference_type distance) {
                auto itr = _itr - distance;
                return const_iterator(_shared_container, itr);
            }

            friend bool operator==(const const_random_access_iterator &lhs, const const_random_access_iterator &rhs) {
                return lhs._itr == rhs._itr;
            }

            friend bool operator!=(const const_random_access_iterator &lhs, const const_random_access_iterator &rhs) {
                return lhs._itr != rhs._itr;
            }

            friend bool operator<(const const_random_access_iterator &lhs, const const_random_access_iterator &rhs) {
                return lhs._itr < rhs._itr;
            }

            friend bool operator<=(const const_random_access_iterator &lhs, const const_random_access_iterator &rhs) {
                return lhs._itr <= rhs._itr;
            }

            friend bool operator>(const const_random_access_iterator &lhs, const const_random_access_iterator &rhs) {
                return lhs._itr > rhs._itr;
            }

            friend bool operator>=(const const_random_access_iterator &lhs, const const_random_access_iterator &rhs) {
                return lhs._itr >= rhs._itr;
            }

            friend const_random_access_iterator operator+(const const_random_access_iterator &lhs,
                                                          difference_type rhs) {
                auto itr = lhs._itr + rhs;
                return const_random_access_iterator(lhs._shared_container, itr);
            }

            friend const_random_access_iterator operator+(difference_type lhs,
                                                          const const_random_access_iterator &rhs) {
                auto itr = lhs + rhs._itr;
                return const_random_access_iterator(rhs._shared_container, itr);
            }

            friend const_random_access_iterator operator-(const const_random_access_iterator &lhs,
                                                          difference_type rhs) {
                auto itr = lhs._itr - rhs;
                return const_random_access_iterator(lhs._shared_container, itr);
            }

            friend difference_type operator-(const const_random_access_iterator &lhs,
                                             const const_random_access_iterator &rhs) {
                return lhs._itr - rhs._itr;
            }

        private:
            const_random_access_iterator(std::shared_ptr<container_type> &&shared_container,
                                         typename container_type::const_iterator &&itr) noexcept
                : _shared_container(std::move(shared_container)),
                  _itr(std::move(itr)) {
            }

            static const_random_access_iterator begin(std::shared_ptr<container_type> shared_container) noexcept {
                return const_random_access_iterator(std::move(shared_container), std::move(shared_container->cbegin()));
            }

            static const_random_access_iterator end(std::shared_ptr<container_type> shared_container) noexcept {
                return const_random_access_iterator(std::move(shared_container), std::move(shared_container->cend()));
            }

            std::shared_ptr<container_type> _shared_container;
            typename container_type::const_iterator _itr;

            friend class snapshot;
        };

        /**
         *  The type of the constant random access iterator.
         */
        using iterator = const_random_access_iterator;

        /**
         *  The alias of iterator.
         */
        using const_iterator = iterator;

        /**
         *  The type of the constant reverse random access iterator.
         */
        using reverse_iterator = std::reverse_iterator<iterator>;

        /**
         *  The alias of reverse_iterator.
         */
        using const_reverse_iterator = reverse_iterator;

        /**
         *  Represents a snapshot of this container from a certain time.
         */
        class snapshot {
        public:
            /**
             *  Returns the associated allocator
             */
            allocator_type get_allocator() const {
                return _shared_container->get_allocator();
            }

            /**
             *  Returns the iterator to the beginning.
             */
            iterator begin() const noexcept {
                return iterator::begin(_shared_container);
            }

            /**
             *  Returns the iterator to the end.
             */
            iterator end() const noexcept {
                return iterator::end(_shared_container);
            }

            /**
             *  Returns the iterator to the beginning.
             */
            iterator cbegin() const noexcept {
                return iterator::begin(_shared_container);
            }

            /**
             *  Returns the iterator to the end.
             */
            iterator cend() const noexcept {
                return iterator::end(_shared_container);
            }

            /**
             *  Returns the reverse iterator to the beginning.
             */
            reverse_iterator rbegin() const noexcept {
                return reverse_iterator(iterator::end(_shared_container));
            }

            /**
             *  Returns the reverse iterator to the end.
             */
            reverse_iterator rend() const noexcept {
                return reverse_iterator(iterator::begin(_shared_container));
            }

            /**
             *  Returns the reverse iterator to the beginning.
             */
            reverse_iterator crbegin() const noexcept {
                return reverse_iterator(iterator::end(_shared_container));
            }

            /**
             *  Returns the reverse iterator to the end.
             */
            reverse_iterator crend() const noexcept {
                return reverse_iterator(iterator::begin(_shared_container));
            }

            /**
             *  Gets the n-th element with bounds checking.
             */
            const_reference at(size_type n) const {
                return _shared_container->at(n);
            }

            /**
             *  Gets the n-th element.
             */
            const_reference operator[](size_type n) const {
                return (*_shared_container)[n];
            }

            /**
             *  Gets the first element.
             */
            const_reference front() const {
                return _shared_container->front();
            }

            /**
             *  Gets the last element.
             */
            const_reference back() const {
                return _shared_container->back();
            }

            /**
             *  Gets the pointer to the underlying array.
             */
            const_pointer data() const noexcept {
                return _shared_container->data();
            }

            /**
             *  Checks whether the container is empty.
             */
            bool empty() const noexcept {
                return _shared_container->empty();
            }

            /**
             *  Returns the number of elements.
             */
            size_type size() const noexcept {
                return _shared_container->size();
            }

            /**
             *  Returns the max possible number of elements.
             */
            size_type max_size() const noexcept {
                return _shared_container->max_size();
            }

            /**
             *  Returns the number of elements that the container has currently allocated space for.
             */
            size_type capacity() const {
                return _shared_container->capacity();
            }

        private:
            snapshot(std::shared_ptr<container_type> shared_container) noexcept
                : _shared_container(std::move(shared_container)) {
            }

            std::shared_ptr<container_type> _shared_container;

            friend class copy_on_write_vector;
        };

        /**
         *  Default constructor. Constructs an empty container.
         */
        copy_on_write_vector() : copy_on_write_vector(allocator_type()) {
        }

        /**
         *  Constructs an empty container.
         *
         *  @param allocator The user supplied allocator.
         */
        explicit copy_on_write_vector(const allocator_type &allocator)
            : _shared_container(std::make_shared<container_type>(allocator)) {
        }

        /**
         *  Constructs the container with the copies of elements with the given value.
         *
         *  @param count     The number of elements.
         *  @param value     The value to initialize elements with.
         *  @param allocator The user supplied allocator.
         */
        copy_on_write_vector(size_type count, const_reference value, const allocator_type &allocator = allocator_type())
            : _shared_container(std::make_shared<container_type>(count, value, allocator)) {
        }

        /**
         *  Constructs the container with the default-inserted instances of value_type.
         *
         *  @param count The number of elements.
         */
        explicit copy_on_write_vector(size_type count) : _shared_container(std::make_shared<container_type>(count)) {
        }

        /**
         *  Constructs the container with the default-inserted instances of value_type.
         *
         *  @param count     The number of elements.
         *  @param allocator The user supplied allocator.
         */
        copy_on_write_vector(size_type count, const allocator_type &allocator)
            : _shared_container(std::make_shared<container_type>(count, allocator)) {
        }

        /**
         *  Constructs the container with the contents of the range [first, last).
         *
         *  @param first     The iterator that appoints the top of the range.
         *  @param last      The iterator that appoints the last next of the range.
         *  @param allocator The user supplied allocator.
         */
        template <class InputIterator>
        copy_on_write_vector(InputIterator first, InputIterator last,
                             const allocator_type &allocator = allocator_type())
            : _shared_container(std::make_shared<container_type>(first, last, allocator)) {
        }

        /**
         *  Copy constructor.
         *
         *  @param other Another container.
         */
        copy_on_write_vector(const copy_on_write_vector &other)
            : _shared_container(std::make_shared<container_type>(*(other.lock()._shared_container))) {
        }

        /**
         *  Copy constructor.
         *
         *  @param other     Another container.
         *  @param allocator The user supplied allocator.
         */
        copy_on_write_vector(const copy_on_write_vector &other, const allocator_type &allocator)
            : _shared_container(std::make_shared<container_type>(*(other.lock()._shared_container), allocator)) {
        }

        /**
         *  Move constructor.
         *
         *  @param other Another container.
         */
        copy_on_write_vector(copy_on_write_vector &&other)
            : _shared_container(std::make_shared<container_type>(std::move(*(other.lock()._shared_container)))) {
        }

        /**
         *  Move constructor.
         *
         *  @param other     Another container.
         *  @param allocator The user supplied allocator.
         */
        copy_on_write_vector(copy_on_write_vector &&other, const allocator_type &allocator)
            : _shared_container(
                  std::make_shared<container_type>(std::move(*(other.lock()._shared_container)), allocator)) {
        }

        /**
         *  Constructs the container with the copy of the elements of the other.
         *
         *  @param other Another container.
         */
        copy_on_write_vector(const container_type &other) : _shared_container(std::make_shared<container_type>(other)) {
        }

        /**
         *  Constructs the container with the copy of the elements of the other.
         *
         *  @param other     Another container.
         *  @param allocator The user supplied allocator.
         */
        copy_on_write_vector(const container_type &other, const allocator_type &allocator)
            : _shared_container(std::make_shared<container_type>(other, allocator)) {
        }

        /**
         *  Constructs the container with the elements of the other using move semantics.
         *
         *  @param other Another container.
         */
        copy_on_write_vector(container_type &&other)
            : _shared_container(std::make_shared<container_type>(std::move(other))) {
        }

        /**
         *  Constructs the container with the elements of the other using move semantics.
         *
         *  @param other     Another container.
         *  @param allocator The user supplied allocator.
         */
        copy_on_write_vector(container_type &&other, const allocator_type &allocator)
            : _shared_container(std::make_shared<container_type>(std::move(other), allocator)) {
        }

        /**
         *  Constructs the container with the contents of the initializer list.
         *
         *  @param initializer_list The initializer list to initialize the elements of the container with.
         *  @param allocator        The user supplied allocator.
         */
        copy_on_write_vector(std::initializer_list<value_type> initializer_list,
                             const allocator_type &allocator = allocator_type())
            : _shared_container(std::make_shared<container_type>(std::move(initializer_list), allocator)) {
        }

        /**
         *  Copy assignment operator.
         */
        copy_on_write_vector &operator=(const copy_on_write_vector &other) {
            _shared_container = std::make_shared<container_type>(*(other.lock()._shared_container));
            return *this;
        }

        /**
         *  Move assignment operator.
         */
        copy_on_write_vector &operator=(copy_on_write_vector &&other) {
            _shared_container = std::make_shared<container_type>(std::move(*(other.lock()._shared_container)));
            return *this;
        }

        /**
         *  Replaces the contents of the container.
         *
         *  @param other Another container to use as data source.
         */
        copy_on_write_vector &operator=(const container_type &other) {
            _shared_container = std::make_shared<container_type>(other);
            return *this;
        }

        /**
         *  Replaces the contents of the container using move semantics.
         *
         *  @param other Another container to use as data source.
         */
        copy_on_write_vector &operator=(container_type &&other) {
            _shared_container = std::make_shared<container_type>(std::move(other));
            return *this;
        }

        /**
         *  Replaces the contents of the container.
         *
         *  @param initializer_list The initializer list to use as data source.
         */
        copy_on_write_vector &operator=(std::initializer_list<value_type> initializer_list) {
            _shared_container = std::make_shared<container_type>(std::move(initializer_list));
            return *this;
        }

        /**
         *  Replaces the contents with the copies of the given value.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param count The number of elements.
         *  @param value The value to initialize elements with.
         */
        void assign(size_type count, const_reference value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->assign(count, value);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Replaces the contents with copies of those in the range [first, last).
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param first The iterator that appoints the top of the range.
         *  @param last  The iterator that appoints the last next of the range.
         */
        template <class InputIterator>
        void assign(InputIterator first, InputIterator last) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->assign(first, last);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Replaces the contents with the elements from the initializer list.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param initializer_list The initializer list to copy the values from.
         */
        void assign(std::initializer_list<value_type> initializer_list) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->assign(std::move(initializer_list));
            _shared_container = std::move(copied_container);
        }

        /**
         *  Returns the associated allocator.
         */
        allocator_type get_allocator() const {
            return lock().get_allocator();
        }

        /**
         *  Gets the n-th element with bounds checking.
         */
        const_reference at(size_type n) const {
            return lock().at(n);
        }

        /**
         *  Gets the n-th element.
         */
        const_reference operator[](size_type n) const {
            return lock()[n];
        }

        /**
         *  Gets the first element.
         */
        const_reference front() const {
            return lock().front();
        }

        /**
         *  Gets the last element.
         */
        const_reference back() const {
            return lock().back();
        }

        /**
         *  Gets the snapshot of the current container.
         */
        snapshot lock() const noexcept {
            return snapshot(_shared_container);
        }

        /**
         *  Checks whether the container is empty.
         */
        bool empty() const noexcept {
            return lock().empty();
        }

        /**
         *  Returns the number of elements.
         */
        size_type size() const noexcept {
            return lock().size();
        }

        /**
         *  Returns the max possible number of elements.
         */
        size_type max_size() const noexcept {
            return lock().max_size();
        }

        /**
         *  Reserves storage.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param new_cap The new capacity of the container.
         */
        void reserve(size_type new_cap) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->reserve(new_cap);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Returns the number of elements that the container has currently allocated space for.
         */
        size_type capacity() const {
            return lock().capacity();
        }

        /**
         *  Requests the removal of unused capacity.
         *  This operation makes a fresh copy of the underlying container.
         */
        void shrink_to_fit() {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->shrink_to_fit();
            _shared_container = std::move(copied_container);
        }

        /**
         *  Clears the contents.
         *  This operation makes a fresh copy of the underlying container.
         */
        void clear() {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(
                std::allocator_traits<allocator_type>::select_on_container_copy_construction(get_allocator()));
            copied_container->reserve(_shared_container->capacity());
            _shared_container = std::move(copied_container);
        }

        /**
         *  Inserts value before the given index.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param index The index to insert.
         *  @param value The value to insert.
         */
        void insert_at(size_type index, const_reference value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->insert(copied_container->begin() + index, value);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Inserts value before the given index.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param index The index to insert.
         *  @param value The value to insert.
         */
        void insert_at(size_type index, value_type &&value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->insert(copied_container->begin() + index, std::move(value));
            _shared_container = std::move(copied_container);
        }

        /**
         *  Inserts the copies of the value before the given index.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param index The index to insert.
         *  @param count The number of elements to insert.
         *  @param value The value to insert.
         */
        void insert_at(size_type index, size_type count, const_reference value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->insert(copied_container->begin() + index, count, value);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Inserts elements from range [first, last) before the given index.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param index The index to insert.
         *  @param first The iterator that appoints the top of the range.
         *  @param last  The iterator that appoints the last next of the range.
         */
        template <class InputIterator>
        void insert_at(size_type index, InputIterator first, InputIterator last) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->insert(copied_container->begin() + index, first, last);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Inserts elements from initializer list before the given index.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param index            The index to insert.
         *  @param initializer_list The initializer list to insert.
         */
        void insert_at(size_type index, std::initializer_list<value_type> initializer_list) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->insert(copied_container->begin() + index, std::move(initializer_list));
            _shared_container = std::move(copied_container);
        }

        /**
         *  Inserts a new element into the container directly before the given index.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param index The index to emplace.
         *  @param args  The arguments to forward to the constructor of the element.
         */
        template <class... ArgTypes>
        void emplace_at(size_type index, ArgTypes &&... args) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->emplace(copied_container->begin() + index, std::forward<ArgTypes>(args)...);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Replaces the first element that compare equal to old_value with a copy of new_value.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param old_value The value to be replaced.
         *  @param new_value The value to replace.
         *
         *  @return true if the element was replaced.
         */
        bool replace(const_reference old_value, const_reference new_value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = std::find(_shared_container->begin(), _shared_container->end(), old_value);
            if (it == _shared_container->end()) {
                return false;
            } else {
                auto copied_container = std::make_shared<container_type>(*_shared_container);
                (*copied_container)[it - _shared_container->begin()] = new_value;
                _shared_container = std::move(copied_container);
                return true;
            }
        }

        /**
         *  Replaces the first element that compare equal to old_value with new_value.
         *  If the element was not replaced, the given value is not moved.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param old_value The value to be replaced.
         *  @param new_value The value to be moved into the replacement element.
         *
         *  @return true if the element was replaced.
         */
        bool replace(const_reference old_value, value_type &&new_value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = std::find(_shared_container->begin(), _shared_container->end(), old_value);
            if (it == _shared_container->end()) {
                return false;
            } else {
                auto copied_container = std::make_shared<container_type>(*_shared_container);
                (*copied_container)[it - _shared_container->begin()] = std::move(new_value);
                _shared_container = std::move(copied_container);
                return true;
            }
        }

        /**
         *  Replaces all elements that compare equal to old_value with a copy of new_value.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param old_value The value to be replaced.
         *  @param new_value The value to replace.
         *
         *  @return The number of elements replaced.
         */
        size_type replace_all(const_reference old_value, const_reference new_value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = _shared_container;
            auto first = _shared_container->begin();
            auto last = _shared_container->end();
            size_type result = 0;
            while (first != last) {
                if (*first == old_value) {
                    if (result++ == 0) {
                        copied_container = std::make_shared<container_type>(*copied_container);
                        first = copied_container->begin() + (first - _shared_container->begin());
                        last = copied_container->end();
                    }
                    (*copied_container)[first - copied_container->begin()] = new_value;
                }
                ++first;
            }
            _shared_container = std::move(copied_container);
            return result;
        }

        /**
         *  Replaces the element at the given index with a copy of the given value.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param index The index to replace.
         *  @param value The value to replace.
         */
        void replace_at(size_type index, const_reference value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            (*copied_container)[index] = value;
            _shared_container = std::move(copied_container);
        }

        /**
         *  Replaces the element at the given index with the given value.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param index The index to replace.
         *  @param value The value to be moved into the replacement element.
         */
        void replace_at(size_type index, value_type &&value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            (*copied_container)[index] = std::move(value);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Replaces the first element for which the given predicate returns true with a copy of the given value.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param pred  The unary predicate which returns ​true if the element value should be replaced.
         *  @param value The value to replace.
         *
         *  @return true if the element was replaced.
         */
        template <class UnaryPredicate>
        bool replace_if(UnaryPredicate pred, const_reference value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = std::find_if(_shared_container->begin(), _shared_container->end(), std::move(pred));
            if (it == _shared_container->end()) {
                return false;
            } else {
                auto copied_container = std::make_shared<container_type>(*_shared_container);
                (*copied_container)[it - _shared_container->begin()] = value;
                _shared_container = std::move(copied_container);
                return true;
            }
        }

        /**
         *  Replaces the first element for which the given predicate returns true with the given value.
         *  If the element was not replaced, the given value is not moved.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param pred  The unary predicate which returns ​true if the element value should be replaced.
         *  @param value The value to be moved into the replacement element.
         *
         *  @return true if the element was replaced.
         */
        template <class UnaryPredicate>
        bool replace_if(UnaryPredicate pred, value_type &&value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = std::find_if(_shared_container->begin(), _shared_container->end(), std::move(pred));
            if (it == _shared_container->end()) {
                return false;
            } else {
                auto copied_container = std::make_shared<container_type>(*_shared_container);
                (*copied_container)[it - _shared_container->begin()] = std::move(value);
                _shared_container = std::move(copied_container);
                return true;
            }
        }

        /**
         *  Replaces all elements for which the given predicate returns true with copies of the given value.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param pred  The unary predicate which returns ​true if the element value should be replaced.
         *  @param value The value to replace.
         *
         *  @return The number of elements replaced.
         */
        template <class UnaryPredicate>
        size_type replace_all_if(UnaryPredicate pred, const_reference value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = _shared_container;
            auto first = _shared_container->begin();
            auto last = _shared_container->end();
            size_type result = 0;
            while (first != last) {
                if (pred(*first)) {
                    if (result++ == 0) {
                        copied_container = std::make_shared<container_type>(*copied_container);
                        first = copied_container->begin() + (first - _shared_container->begin());
                        last = copied_container->end();
                    }
                    (*copied_container)[first - copied_container->begin()] = value;
                }
                ++first;
            }
            _shared_container = std::move(copied_container);
            return result;
        }

        /**
         *  Erases the first element that compare equal to the given value.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param value The value to be erased.
         *
         *  @return true if the element was erased.
         */
        bool erase(const_reference value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = std::find(_shared_container->begin(), _shared_container->end(), value);
            if (it == _shared_container->end()) {
                return false;
            } else {
                auto copied_container = std::make_shared<container_type>(*_shared_container);
                copied_container->erase(copied_container->begin() + (it - _shared_container->begin()));
                _shared_container = std::move(copied_container);
                return true;
            }
        }

        /**
         *  Erases all elements that compare equal to the given value.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param value The value to be erased.
         *
         *  @return The number of elements erased.
         */
        size_type erase_all(const_reference value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            auto it = std::remove(copied_container->begin(), copied_container->end(), value);
            auto result = copied_container->end() - it;
            copied_container->erase(it, copied_container->end());
            _shared_container = std::move(copied_container);
            return result;
        }

        /**
         *  Erases the element at the given index.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param index The index to erase.
         */
        void erase_at(size_type index) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->erase(copied_container->begin() + index);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Erases the elements in the range [first, last).
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param first The index of the top of the range.
         *  @param last  The index of the last next of the range.
         */
        void erase_range(size_type first, size_type last) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->erase(copied_container->begin() + first, copied_container->begin() + last);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Erases the first element for which the given predicate returns true.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param pred The unary predicate which returns ​true if the element value should be erased.
         *
         *  @return true if the element was erased.
         */
        template <class UnaryPredicate>
        bool erase_if(UnaryPredicate pred) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = std::find_if(_shared_container->begin(), _shared_container->end(), std::move(pred));
            if (it == _shared_container->end()) {
                return false;
            } else {
                auto copied_container = std::make_shared<container_type>(*_shared_container);
                copied_container->erase(copied_container->begin() + (it - _shared_container->begin()));
                _shared_container = std::move(copied_container);
                return true;
            }
        }

        /**
         *  Erases all elements for which the given predicate returns true.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param pred The unary predicate which returns ​true if the element value should be erased.
         *
         *  @return The number of elements erased.
         */
        template <class UnaryPredicate>
        size_type erase_all_if(UnaryPredicate pred) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            auto it = std::remove_if(copied_container->begin(), copied_container->end(), std::move(pred));
            auto result = copied_container->end() - it;
            copied_container->erase(it, copied_container->end());
            _shared_container = std::move(copied_container);
            return result;
        }

        /**
         *  Adds the element to the end.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param value The new element is initialized as a copy of this value.
         */
        void push_back(const_reference value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->push_back(value);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Adds the element to the end.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param value The value to be moved into the new element.
         */
        void push_back(value_type &&value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->push_back(std::move(value));
            _shared_container = std::move(copied_container);
        }

        /**
         *  Adds elements from range [first, last) to the end.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param first The iterator that appoints the top of the range.
         *  @param last  The iterator that appoints the last next of the range.
         */
        template <class InputIterator>
        void push_back(InputIterator first, InputIterator last) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            while (first != last) {
                copied_container->push_back(*first);
                ++first;
            }
            _shared_container = std::move(copied_container);
        }

        /**
         *  Adds the element to the end if not present.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param value The new element is initialized as a copy of this value.
         *
         *  @return true if the element was added.
         */
        bool push_back_if_absent(const_reference value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = std::find(_shared_container->begin(), _shared_container->end(), value);
            if (it != _shared_container->end()) {
                return false;
            } else {
                auto copied_container = std::make_shared<container_type>(*_shared_container);
                copied_container->push_back(value);
                _shared_container = std::move(copied_container);
                return true;
            }
        }

        /**
         *  Adds the element to the end if not present.
         *  If the element was not added, the given value is not moved.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param value The value to be moved into the new element.
         *
         *  @return true if the element was added.
         */
        bool push_back_if_absent(value_type &&value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = std::find(_shared_container->begin(), _shared_container->end(), value);
            if (it != _shared_container->end()) {
                return false;
            } else {
                auto copied_container = std::make_shared<container_type>(*_shared_container);
                copied_container->push_back(std::move(value));
                _shared_container = std::move(copied_container);
                return true;
            }
        }

        /**
         *  Adds elements that are not already contained in this container from range [first, last) to the end.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param first The iterator that appoints the top of the range.
         *  @param last  The iterator that appoints the last next of the range.
         *
         *  @return The number of elements added.
         */
        template <class InputIterator>
        size_type push_back_if_absent(InputIterator first, InputIterator last) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = _shared_container;
            size_type result = 0;
            while (first != last) {
                auto it = std::find(copied_container->begin(), copied_container->end(), *first);
                if (it == copied_container->end()) {
                    if (result++ == 0) {
                        copied_container = std::make_shared<container_type>(*copied_container);
                    }
                    copied_container->push_back(*first);
                }
                ++first;
            }
            _shared_container = std::move(copied_container);
            return result;
        }

        /**
         *  Adds a new element into the container directly to the end.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param args The arguments to forward to the constructor of the element.
         */
        template <class... ArgTypes>
        void emplace_back(ArgTypes &&... args) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->emplace_back(std::forward<ArgTypes>(args)...);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Removes the last element.
         *  This operation makes a fresh copy of the underlying container.
         */
        void pop_back() {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->pop_back();
            _shared_container = std::move(copied_container);
        }

        /**
         *  Resizes the container.
         *  If the current size is less than the given count, the default-inserted instances of value_type are appended.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param count The new size of the container.
         */
        void resize(size_type count) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->resize(count);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Resizes the container.
         *  If the current size is less than the given count, the copies of the given value are appended.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param count The new size of the container.
         *  @param value The value to initialize the new elements with.
         */
        void resize(size_type count, const_reference &value) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->resize(count, value);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Swaps the contents.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param other The container to exchange the contents with.
         */
        void swap(copy_on_write_vector &other) {
            std::lock_guard<std::mutex> lock(_mutex);
            std::lock_guard<std::mutex> other_lock(other._mutex);

            auto copied_container = std::make_shared<container_type>(*_shared_container);
            auto other_copied_container = std::make_shared<container_type>(*(other._shared_container));

            copied_container->swap(*other_copied_container);

            _shared_container = std::move(copied_container);
            other._shared_container = std::move(other_copied_container);
        }

        /**
         *  Swaps the contents.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param other The container to exchange the contents with.
         */
        void swap(container_type &other) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            copied_container->swap(other);
            _shared_container = std::move(copied_container);
        }

        /**
         *  Sorts the elements into ascending order.
         *  This operation makes a fresh copy of the underlying container.
         */
        void sort() {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            std::sort(copied_container->begin(), copied_container->end());
            _shared_container = std::move(copied_container);
        }

        /**
         *  Sorts the elements into ascending order.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param comp The binary function that returns ​true if the first argument is less than the second.
         */
        template <class Compare>
        void sort(Compare comp) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            std::sort(copied_container->begin(), copied_container->end(), std::move(comp));
            _shared_container = std::move(copied_container);
        }

        /**
         *  Sorts the elements into ascending order, while preserving order between equal elements.
         *  This operation makes a fresh copy of the underlying container.
         */
        void stable_sort() {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            std::stable_sort(copied_container->begin(), copied_container->end());
            _shared_container = std::move(copied_container);
        }

        /**
         *  Sorts the elements into ascending order, while preserving order between equal elements.
         *  This operation makes a fresh copy of the underlying container.
         *
         *  @param comp The binary function that returns ​true if the first argument is less than the second.
         */
        template <class Compare>
        void stable_sort(Compare comp) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto copied_container = std::make_shared<container_type>(*_shared_container);
            std::stable_sort(copied_container->begin(), copied_container->end(), std::move(comp));
            _shared_container = std::move(copied_container);
        }

        /**
         *  Compares the contents of two containers.
         */
        friend bool operator==(const copy_on_write_vector &lhs, const copy_on_write_vector &rhs) {
            return *(lhs._shared_container) == *(rhs._shared_container);
        }

        /**
         *  Compares the contents of two containers.
         */
        friend bool operator!=(const copy_on_write_vector &lhs, const copy_on_write_vector &rhs) {
            return *(lhs._shared_container) != *(rhs._shared_container);
        }

        /**
         *  Compares the contents of two containers.
         */
        friend bool operator<(const copy_on_write_vector &lhs, const copy_on_write_vector &rhs) {
            return *(lhs._shared_container) < *(rhs._shared_container);
        }

        /**
         *  Compares the contents of two containers.
         */
        friend bool operator<=(const copy_on_write_vector &lhs, const copy_on_write_vector &rhs) {
            return *(lhs._shared_container) <= *(rhs._shared_container);
        }

        /**
         *  Compares the contents of two containers.
         */
        friend bool operator>(const copy_on_write_vector &lhs, const copy_on_write_vector &rhs) {
            return *(lhs._shared_container) > *(rhs._shared_container);
        }

        /**
         *  Compares the contents of two containers.
         */
        friend bool operator>=(const copy_on_write_vector &lhs, const copy_on_write_vector &rhs) {
            return *(lhs._shared_container) >= *(rhs._shared_container);
        }

    private:
        std::shared_ptr<container_type> _shared_container;
        std::mutex _mutex;
    };
}

#endif /* defined(__ferrum__copy_on_write_vector__) */
