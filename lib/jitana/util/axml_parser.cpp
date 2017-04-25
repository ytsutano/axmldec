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
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <boost/iostreams/device/mapped_file.hpp>

#include "jitana/util/axml_parser.hpp"
#include "jitana/util/stream_reader.hpp"

using namespace jitana;

namespace jitana {
    class axml_parser {
    private:
        static constexpr uint16_t res_null_type = 0x0001;
        static constexpr uint16_t res_string_pool_type = 0x0001;
        static constexpr uint16_t res_table_type = 0x0002;
        static constexpr uint16_t res_xml_type = 0x0003;

        static constexpr uint16_t res_xml_first_chunk_type = 0x0100;
        static constexpr uint16_t res_xml_start_namespace_type = 0x0100;
        static constexpr uint16_t res_xml_end_namespace_type = 0x0101;
        static constexpr uint16_t res_xml_start_element_type = 0x0102;
        static constexpr uint16_t res_xml_end_element_type = 0x0103;
        static constexpr uint16_t res_xml_cdata_type = 0x0104;
        static constexpr uint16_t res_xml_last_chunk_type = 0x017f;
        static constexpr uint16_t res_xml_resource_map_type = 0x0180;

        static constexpr uint16_t res_table_package_type = 0x0200;
        static constexpr uint16_t res_table_type_type = 0x0201;
        static constexpr uint16_t res_table_type_spec_type = 0x0202;
        static constexpr uint16_t res_table_library_type = 0x0203;

    public:
        enum complex_unit {
            complex_unit_px = 0,
            complex_unit_dip = 1,
            complex_unit_sp = 2,
            complex_unit_pt = 3,
            complex_unit_in = 4,
            complex_unit_mm = 5,
            complex_unit_fraction = 0,
            complex_unit_fraction_parent = 1
        };

        struct res_chunk_header {
            uint16_t type;
            uint16_t header_size;
            uint32_t size;
        };

        struct resource_value {
            uint16_t size;
            uint8_t res0;
            uint8_t data_type;
            uint32_t data;

            enum {
                // The 'data' is either 0 or 1, specifying this resource is
                // either undefined or empty, respectively.
                type_null = 0x00,
                // The 'data' holds a ResTable_ref, a reference to another
                // resource table entry.
                type_reference = 0x01,
                // The 'data' holds an attribute resource identifier.
                type_attribute = 0x02,
                // The 'data' holds an index into the containing resource
                // table's global value string pool.
                type_string = 0x03,
                // The 'data' holds a single-precision floating point number.
                type_float = 0x04,
                // The 'data' holds a complex number encoding a dimension value,
                // such as "100in".
                type_dimension = 0x05,
                // The 'data' holds a complex number encoding a fraction of a
                // container.
                type_fraction = 0x06,
                // The 'data' holds a dynamic ResTable_ref, which needs to be
                // resolved before it can be used like a type_reference.
                type_dynamic_reference = 0x07,

                // Beginning of integer flavors...
                type_first_int = 0x10,

                // The 'data' is a raw integer value of the form n..n.
                type_int_dec = 0x10,
                // The 'data' is a raw integer value of the form 0xn..n.
                type_int_hex = 0x11,
                // The 'data' is either 0 or 1, for input "false" or "true"
                // respectively.
                type_int_boolean = 0x12,

                // Beginning of color integer flavors...
                type_first_color_int = 0x1c,

                // The 'data' is a raw integer value of the form #aarrggbb.
                type_int_color_argb8 = 0x1c,
                // The 'data' is a raw integer value of the form #rrggbb.
                type_int_color_rgb8 = 0x1d,
                // The 'data' is a raw integer value of the form #argb.
                type_int_color_argb4 = 0x1e,
                // The 'data' is a raw integer value of the form #rgb.
                type_int_color_rgb4 = 0x1f,

                // ...end of integer flavors.
                type_last_color_int = 0x1f,

                // ...end of integer flavors.
                type_last_int = 0x1f
            };

