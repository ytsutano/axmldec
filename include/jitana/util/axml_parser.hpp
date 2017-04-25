/*
 * Copyright (c) 2016, Yutaka Tsutano
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

#ifndef JITANA_AXML_PARSER_HPP
#define JITANA_AXML_PARSER_HPP

#include <type_traits>

#include <boost/property_tree/ptree.hpp>

#include "jitana/util/stream_reader.hpp"

namespace jitana {
    struct axml_parser_error : std::runtime_error {
        using runtime_error::runtime_error;
    };

    struct axml_parser_magic_mismatched : axml_parser_error {
        using axml_parser_error::axml_parser_error;
    };

    void read_axml(const std::string& filename,
                   boost::property_tree::ptree& pt);
}

#endif
