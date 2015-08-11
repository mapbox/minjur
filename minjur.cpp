
#include <iostream>
#include <fstream>
#include <cassert>
#include <set>
#include <getopt.h>

#include <osmium/geom/geojson.hpp>
#include <osmium/geom/tile.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/geom/rapid_geojson.hpp>
#include <osmium/visitor.hpp>

// these must be include in this order
#include <osmium/index/map/all.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#pragma GCC diagnostic pop

typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;

typedef osmium::handler::NodeLocationsForWays<index_pos_type> location_handler_type;

typedef rapidjson::Writer<rapidjson::StringBuffer> writer_type;

typedef std::set<osmium::geom::Tile> tileset_type;

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

    void add_properties(const osmium::OSMObject& object, const char* id_name) {
        m_writer.String("properties");

        m_writer.StartObject();

        m_writer.String(id_name);
        m_writer.Int64(object.id());

        m_writer.String("_version");
        m_writer.Int(object.version());

        m_writer.String("_changeset");
        m_writer.Int(object.changeset());

        m_writer.String("_uid");
        m_writer.Int(object.uid());

        m_writer.String("_user");
        m_writer.String(object.user());

        m_writer.String("_timestamp");
        m_writer.Int(object.timestamp().seconds_since_epoch());

        for (const auto& tag : object.tags()) {
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
        m_geometry_error_count(0),
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

        osmium::geom::Tile tile(m_zoom, node.location());

        if (!m_tiles.empty() && !m_tiles.count(tile)) {
            return;
        }

        JSONFeature feature;
        feature.add_point(node);
        feature.add_properties(node, "_osm_node_id");
        feature.append_to(m_buffer);

        maybe_flush();
    }

    void way(const osmium::Way& way) {
        try {
            if (!m_tiles.empty()) {
                bool keep = false;
                for (auto ref : way.nodes()) {
                    osmium::geom::Tile tile(m_zoom, ref.location());
                    if (m_tiles.count(tile)) {
                        keep = true;
                        break;
                    }
                }

                if (!keep) {
                    return;
                }
            }

            {
                JSONFeature feature;
                feature.add_linestring(way);
                feature.add_properties(way, "_osm_way_id");
                feature.append_to(m_buffer);
            }

            if (m_create_polygons && way.is_closed()) {
                JSONFeature feature;
                feature.add_polygon(way);
                feature.add_properties(way, "_osm_way_id");
                feature.append_to(m_buffer);
            }
        } catch (osmium::geometry_error&) {
            ++m_geometry_error_count;
        } catch (osmium::invalid_location&) {
            ++m_geometry_error_count;
        }
        maybe_flush();
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
              << "  -d, --dump=FILE            Dump location cache to file after run\n" \
              << "  -h, --help                 This help message\n" \
              << "  -l, --location-store=TYPE  Set location store\n" \
              << "  -L, --list-location-stores Show available location stores\n" \
              << "  -n, --nodes=sparse|dense   Are node IDs sparse or dense?\n" \
              << "  -p, --polygons             Create polygons from closed ways\n" \
              << "  -t, --tilefile=FILE        File with tiles to filter\n" \
              << "  -z, --zoom=ZOOM            Zoom level for tiles (default: 15)\n";
}

tileset_type read_tiles_list(const std::string& filename) {
    tileset_type tiles;
    if (!filename.empty()) {
        std::ifstream file(filename);
        if (! file.is_open()) {
            std::cerr << "can't open file file\n";
            exit(1);
        }
        uint32_t z;
        uint32_t x;
        uint32_t y;
        while (file) {
            file >> z;
            file >> x;
            file >> y;
            tiles.emplace(z, x, y);
        }
    }
    return tiles;
}

int main(int argc, char* argv[]) {
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    static struct option long_options[] = {
        {"dump",                 required_argument, 0, 'd'},
        {"help",                       no_argument, 0, 'h'},
        {"location-store",       required_argument, 0, 'l'},
        {"list-location-stores",       no_argument, 0, 'L'},
        {"nodes",                required_argument, 0, 'n'},
        {"polygons",                   no_argument, 0, 'p'},
        {"tilefile",             required_argument, 0, 't'},
        {"zoom",                 required_argument, 0, 'z'},
        {0, 0, 0, 0}
    };

    std::string location_store;
    std::string locations_dump_file;
    std::string tile_file_name;
    bool create_polygons = false;
    int zoom = 15;
    bool nodes_dense = false;

    while (true) {
        int c = getopt_long(argc, argv, "d:hl:Ln:pt:z:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'd':
                locations_dump_file = optarg;
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
            case 'p':
                create_polygons = true;
                break;
            case 't':
                tile_file_name = optarg;
                break;
            case 'z':
                zoom = atoi(optarg);
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
    if (remaining_args > 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE]" << std::endl;
        exit(1);
    } else if (remaining_args == 1) {
        input_filename = argv[optind];
        std::cerr << "Reading from '" << input_filename << "'...\n";
    } else {
        input_filename = "-";
        std::cerr << "Reading from STDIN...\n";
    }

    tileset_type tiles { read_tiles_list(tile_file_name) };

    osmium::io::Reader reader(input_filename);

    std::unique_ptr<index_pos_type> index_pos = map_factory.create_map(location_store);
    location_handler_type location_handler(*index_pos);
    location_handler.ignore_errors();

    JSONHandler json_handler(create_polygons, tiles, zoom);

    osmium::apply(reader, location_handler, json_handler);
    reader.close();

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