            friend inline std::ostream& operator<<(std::ostream& os,
                                                   const resource_value& x)
            {
                auto print_complex = [&](bool frac) {
                    constexpr float mantissa_mult = 1.0f / (1 << 8);
                    constexpr float radix_mults[]
                            = {mantissa_mult * 1.0f,
                               mantissa_mult * 1.0f / (1 << 7),
                               mantissa_mult * 1.0f / (1 << 15),
                               mantissa_mult * 1.0f / (1 << 23)};
                    float value = static_cast<int32_t>(x.data & 0xffffff00)
                            * radix_mults[(x.data >> 4) & 0x3];

                    if (frac) {
                        os << value * 100;
                        switch (x.data & 0xf) {
                        case complex_unit_fraction:
                            os << "%";
                            break;
                        case complex_unit_fraction_parent:
                            os << "%p";
                            break;
                        }
                    }
                    else {
                        os << value;
                        switch (x.data & 0xf) {
                        case complex_unit_px:
                            os << "px";
                            break;
                        case complex_unit_dip:
                            os << "dip";
                            break;
                        case complex_unit_sp:
                            os << "sp";
                            break;
                        case complex_unit_pt:
                            os << "pt";
                            break;
                        case complex_unit_in:
                            os << "in";
                            break;
                        case complex_unit_mm:
                            os << "mm";
                            break;
                        }
                    }
                };

                switch (x.data_type) {
                case type_null:
                    os << "null";
                    break;
                // case type_reference:
                //     break;
                // case type_attribute:
                //     break;
                // case type_string:
                //     break;
                case type_float:
                    float f;
                    std::memcpy(&f, &x.data, sizeof(f));
                    os << f;
                    break;
                case type_dimension:
                    print_complex(false);
                    break;
                case type_fraction:
                    print_complex(true);
                    break;
                // case type_dynamic_reference:
                //     break;
                // case type_first_int:
                //     break;
                case type_int_dec:
                    os << std::dec << x.data;
                    break;
                case type_int_hex:
                    os << "0x" << std::hex << x.data;
                    break;
                case type_int_boolean:
                    os << (x.data ? "true" : "false");
                    break;
                // case type_first_color_int:
                //     os << x.data;
                //     break;
                // case type_int_color_argb8:
                //     break;
                // case type_int_color_rgb8:
                //     break;
                // case type_int_color_argb4:
                //     break;
                // case type_int_color_rgb4:
                //     break;
                // case type_last_color_int:
                //     break;
                // case type_last_int:
                //     break;
                default:
                    os << "type" << (int)x.data_type << "/" << x.data;
                }

                return os;
            }
        };

    public:
        axml_parser(stream_reader& reader, boost::property_tree::ptree& pt)
                : reader_(reader), pt_(pt)
        {
        }

        void parse()
        {
            xml_stack_.clear();
            xml_stack_.emplace_back(&pt_);

            const auto& header = reader_.get<res_chunk_header>();

            // Make sure it's the right file type.
            if (header.type != res_xml_type) {
                throw axml_parser_magic_mismatched("not a binary XML file");
            }

            // Apply pull parsing.
            while (reader_.head() < static_cast<size_t>(header.size)) {
                auto saved_reader = reader_;

                const auto& header = reader_.peek<res_chunk_header>();
                switch (header.type) {
                case res_string_pool_type:
                    parse_string_pool();
                    break;
                case res_xml_resource_map_type:
                    parse_resource_map();
                    break;
                case res_xml_start_namespace_type:
                    parse_start_namespace();
                    break;
                case res_xml_end_namespace_type:
                    parse_end_namespace();
                    break;
                case res_xml_start_element_type:
                    parse_xml_start_element();
                    break;
                case res_xml_end_element_type:
                    parse_xml_end_element();
                    break;
                case res_xml_cdata_type:
                    parse_xml_cdata();
                    break;
                default:
                    std::stringstream ss;
                    ss << "unknown chunk type 0x" << std::hex << header.type;
                    throw axml_parser_error(ss.str());
                }

                reader_ = saved_reader;
                reader_.move_head_forward(header.size);
            }
        }

    private:
        void parse_string_pool()
        {
            /*const auto& header =*/reader_.get<res_chunk_header>();

            auto string_count = reader_.get<uint32_t>();
            auto style_count = reader_.get<uint32_t>();
            auto flags = reader_.get<uint32_t>();
            // bool sorted_flag = flags & (1 << 0);
            bool utf8_flag = flags & (1 << 8);
            auto strings_start = reader_.get<uint32_t>();
            /*auto styles_start =*/reader_.get<uint32_t>();

            if (style_count != 0) {
                throw axml_parser_error("styles are not supported");
            }

            // Get the string offsets.
            std::vector<uint32_t> string_offsets(string_count);
            for (auto& off : string_offsets) {
                off = reader_.get<uint32_t>();
            }

            // Fill the string table. TODO: support unicode.
            strings_.clear();
            strings_.reserve(string_count);
            for (auto off : string_offsets) {
                reader_.move_head(strings_start + 8 + off);

                strings_.emplace_back();
                auto& str = strings_.back();

                if (utf8_flag) {
                    reader_.get<uint8_t>();

                    // Compute the string length.
                    size_t len = reader_.get<uint8_t>();
                    if (len & 0x80) {
                        /*len |= ((len & 0x7f) << 8) |*/ reader_.get<uint8_t>();
                    }

                    // Fill characters.
                    if (len != 0) {
                        str = reader_.get_c_str();
                    }
                }
                else {
                    // Compute the string length.
                    size_t len = reader_.get<uint16_t>();
                    if (len & 0x8000) {
                        len |= ((len & 0x7fff) << 16) | reader_.get<uint16_t>();
                    }

                    // Fill characters.
                    str.resize(len);
                    for (auto& c : str) {
                        c = reader_.get<uint16_t>();
                    }

                    if (reader_.get<uint16_t>() != 0) {
                        throw axml_parser_error("invalid UTF-16 string");
                    }
                }
            }
        }

