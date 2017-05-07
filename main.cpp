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

#include "jitana/util/axml_parser.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

void process_xml(const std::string& input_filename,
                 const std::string& output_filename)
{
    namespace boost_pt = boost::property_tree;

    // Property tree for storing the XML content.
    boost_pt::ptree pt;

    // Load from the input file.
    {
        try {
            // First, try to read as a binary XML file.
            jitana::read_axml(input_filename, pt);
        }
        catch (const jitana::axml_parser_not_an_axml_file& e) {
            // Binary parser has faied: try to read it as a normal XML file.
            boost_pt::read_xml(input_filename, pt);
        }
    }

    // Write decoded XML to the output.
    {
        // Construct the output stream.
        std::ostream* os = &std::cout;
        std::ofstream ofs;
        if (!output_filename.empty()) {
            ofs.open(output_filename);
            os = &ofs;
        }

        // Write the ptree to the output.
        {
#if BOOST_MAJOR_VERSION == 1 && BOOST_MINOR_VERSION < 56
            boost_pt::xml_writer_settings<char> settings(' ', 2);
#else
            boost_pt::xml_writer_settings<std::string> settings(' ', 2);
#endif
            boost_pt::write_xml(*os, pt, settings);
        }
    }
}

int main(int argc, char** argv)
{
    namespace po = boost::program_options;

    // Declare command line argument options.
    po::options_description desc("Allowed options");
    desc.add_options()("help", "Display available options")(
            "version", "Display version number")(
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

        if (vmap.count("version")) {
            // Print version and quit.
            std::cout << "axmldec ";
            std::cout << AXMLDEC_VERSION_MAJOR;
            std::cout << "." << AXMLDEC_VERSION_MINOR;
            std::cout << "." << AXMLDEC_VERSION_PATCH;
            std::cout << " (" << AXMLDEC_BUILD_TIMESTAMP << ")\n";
            std::cout << "Copyright (C) 2017 Yutaka Tsutano.\n";
            return 0;
        }

        if (vmap.count("help") || !vmap.count("input-file")) {
            // Print help and quit.
            std::cout << "Usage: axmldec [options] <input_file>\n\n";
            std::cout << desc << "\n";
            return 0;
        }

        if (vmap.count("input-file")) {
            auto input_filename = vmap["input-file"].as<std::string>();
            auto output_filename = vmap.count("output-file")
                    ? vmap["output-file"].as<std::string>()
                    : "";

            // Process the file.
            process_xml(input_filename, output_filename);
        }
    }
    catch (std::ios::failure& e) {
        std::cerr << "error: failed to open the input file\n";
        return 1;
    }
    catch (std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
