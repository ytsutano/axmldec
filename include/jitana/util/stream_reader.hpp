/*
 * Copyright (c) 2015, 2016, 2017, Yutaka Tsutano
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef JITANA_STREAM_READER_HPP
#define JITANA_STREAM_READER_HPP

#include <cstdint>
#include <stdexcept>

namespace jitana {
    /// An utility class for extracting values with variable length types from a
    /// memory space.
    class stream_reader {
    public:
        /// Creates a stream_reader instance.
        stream_reader()
        {
            set_memory_range(nullptr, nullptr);
        }

        /// Creates a stream_reader instance.
        explicit stream_reader(const void* first, const void* last)
        {
            set_memory_range(first, last);
        }

        /// Sets the begin and end pointers.
        void set_memory_range(const void* first, const void* last)
        {
            begin_ptr_ = reinterpret_cast<const uint8_t*>(first);
            end_ptr_ = reinterpret_cast<const uint8_t*>(last);
            move_head(0);
        }

        /// Returns the begin pointer.
        const void* begin() const
        {
            return begin_ptr_;
        }

        /// Returns the end pointer.
        const void* end() const
        {
            return end_ptr_;
        }

        /// Returns the buffer size.
        size_t size() const
        {
            return end_ptr_ - begin_ptr_;
        }

        /// Moves the head.
        void move_head(size_t pos = 0) const
        {
            head_ptr_ = begin_ptr_ + pos;
            validate_head();
        }

        /// Moves the head forward.
        void move_head_forward(int off = 0) const
        {
            head_ptr_ += off;
            validate_head();
        }

        /// Returns the head index.
        size_t head() const
        {
            return head_ptr_ - begin_ptr_;
        }

        /// Returns the reference to the value of specified type from the head
        /// without moving the head.
        template <typename T>
        const T& peek() const
        {
            validate_head(sizeof(T));
            return *reinterpret_cast<const T*>(head_ptr_);
        }

        /// Returns the reference to the value of specified type from the head.
        ///
        /// The head moves forward to point the next value as a result.
        template <typename T>
        const T& get() const
        {
            const T& temp = peek<T>();
            head_ptr_ += sizeof(T);
            return temp;
        }

        /// Returns the uleb128 value from the current head.
        ///
        /// The head moves forward to point the next value as a result.
        uint32_t get_uleb128() const
        {
            uint32_t decoded = 0;

            for (int i = 0; i < 5; ++i) {
                // Read a new byte.
                const uint8_t byte_read = get<uint8_t>();

                decoded |= (byte_read & 0x7f) << (7 * i);

                // If the top bit becomes zero, it is done.
                if (!(byte_read & 0x80)) {
                    break;
                }
            }

            return decoded;
        }

        /// Returns the sleb128 value from the current head.
        ///
        /// The head moves forward to point the next value as a result.
        int32_t get_sleb128() const
        {
            uint32_t decoded = 0;

            for (int i = 0; i < 5; ++i) {
                // Read a new byte.
                const uint8_t byte_read = get<uint8_t>();

                decoded |= (byte_read & 0x7f) << (7 * i);

                // If the top bit becomes zero, it is done.
                if (!(byte_read & 0x80)) {
                    // If the second top bit is one, it needs to be extended.
                    if (byte_read & 0x40) {
                        // Sign extend by one.
                        for (++i; i < 5; ++i) {
                            decoded |= 0x7f << (7 * i);
                        }
                    }
                    break;
                }
            }

            return int32_t(decoded);
        }

        /// Returns the uleb128p1 value from the current head.
        ///
        /// The head moves forward to point the next value as a result.
        uint32_t get_uleb128p1() const
        {
            return get_uleb128() - 1;
        }

        /// Reads the array of bytes from the current head.
        ///
        /// The head moves forward to point the next value as a result.
        bool get_array(uint8_t* array, size_t length) const
        {
            // Copy the elements.
            for (size_t i = 0; i < length; ++i) {
                array[i] = get<uint8_t>();
            }

            return true;
        }

        /// Returns the null-terminated string from the current head.
        ///
        /// The head moves forward to point the next value as a result.
        const char* get_c_str() const
        {
            const char* str = reinterpret_cast<const char*>(head_ptr_);

            // Move the head pointer forward to the next value.
            while (*head_ptr_++ != '\0') {
                if (head_ptr_ >= end_ptr_) {
                    throw std::runtime_error("invalid string");
                }
            }

            return str;
        }

    private:
        /// Validates the head pointer against the begin/end pointers.
        void validate_head(size_t type_size = 0) const
        {
            if (head_ptr_ < begin_ptr_ || head_ptr_ + type_size > end_ptr_) {
                throw std::runtime_error("invalid offset");
            }
        }

        /// The begin pointer.
        const uint8_t* begin_ptr_;

        /// The end pointer.
        const uint8_t* end_ptr_;

        /// The head pointer.
        mutable const uint8_t* head_ptr_;
    };
}

#endif
