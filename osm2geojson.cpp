
#include <iostream>
#include <cassert>
#include <getopt.h>

#include <osmium/index/map/all.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>

#include <osmium/geom/geojson.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>

typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;

typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

class JSONHandler : public osmium::handler::Handler {

    osmium::geom::GeoJSONFactory<> m_factory;
//    int m_fd;

public:

    JSONHandler(const std::string& /*filename*/) {
//        m_fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
//        assert(m_fd >= 0);
    }

    void node(const osmium::Node& node) {
        if (!node.tags().empty()) {
            std::string point = m_factory.create_point(node);
            std::cout << "{ \"geometry\": " <<  point;
            for (const auto& tag : node.tags()) {
                std::cout << ",{\"" << tag.key() << "\":\"" << tag.value() << "\"}";
            }
            std::cout << "}\n";
        }
//        write(m_fd, point.c_str(), point.size());
    }

    void way(const osmium::Way& way) {
        try {
            std::string linestring = m_factory.create_linestring(way);
            std::cout << "{ \"geometry\": " << linestring;
            for (const auto& tag : way.tags()) {
                std::cout << ",{\"" << tag.key() << "\":\"" << tag.value() << "\"}";
            }
            std::cout << "}\n";
        } catch(...) {
            return;
        }
//        write(m_fd, linestring.c_str(), linestring.size());
    }

};

/* ================================================== */

void print_help() {
    std::cout << "osmium_toogr [OPTIONS] [INFILE [OUTFILE]]\n\n" \
              << "If INFILE is not given stdin is assumed.\n" \
              << "If OUTFILE is not given 'ogr_out' is used.\n" \
              << "\nOptions:\n" \
              << "  -h, --help                 This help message\n" \
              << "  -l, --location_store=TYPE  Set location store\n" \
              << "  -f, --format=FORMAT        Output OGR format (Default: 'SQLite')\n" \
              << "  -L                         See available location stores\n";
}

int main(int argc, char* argv[]) {
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    static struct option long_options[] = {
        {"help",                 no_argument, 0, 'h'},
        {"location_store",       required_argument, 0, 'l'},
        {"dump",                 no_argument, 0, 'd'},
        {"list_location_stores", no_argument, 0, 'L'},
        {0, 0, 0, 0}
    };

    std::string location_store { "sparse_mem_array" };
    bool dump = false;

    while (true) {
        int c = getopt_long(argc, argv, "hdl:L", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                exit(0);
            case 'd':
                dump = true;
                break;
            case 'l':
                location_store = optarg;
                break;
            case 'L':
                std::cout << "Available map types:\n";
                for (const auto& map_type : map_factory.map_types()) {
                    std::cout << "  " << map_type << "\n";
                }
                exit(0);
            default:
                exit(1);
        }
    }

    std::string input_filename;
    std::string output_filename("out.json");
    int remaining_args = argc - optind;
    if (remaining_args > 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE [OUTFILE]]" << std::endl;
        exit(1);
    } else if (remaining_args == 2) {
        input_filename =  argv[optind];
        output_filename = argv[optind+1];
    } else if (remaining_args == 1) {
        input_filename =  argv[optind];
    } else {
        input_filename = "-";
    }

    osmium::io::Reader reader(input_filename);

    std::unique_ptr<index_pos_type> index_pos = map_factory.create_map(location_store);
    index_neg_type index_neg;
    location_handler_type location_handler(*index_pos, index_neg);
    location_handler.ignore_errors();

    JSONHandler json_handler(output_filename);

    osmium::apply(reader, location_handler, json_handler);
    reader.close();

    google::protobuf::ShutdownProtobufLibrary();

    if (dump) {
        int locations_fd = open("locations.dump", O_WRONLY | O_CREAT, 0644);
        if (locations_fd < 0) {
            throw std::system_error(errno, std::system_category(), "Open failed");
        }
        if (location_store.substr(0, 5) == "dense") {
            index_pos->dump_as_array(locations_fd);
        } else {
            index_pos->dump_as_list(locations_fd);
        }
        close(locations_fd);
    }
}

