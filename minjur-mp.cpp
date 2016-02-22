
#include <iostream>
#include <fstream>
#include <cassert>
#include <set>
#include <getopt.h>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/geom/geojson.hpp>
#include <osmium/geom/tile.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/tags/filter.hpp>
#include <osmium/tags/taglist.hpp>

// these must be include in this order
#include <osmium/index/map/all.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

#include "json_feature.hpp"

typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;

typedef osmium::handler::NodeLocationsForWays<index_pos_type> location_handler_type;


class JSONHandler : public osmium::handler::Handler {

    std::string m_buffer;
    int m_geometry_error_count;
    std::string m_attr_prefix;
    std::unique_ptr<std::ofstream> m_error_stream;

    void flush_to_output() {
        auto written = write(1, m_buffer.data(), m_buffer.size());
        assert(written == long(m_buffer.size()));
        m_buffer.clear();
    }

    void maybe_flush() {
        if (m_buffer.size() > 1024*1024) {
            flush_to_output();
        }
    }

    void report_geometry_problem(const osmium::OSMObject& object, const char* error) {
        ++m_geometry_error_count;
        if (m_error_stream) {
            *m_error_stream << osmium::item_type_to_char(object.type()) << object.id() << ":" << error << "\n";
        }
    }

public:

    JSONHandler(const std::string& error_file, const std::string& attr_prefix) :
        m_buffer(),
        m_geometry_error_count(0),
        m_attr_prefix(attr_prefix),
        m_error_stream(nullptr) {
        if (!error_file.empty()) {
            m_error_stream.reset(new std::ofstream(error_file));
        }
    }

    ~JSONHandler() {
        flush_to_output();
    }

    void node(const osmium::Node& node) {
        if (node.tags().empty()) {
            return;
        }

        try {
            JSONFeature feature(m_attr_prefix);
            feature.add_point(node);
            feature.add_properties(node);
            feature.append_to(m_buffer);
        } catch (osmium::geometry_error&) {
            report_geometry_problem(node, "geometry_error");
        } catch (osmium::invalid_location&) {
            report_geometry_problem(node, "invalid_location");
        }

        maybe_flush();
    }

    void way(const osmium::Way& way) {
        try {
            JSONFeature feature(m_attr_prefix);
            feature.add_linestring(way);
            feature.add_properties(way);
            feature.append_to(m_buffer);
        } catch (osmium::geometry_error&) {
            report_geometry_problem(way, "geometry_error");
        } catch (osmium::invalid_location&) {
            report_geometry_problem(way, "invalid_location");
        }

        maybe_flush();
    }

    void area(const osmium::Area& area) {
        try {
            JSONFeature feature(m_attr_prefix);
            feature.add_multipolygon(area);
            feature.add_properties(area);
            feature.append_to(m_buffer);
        } catch (osmium::geometry_error&) {
            report_geometry_problem(area, "geometry_error");
        } catch (osmium::invalid_location&) {
            report_geometry_problem(area, "invalid_location");
        }

        maybe_flush();
    }

    int geometry_error_count() const {
        return m_geometry_error_count;
    }

}; // class JSONHandler

/* ================================================== */

void print_help() {
    std::cout << "minjur [OPTIONS] INFILE\n\n" \
              << "Output is always to stdout.\n" \
              << "\nOptions:\n" \
              << "  -e, --error-file=FILE      Write errors to file\n" \
              << "  -h, --help                 This help message\n" \
              << "  -l, --location-store=TYPE  Set location store\n" \
              << "  -L, --list-location-stores Show available location stores\n" \
              << "  -n, --nodes=sparse|dense   Are node IDs sparse or dense?\n" \
              << "  -a, --attr-prefix=PREFIX   Optional prefix for attributes, defaults to '@'\n";
}


int main(int argc, char* argv[]) {
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    static struct option long_options[] = {
        {"dump",                 required_argument, 0, 'd'},
        {"error-file",           required_argument, 0, 'e'},
        {"help",                       no_argument, 0, 'h'},
        {"location-store",       required_argument, 0, 'l'},
        {"list-location-stores",       no_argument, 0, 'L'},
        {"nodes",                required_argument, 0, 'n'},
        {"attr-prefix",          required_argument, 0, 'a'},
        {0, 0, 0, 0}
    };

    std::string location_store;
    std::string locations_dump_file;
    std::string error_file;
    std::string attr_prefix = "@";
    bool nodes_dense = false;

    while (true) {
        int c = getopt_long(argc, argv, "d:e:hl:Ln:a:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'd':
                locations_dump_file = optarg;
                break;
            case 'e':
                error_file = optarg;
                break;
            case 'h':
                print_help();
                exit(0);
            case 'l':
                location_store = optarg;
                break;
            case 'L':
                std::cout << "Available map types:\n";
                for (const auto& map_type : map_factory.map_types()) {
                    std::cout << "  " << map_type << "\n";
                }
                exit(0);
            case 'n':
                if (!strcmp(optarg, "sparse")) {
                    nodes_dense = false;
                } else if (!strcmp(optarg, "dense")) {
                    nodes_dense = true;
                } else {
                    std::cerr << "Set --nodes, -n to 'sparse' or 'dense'\n";
                    exit(1);
                }
                break;
            case 'a':
                attr_prefix = optarg;
                break;
            default:
                exit(1);
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
    int remaining_args = argc - optind;
    if (remaining_args == 1) {
        input_filename = argv[optind];
        std::cerr << "Reading from '" << input_filename << "'...\n";
    } else {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] INFILE\n";
        exit(1);
    }

    osmium::area::Assembler::config_type assembler_config;
    osmium::area::MultipolygonCollector<osmium::area::Assembler> collector(assembler_config);

    std::cerr << "Pass 1...\n";
    osmium::io::Reader reader1(input_filename, osmium::osm_entity_bits::relation);
    collector.read_relations(reader1);
    reader1.close();
    std::cerr << "Pass 1 done\n";


    std::unique_ptr<index_pos_type> index_pos = map_factory.create_map(location_store);
    location_handler_type location_handler(*index_pos);
    location_handler.ignore_errors();

    JSONHandler json_handler(error_file, attr_prefix);

    std::cerr << "Pass 2...\n";
    osmium::io::Reader reader2(input_filename);
    osmium::apply(reader2, location_handler, json_handler, collector.handler([&json_handler](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, json_handler);
    }));
    reader2.close();
    std::cerr << "Pass 2 done\n";


    if (json_handler.geometry_error_count()) {
        std::cerr << "Number of geometry errors (not written to output): " << json_handler.geometry_error_count() << "\n";
    }

    if (!locations_dump_file.empty()) {
        std::cerr << "Writing locations store to '" << locations_dump_file << "'...\n";
        int locations_fd = open(locations_dump_file.c_str(), O_WRONLY | O_CREAT, 0644);
        if (locations_fd < 0) {
            std::cerr << "Can not open file: " << strerror(errno) << "\n";
            exit(1);
        }
        if (location_store.substr(0, 5) == "dense") {
            index_pos->dump_as_array(locations_fd);
        } else {
            index_pos->dump_as_list(locations_fd);
        }
        close(locations_fd);
    }

    std::cerr << "Done.\n";
}