        void parse_resource_map()
        {
            /*const auto& header =*/reader_.get<res_chunk_header>();

            /*auto package_count =*/reader_.get<uint32_t>();
        }

        void parse_start_namespace()
        {
            /*const auto& header =*/reader_.get<res_chunk_header>();

            /*auto line_num =*/reader_.get<uint32_t>();
            /*auto comment =*/reader_.get<uint32_t>();
            auto prefix = reader_.get<uint32_t>();
            auto uri = reader_.get<uint32_t>();

            xml_stack_.back().namespaces.emplace_back(uri, prefix);
        }

        void parse_end_namespace()
        {
            /*const auto& header =*/reader_.get<res_chunk_header>();

            /*auto line_num =*/reader_.get<uint32_t>();
            /*auto comment =*/reader_.get<uint32_t>();
            /*auto prefix =*/reader_.get<uint32_t>();
            /*auto uri =*/reader_.get<uint32_t>();

            xml_stack_.back().namespaces.pop_back();
        }

        void parse_xml_start_element()
        {
            using path_type = boost::property_tree::ptree::path_type;

            /*const auto& header =*/reader_.get<res_chunk_header>();

            /*auto line_num =*/reader_.get<uint32_t>();
            /*auto comment =*/reader_.get<uint32_t>();
            /*auto ns =*/reader_.get<uint32_t>();
            auto name = reader_.get<uint32_t>();
            /*auto attribute_size =*/reader_.get<uint32_t>();
            auto attribute_count = reader_.get<uint16_t>();
            /*auto id_index =*/reader_.get<uint16_t>();
            /*auto class_index =*/reader_.get<uint16_t>();
            /*auto style_index =*/reader_.get<uint16_t>();

            // Create ptree for the new element.
            auto& elem_pt = xml_stack_.back().pt->add(
                    path_type(strings_[name], '`'), "");
            for (const auto& ns : xml_stack_.back().namespaces) {
                elem_pt.add(path_type("<xmlattr>`xmlns:" + strings_[ns.second],
                                      '`'),
                            strings_[ns.first]);
            }
            xml_stack_.emplace_back(&elem_pt);

            // Create ptree for the attributes.
            for (int i = 0; i < attribute_count; ++i) {
                auto attr_ns = reader_.get<uint32_t>();
                auto attr_name = reader_.get<uint32_t>();
                auto attr_raw_val = reader_.get<uint32_t>();
                auto value = reader_.get<resource_value>();

                std::string name = "<xmlattr>`";
                if (attr_ns != 0xffffffff) {
                    auto prefix = lookup_prefix(attr_ns);
                    if (prefix != 0xffffffff) {
                        // Add namespace prefix.
                        name += strings_[prefix];
                        name += ":";
                    }
                }
                name += strings_[attr_name];

                std::stringstream ss;
                if (attr_raw_val != 0xffffffff) {
                    ss << strings_[attr_raw_val];
                }
                else {
                    // TODO: print in human readable format.
                    ss << value;
                }

                elem_pt.add(path_type(name, '`'), ss.str());
            }
        }

        void parse_xml_end_element()
        {
            /*const auto& header =*/reader_.get<res_chunk_header>();

            /*auto line_num =*/reader_.get<uint32_t>();
            /*auto comment =*/reader_.get<uint32_t>();
            /*auto ns =*/reader_.get<uint32_t>();
            /*auto name =*/reader_.get<uint32_t>();

            xml_stack_.pop_back();
        }

        void parse_xml_cdata()
        {
            /*const auto& header =*/reader_.get<res_chunk_header>();

            /*auto line_num =*/reader_.get<uint32_t>();
            /*auto comment =*/reader_.get<uint32_t>();
            auto text = reader_.get<uint32_t>();
            reader_.get<uint32_t>();
            reader_.get<uint32_t>();

            xml_stack_.back().pt->add("<xmltext>", strings_[text]);
        }

        uint32_t lookup_prefix(uint32_t uri)
        {
            auto stack_it = rbegin(xml_stack_);
            ++stack_it;
            for (; stack_it != rend(xml_stack_); ++stack_it) {
                const auto& v = stack_it->namespaces;
                auto it = std::find_if(rbegin(v), rend(v), [&](const auto& x) {
                    return x.first == uri;
                });
                if (it != rend(v)) {
                    return it->second;
                }
            }

            return 0xffffffff;
        }

    private:
        stream_reader& reader_;
        std::vector<std::string> strings_;
        boost::property_tree::ptree& pt_;

        struct xml_stack_item {
            boost::property_tree::ptree* pt;
            std::vector<std::pair<uint32_t, uint32_t>> namespaces;

            xml_stack_item(boost::property_tree::ptree* pt) : pt(pt)
            {
            }
        };
        std::vector<xml_stack_item> xml_stack_;
    };
}

void jitana::read_axml(const std::string& filename,
                       boost::property_tree::ptree& pt)
{
    boost::iostreams::mapped_file file(filename);

    stream_reader reader(file.data());
    axml_parser p(reader, pt);
    p.parse();
}
