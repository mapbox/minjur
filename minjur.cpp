
#include <iostream>
#include <cassert>
#include <getopt.h>

#include <osmium/index/map/all.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>

#include <osmium/geom/geojson.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>

#include "rapid_geojson.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#pragma GCC diagnostic pop

typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;

typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

class JSONHandler : public osmium::handler::Handler {

    typedef rapidjson::Writer<rapidjson::StringBuffer> writer_type;

    rapidjson::StringBuffer m_stream;
    writer_type m_writer;
    osmium::geom::RapidGeoJSONFactory<writer_type> m_factory;

    void write_tags(const osmium::TagList& tags) {
        m_writer.String("properties");

        m_writer.StartObject();
        for (const auto& tag : tags) {
            m_writer.String(tag.key());
            m_writer.String(tag.value());
        }
        m_writer.EndObject();
    }

    void flush_to_output() {
        auto written = write(1, m_stream.GetString(), m_stream.GetSize());
        assert(written == long(m_stream.GetSize()));
        m_stream.Clear();
    }

    void maybe_flush() {
        if (m_stream.GetSize() > 1024*1024) {
            flush_to_output();
        }
        m_writer.Reset(m_stream);
    }

public:

    JSONHandler() :
        m_stream(),
        m_writer(m_stream),
        m_factory(m_writer) {
    }

    ~JSONHandler() {
        flush_to_output();
    }

    void node(const osmium::Node& node) {
        if (node.tags().empty()) {
            return;
        }

        m_writer.StartObject();
        m_writer.String("type");
        m_writer.String("Feature");

        m_factory.create_point(node);
        write_tags(node.tags());

        m_writer.EndObject();

        m_stream.Put('\n');
        maybe_flush();
    }

    void way(const osmium::Way& way) {
        try {
            m_writer.StartObject();
            m_writer.String("type");
            m_writer.String("Feature");

            m_factory.create_linestring(way);
            write_tags(way.tags());

            m_writer.EndObject();

            m_stream.Put('\n');
            maybe_flush();

        } catch(...) {
            return;
        }
    }

};

/* ================================================== */

void print_help() {
    std::cout << "minjur [OPTIONS] [INFILE]\n\n" \
              << "If INFILE is not given stdin is assumed.\n" \
              << "Output is always to stdout.\n" \
              << "\nOptions:\n" \
              << "  -d, --dump                 Dump location store after run\n" \
              << "  -h, --help                 This help message\n" \
              << "  -l, --location_store=TYPE  Set location store\n" \
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
    int remaining_args = argc - optind;
    if (remaining_args > 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE]" << std::endl;
        exit(1);
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

    JSONHandler json_handler;

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

