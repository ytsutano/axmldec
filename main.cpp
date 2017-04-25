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

#include <iostream>

#include "jitana/util/axml_parser.hpp"
#include <boost/property_tree/xml_parser.hpp>

void process(const std::string& filename)
{
    boost::property_tree::ptree pt;
    try {
        // First, try to read as a binary XML file.
        jitana::read_axml(filename, pt);
    }
    catch (const jitana::axml_parser_magic_mismatched& e) {
        // Binary parser has faied: try to read it as a normal XML file.
        boost::property_tree::read_xml(
                filename, pt, boost::property_tree::xml_parser::trim_whitespace,
                std::locale());
    }

    write_xml(std::cout, pt);
    std::cout << "\n";
}

int main(int argc, char** argv)
{
    if (argc <= 1) {
        std::cerr << "usage: " << argv[0] << " input_file\n";
        exit(1);
    }

    try {
        process(argv[1]);
    }
    catch (const jitana::axml_parser_error& e) {
        std::cerr << "binary parser failed: " << e.what() << "\n";
    }
    catch (const boost::property_tree::xml_parser::xml_parser_error& e) {
        std::cerr << "XML parser failed: " << e.what() << "\n";
    }
    catch (const std::runtime_error& e) {
        std::cerr << "error: " << e.what() << "\n";
    }
}
