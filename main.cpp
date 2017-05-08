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

#ifdef ENABLE_APK_LOADING
#include <unzip.h>
#endif

namespace boost_pt = boost::property_tree;

struct membuf : std::streambuf {
    membuf(const char* base, size_t size)
    {
        char* p(const_cast<char*>(base));
        this->setg(p, p, p + size);
    }
};

struct imemstream : virtual membuf, std::istream {
    imemstream(char const* base, size_t size)
            : membuf(base, size),
              std::istream(static_cast<std::streambuf*>(this))
    {
    }
};

std::vector<char> extract_manifest(const std::string& input_filename)
{
#ifdef ENABLE_APK_LOADING
    auto* apk = unzOpen(input_filename.c_str());
    if (apk == nullptr) {
        throw std::runtime_error("not an APK file");
    }

    if (unzLocateFile(apk, "AndroidManifest.xml", 0) != UNZ_OK) {
        unzClose(apk);
        throw std::runtime_error("AndroidManifest.xml is not found in APK");
    }

    if (unzOpenCurrentFile(apk) != UNZ_OK) {
        unzClose(apk);
        throw std::runtime_error("failed to open AndroidManifest.xml in APK");
    }

    unz_file_info64 info;
    if (unzGetCurrentFileInfo64(apk, &info, nullptr, 0, nullptr, 0, nullptr, 0)
        != UNZ_OK) {
        unzCloseCurrentFile(apk);
        unzClose(apk);
        throw std::runtime_error("failed to open AndroidManifest.xml in APK");
    }

    std::vector<char> content(info.uncompressed_size);
    constexpr size_t read_size = 1 << 15;
    for (size_t offset = 0;; offset += read_size) {
        int len = unzReadCurrentFile(apk, content.data() + offset, read_size);
        if (len == 0) {
            break;
        }
        else if (len < 0) {
            unzCloseCurrentFile(apk);
            unzClose(apk);
            throw std::runtime_error(
                    "failed to read AndroidManifest.xml in APK");
        }
    }

    unzCloseCurrentFile(apk);
    unzClose(apk);

    return content;
#else
    (void)input_filename;
    throw std::runtime_error("axmldec is compiled without APK loading support");
#endif
}

void write_xml(const std::string& output_filename, const boost_pt::ptree& pt)
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

void process_file(const std::string& input_filename,
                  const std::string& output_filename)
{
    // Property tree for storing the XML content.
    boost_pt::ptree pt;

    // Load the XML into ptree.
    std::ifstream ifs(input_filename, std::ios::binary);
    if (ifs.peek() == 'P') {
        auto content = extract_manifest(input_filename);
        imemstream ims(content.data(), content.size());
        jitana::read_axml(ims, pt);
    }
    else if (ifs.peek() == 0x03) {
        jitana::read_axml(ifs, pt);
    }
    else {
        boost_pt::read_xml(ifs, pt, boost_pt::xml_parser::trim_whitespace);
    }

    // Write the tree as an XML file.
    write_xml(output_filename, pt);
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
            std::cout << "APK loading support: ";
#ifdef ENABLE_APK_LOADING
            std::cout << "enabled\n";
#else
            std::cout << "disabled\n";
#endif
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
            process_file(input_filename, output_filename);
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
