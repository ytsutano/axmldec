/*
 * Copyright (c) 2015, 2016, Yutaka Tsutano
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

#include <type_traits>

namespace jitana {
    /// An utility class for extracting values with variable length types from a
    /// memory space.
    class stream_reader {
    public:
        /// Creates a stream_reader instance.
        explicit stream_reader(const void* base_ptr = nullptr)
        {
            set_base(base_ptr);
        }

        /// Sets the base pointer.
        void set_base(const void* base_ptr)
        {
            base_ptr_ = reinterpret_cast<const uint8_t*>(base_ptr);
            move_head(0);
        }

        /// Returns the base pointer.
        const void* base() const
        {
            return base_ptr_;
        }

        /// Moves the head.
        void move_head(size_t pos = 0) const
        {
            head_ptr_ = base_ptr_ + pos;
        }

        /// Moves the head forward.
        void move_head_forward(int off = 0) const
        {
            head_ptr_ += off;
        }

        /// Returns the head index.
        size_t head() const
        {
            return head_ptr_ - base_ptr_;
        }

        /// Returns the reference to the value of specified type from the head
        /// without moving the head.
        template <typename T>
        const T& peek() const
        {
            return *reinterpret_cast<const T*>(head_ptr_);
        }

        /// Returns the reference to the value of specified type from pos
        /// without moving the head.
        template <typename T>
        const T& peek_at(size_t pos) const
        {
            return *reinterpret_cast<const T*>(base_ptr_ + pos);
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
            while (*head_ptr_++ != '\0')
                ;

            return str;
        }

    private:
        /// The base pointer.
        const uint8_t* base_ptr_;

        /// The head pointer.
        mutable const uint8_t* head_ptr_;
    };
}

#endif
