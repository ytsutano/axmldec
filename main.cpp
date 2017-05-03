/*
 * Copyright (c) 2016, 2017, Yutaka Tsutano
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
#include <vector>
#include <string>

#include "jitana/util/axml_parser.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

void process_xml(std::ostream& os, const std::string& filename)
{
    boost::property_tree::ptree pt;
    try {
        // First, try to read as a binary XML file.
        jitana::read_axml(filename, pt);
    }
    catch (const jitana::axml_parser_magic_mismatched& e) {
        // Binary parser has faied: try to read it as a normal XML file.
        boost::property_tree::read_xml(filename, pt);
    }

    boost::property_tree::xml_writer_settings<std::string> settings(' ', 2);
    write_xml(os, pt, settings);
}

int main(int argc, char** argv)
{
    namespace po = boost::program_options;

    // Declare command line argument options.
    po::options_description desc("Allowed options");
    desc.add_options()("help", "Display available options")(
            "input-file,i", po::value<std::string>(), "Input file")(
            "output-file,o", po::value<std::string>(), "Output file");
    po::positional_options_description p;
    p.add("input-file", -1);

    try {
        // Process the command line arguments.
        po::variables_map vmap;
        po::store(po::command_line_parser(argc, argv)
                          .options(desc)
                          .positional(p)
                          .run(),
                  vmap);
        po::notify(vmap);

        if (vmap.count("help") || !vmap.count("input-file")) {
            // Print help and quit.
            std::cout << "Usage: axmldec [options] <input_file>\n";
            std::cout << desc << "\n";
            return 0;
        }

        if (vmap.count("input-file")) {
            // Construct the output stream.
            std::ostream* os = &std::cout;
            std::ofstream ofs;
            if (vmap.count("output-file")) {
                ofs.open(vmap["output-file"].as<std::string>());
                os = &ofs;
            }

            // Process the file.
            process_xml(*os, vmap["input-file"].as<std::string>());
        }
    }
    catch (std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
