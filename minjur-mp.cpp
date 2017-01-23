
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/handler/check_order.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/visitor.hpp>

// these must be include in this order
#include <osmium/index/map/all.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

#include "minjur_version.hpp"
#include "json_feature.hpp"
#include "json_handler.hpp"

using index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;


class JSONAreaHandler : public JSONHandler {

public:

    JSONAreaHandler(const std::string& error_file, const std::string& attr_prefix, bool with_id) :
        JSONHandler(error_file, attr_prefix, with_id) {
    }

    void node(const osmium::Node& node) {
        if (node.tags().empty()) {
            return;
        }

        try {
            JSONFeature feature{attr_names()};
            if (with_id()) {
                feature.add_id("n", node.id());
            }
            feature.add_point(node);
            feature.add_properties(node);
            feature.append_to(buffer());
        } catch (const osmium::geometry_error&) {
            report_geometry_problem(node, "geometry_error");
        } catch (const osmium::invalid_location&) {
            report_geometry_problem(node, "invalid_location");
        }

        maybe_flush();
    }

    void way(const osmium::Way& way) {
        if (way.nodes().size() <= 1) {
            return;
        }
        try {
            JSONFeature feature{attr_names()};
            if (with_id()) {
                feature.add_id("w", way.id());
            }
            feature.add_linestring(way);
            feature.add_properties(way);
            feature.append_to(buffer());
        } catch (const osmium::geometry_error&) {
            report_geometry_problem(way, "geometry_error");
        } catch (const osmium::invalid_location&) {
            report_geometry_problem(way, "invalid_location");
        }

        maybe_flush();
    }

    void area(const osmium::Area& area) {
        try {
            JSONFeature feature{attr_names()};
            if (with_id()) {
                feature.add_id("a", area.id());
            }
            feature.add_multipolygon(area);
            feature.add_properties(area);
            feature.append_to(buffer());
        } catch (const osmium::geometry_error&) {
            report_geometry_problem(area, "geometry_error");
        } catch (const osmium::invalid_location&) {
            report_geometry_problem(area, "invalid_location");
        }

        maybe_flush();
    }

}; // class JSONAreaHandler

/* ================================================== */

void print_help() {
    std::cout << "minjur-mp [OPTIONS] INFILE\n\n"
              << "Output is always to stdout.\n"
              << "\nOptions:\n"
              << "  -e, --error-file=FILE      Write errors to file\n"
              << "  -h, --help                 This help message\n"
              << "  -v, --version                 Display version\n"
              << "  -i, --with-id              Add unique id to each feature\n"
              << "  -l, --location-store=TYPE  Set location store\n"
              << "  -L, --list-location-stores Show available location stores\n"
              << "  -n, --nodes=sparse|dense   Are node IDs sparse or dense?\n"
              << "  -a, --attr-prefix=PREFIX   Optional prefix for attributes, defaults to '@'\n";
}

void print_version() {
    std::cout << MINJUR_VERSION_STRING << "\n";
}

int main(int argc, char* argv[]) {
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    static struct option long_options[] = {
        {"error-file",           required_argument, 0, 'e'},
        {"help",                       no_argument, 0, 'h'},
        {"version",                    no_argument, 0, 'v'},
        {"with-id",                    no_argument, 0, 'i'},
        {"location-store",       required_argument, 0, 'l'},
        {"list-location-stores",       no_argument, 0, 'L'},
        {"nodes",                required_argument, 0, 'n'},
        {"attr-prefix",          required_argument, 0, 'a'},
        {0, 0, 0, 0}
    };

    std::string location_store;
    std::string error_file;
    std::string attr_prefix = "@";
    bool nodes_dense = false;
    bool with_id = false;

    while (true) {
        int c = getopt_long(argc, argv, "e:hil:Ln:a:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'e':
                error_file = optarg;
                break;
            case 'h':
                print_help();
                std::exit(0);
            case 'v':
                print_version();
                std::exit(0);
            case 'i':
                with_id = true;
                break;
            case 'l':
                location_store = optarg;
                break;
            case 'L':
                std::cout << "Available map types:\n";
                for (const auto& map_type : map_factory.map_types()) {
                    std::cout << "  " << map_type << "\n";
                }
                std::exit(0);
            case 'n':
                if (!std::strcmp(optarg, "sparse")) {
                    nodes_dense = false;
                } else if (!std::strcmp(optarg, "dense")) {
                    nodes_dense = true;
                } else {
                    std::cerr << "Set --nodes, -n to 'sparse' or 'dense'\n";
                    std::exit(1);
                }
                break;
            case 'a':
                attr_prefix = optarg;
                break;
            default:
                std::exit(1);
        }
    }

    if (location_store.empty()) {
        location_store = nodes_dense ? "dense" : "sparse";

        if (map_factory.has_map_type(location_store + "_mmap_array")) {
            location_store.append("_mmap_array");
        } else {
            location_store.append("_mem_array");
        }
    }
    std::cerr << "Using the '" << location_store << "' location store. Use -l or -n to change this.\n";

    std::string input_filename;
    const int remaining_args = argc - optind;
    if (remaining_args == 1) {
        input_filename = argv[optind];
        std::cerr << "Reading from '" << input_filename << "'...\n";
    } else {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] INFILE\n";
        std::exit(1);
    }

    osmium::area::Assembler::config_type assembler_config;
    osmium::area::MultipolygonCollector<osmium::area::Assembler> collector{assembler_config};

    std::cerr << "Pass 1...\n";
    osmium::io::Reader reader1{input_filename, osmium::osm_entity_bits::relation};
    collector.read_relations(reader1);
    reader1.close();
    std::cerr << "Pass 1 done\n";


    std::unique_ptr<index_type> index = map_factory.create_map(location_store);
    location_handler_type location_handler{*index};
    location_handler.ignore_errors();

    JSONAreaHandler json_handler{error_file, attr_prefix, with_id};
    osmium::handler::CheckOrder check_order_handler;

    std::cerr << "Pass 2...\n";
    osmium::io::Reader reader2{input_filename};
    osmium::apply(reader2, check_order_handler, location_handler, json_handler, collector.handler([&json_handler](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, json_handler);
    }));
    reader2.close();
    std::cerr << "Pass 2 done\n";


    if (json_handler.geometry_error_count()) {
        std::cerr << "Number of geometry errors (not written to output): " << json_handler.geometry_error_count() << "\n";
    }

    std::cerr << "Done.\n";
}

