
#include <iostream>
#include <fstream>
#include <cassert>
#include <set>
#include <getopt.h>

#include <osmium/index/map/all.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>

#include <osmium/geom/geojson.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>

#include "rapid_geojson.hpp"
#include "tile.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#pragma GCC diagnostic pop

typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;

typedef osmium::handler::NodeLocationsForWays<index_pos_type> location_handler_type;

typedef rapidjson::Writer<rapidjson::StringBuffer> writer_type;

typedef std::set<std::pair<unsigned int, unsigned int>> tileset_type;

class JSONFeature {

    rapidjson::StringBuffer m_stream;
    writer_type m_writer;
    osmium::geom::RapidGeoJSONFactory<writer_type> m_factory;

public:

    JSONFeature() :
        m_stream(),
        m_writer(m_stream),
        m_factory(m_writer) {
        m_writer.StartObject();
        m_writer.String("type");
        m_writer.String("Feature");
    }

    void add_point(const osmium::Node& node) {
        m_factory.create_point(node);
    }

    void add_linestring(const osmium::Way& way) {
        m_factory.create_linestring(way);
    }

    void add_polygon(const osmium::Way& way) {
        m_factory.create_polygon(way);
    }

    void add_tags(const osmium::TagList& tags, const char* id_name, osmium::object_id_type id) {
        m_writer.String("properties");

        m_writer.StartObject();
        m_writer.String(id_name);
        m_writer.Double(id);
        for (const auto& tag : tags) {
            m_writer.String(tag.key());
            m_writer.String(tag.value());
        }
        m_writer.EndObject();
    }

    void append_to(std::string& buffer) {
        m_writer.EndObject();

        buffer.append(m_stream.GetString(), m_stream.GetSize());
        buffer.append(1, '\n');
    }

}; // class JSONFeature


class JSONHandler : public osmium::handler::Handler {

    std::string m_buffer;
    int m_geometry_error_count;
    bool m_create_polygons;
    tileset_type m_tiles;
    unsigned int m_zoom;

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

public:

    JSONHandler(bool create_polygons, const tileset_type& tiles, unsigned int zoom) :
        m_buffer(),
        m_create_polygons(create_polygons),
        m_tiles(tiles),
        m_zoom(zoom) {
    }

    ~JSONHandler() {
        flush_to_output();
    }

    void node(const osmium::Node& node) {
        if (node.tags().empty()) {
            return;
        }

        auto tile = latlon2tile(node.location().lat(), node.location().lon(), m_zoom);

        if (!m_tiles.empty() && !m_tiles.count(tile)) {
            return;
        }

        JSONFeature feature;
        feature.add_point(node);
        feature.add_tags(node.tags(), "_osm_node_id", node.id());
        feature.append_to(m_buffer);

        maybe_flush();
    }

    void way(const osmium::Way& way) {
        if (!m_tiles.empty()) {
            bool keep = false;
            for (auto ref : way.nodes()) {
                auto tile = latlon2tile(ref.location().lat(), ref.location().lon(), m_zoom);
                if (m_tiles.count(tile)) {
                    keep = true;
                    break;
                }
            }

            if (!keep) {
                return;
            }
        }

        try {
            {
                JSONFeature feature;
                feature.add_linestring(way);
                feature.add_tags(way.tags(), "_osm_way_id", way.id());
                feature.append_to(m_buffer);
            }

            if (m_create_polygons && way.is_closed()) {
                JSONFeature feature;
                feature.add_polygon(way);
                feature.add_tags(way.tags(), "_osm_way_id", way.id());
                feature.append_to(m_buffer);
            }

            maybe_flush();
        } catch (osmium::geometry_error&) {
            ++m_geometry_error_count;
        }
    }

    int geometry_error_count() const {
        return m_geometry_error_count;
    }

}; // class JSONHandler

/* ================================================== */

void print_help() {
    std::cout << "minjur [OPTIONS] [INFILE]\n\n" \
              << "If INFILE is not given stdin is assumed.\n" \
              << "Output is always to stdout.\n" \
              << "\nOptions:\n" \
              << "  -d, --dump                 Dump location store after run\n" \
              << "  -h, --help                 This help message\n" \
              << "  -l, --location_store=TYPE  Set location store\n" \
              << "  -p, --polygons             Create polygons from closed ways\n" \
              << "  -t, --tilefile=FILE        File with tiles to filter\n" \
              << "  -L                         See available location stores\n" \
              << "  -z, --zoom=ZOOM            Zoom level for tiles (default: 15)\n";
}

int main(int argc, char* argv[]) {
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    static struct option long_options[] = {
        {"dump",                 no_argument, 0, 'd'},
        {"help",                 no_argument, 0, 'h'},
        {"location_store",       required_argument, 0, 'l'},
        {"list_location_stores", no_argument, 0, 'L'},
        {"polygons",             no_argument, 0, 'p'},
        {"tilefile",             required_argument, 0, 't'},
        {"zoom",                 required_argument, 0, 'z'},
        {0, 0, 0, 0}
    };

    std::string location_store { "sparse_mem_array" };
    std::string tile_file_name;
    bool dump = false;
    bool create_polygons = false;
    int zoom = 15;

    while (true) {
        int c = getopt_long(argc, argv, "dhl:Lpt:z:", long_options, 0);
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
            case 't':
                tile_file_name = optarg;
                break;
            case 'p':
                create_polygons = true;
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
            case 'z':
                zoom = atoi(optarg);
                break;
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

    tileset_type tiles;
    if (!tile_file_name.empty()) {
        std::ifstream file(tile_file_name);
        if (! file.is_open()) {
            std::cerr << "can't open file file\n";
            exit(1);
        }
        unsigned int x;
        unsigned int y;
        while (file) {
            file >> x;
            file >> y;
            tiles.insert(std::make_pair(x, y));
        }
    }

    osmium::io::Reader reader(input_filename);

    std::unique_ptr<index_pos_type> index_pos = map_factory.create_map(location_store);
    location_handler_type location_handler(*index_pos);
    location_handler.ignore_errors();

    JSONHandler json_handler(create_polygons, tiles, zoom);

    osmium::apply(reader, location_handler, json_handler);
    reader.close();

    if (json_handler.geometry_error_count()) {
        std::cerr << "Geometry error count: " << json_handler.geometry_error_count() << "\n";
    }

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

