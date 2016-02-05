
#include <iostream>
#include <fstream>
#include <cassert>
#include <set>
#include <getopt.h>

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

typedef std::set<osmium::geom::Tile> tileset_type;


class JSONHandler : public osmium::handler::Handler {

    std::string m_buffer;
    int m_geometry_error_count;
    bool m_create_polygons;
    tileset_type m_tiles;
    unsigned int m_zoom;
    std::unique_ptr<std::ofstream> m_error_stream;
    osmium::tags::KeyValueFilter m_filter;

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

    void report_geometry_problem(const osmium::Way& way, const char* error) {
        ++m_geometry_error_count;
        if (m_error_stream) {
            *m_error_stream << way.id() << ":" << error << "\n";
        }
    }

    std::pair<bool, bool> linestring_and_or_polygon(const osmium::Way& way) const {
        bool output_as_linestring = true;
        bool output_as_polygon = false;

        if (way.is_closed() && osmium::tags::match_any_of(way.tags(), m_filter)) {
            output_as_linestring = false;
            output_as_polygon = true;
        }

        return std::make_pair(output_as_linestring, output_as_polygon);
    }

public:

    JSONHandler(bool create_polygons, const tileset_type& tiles, unsigned int zoom, const std::string& error_file) :
        m_buffer(),
        m_geometry_error_count(0),
        m_create_polygons(create_polygons),
        m_tiles(tiles),
        m_zoom(zoom),
        m_error_stream(nullptr),
        m_filter(false) {

        m_filter.add(false, "aeroway", "gate");
        m_filter.add(false, "aeroway", "taxiway");
        m_filter.add(true, "aeroway");

        m_filter.add(false, "amenity", "atm");
        m_filter.add(false, "amenity", "bbq");
        m_filter.add(false, "amenity", "bench");
        m_filter.add(false, "amenity", "bureau_de_change");
        m_filter.add(false, "amenity", "clock");
        m_filter.add(false, "amenity", "drinking_water");
        m_filter.add(false, "amenity", "grit_bin");
        m_filter.add(false, "amenity", "parking_entrance");
        m_filter.add(false, "amenity", "post_box");
        m_filter.add(false, "amenity", "telephone");
        m_filter.add(false, "amenity", "vending_machine");
        m_filter.add(false, "amenity", "waste_basket");
        m_filter.add(true, "amenity");

        m_filter.add(true, "area");

        m_filter.add(true, "area:highway");

        m_filter.add(false, "building", "entrance");
        m_filter.add(false, "building", "no");
        m_filter.add(true, "building");

        m_filter.add(true, "craft");

        m_filter.add(false, "emergency", "fire_hydrant");
        m_filter.add(false, "emergency", "phone");
        m_filter.add(true, "emergency");

        m_filter.add(false, "golf", "hole");
        m_filter.add(true, "golf");

        m_filter.add(false, "historic", "boundary_stone");
        m_filter.add(true, "historic");

        m_filter.add(false, "junction", "roundabout");
        m_filter.add(true, "junction");

        m_filter.add(true, "landuse");

        m_filter.add(false, "leisure", "picnic_table");
        m_filter.add(false, "leisure", "track");
        m_filter.add(false, "leisure", "slipway");
        m_filter.add(true, "leisure");

        m_filter.add(false, "man_made", "cutline");
        m_filter.add(false, "man_made", "embankment");
        m_filter.add(false, "man_made", "flagpole");
        m_filter.add(false, "man_made", "mast");
        m_filter.add(false, "man_made", "petroleum_well");
        m_filter.add(false, "man_made", "pipeline");
        m_filter.add(false, "man_made", "survey_point");
        m_filter.add(true, "man_made");

        m_filter.add(true, "military");

        m_filter.add(false, "natural", "coastline");
        m_filter.add(false, "natural", "peak");
        m_filter.add(false, "natural", "saddle");
        m_filter.add(false, "natural", "spring");
        m_filter.add(false, "natural", "tree");
        m_filter.add(false, "natural", "tree_row");
        m_filter.add(false, "natural", "volcano");
        m_filter.add(true, "natural");

        m_filter.add(true, "office");

        m_filter.add(true, "piste:type");

        m_filter.add(true, "place");

        m_filter.add(false, "power", "line");
        m_filter.add(false, "power", "minor_line");
        m_filter.add(false, "power", "pole");
        m_filter.add(false, "power", "tower");
        m_filter.add(true, "power");

        m_filter.add(false, "public_transport", "stop_position");
        m_filter.add(true, "public_transport");

        m_filter.add(true, "shop");

        m_filter.add(false, "tourism", "viewpoint");
        m_filter.add(true, "tourism");

        m_filter.add(false, "waterway", "canal");
        m_filter.add(false, "waterway", "ditch");
        m_filter.add(false, "waterway", "drain");
        m_filter.add(false, "waterway", "river");
        m_filter.add(false, "waterway", "stream");
        m_filter.add(false, "waterway", "weir");
        m_filter.add(true, "waterway");

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

            std::pair<bool, bool> l_p = { true, false };
            if (m_create_polygons) {
                l_p =linestring_and_or_polygon(way);
            }

            if (l_p.first) { // output as linestring
                JSONFeature feature;
                feature.add_linestring(way);
                feature.add_properties(way, "_osm_way_id");
                feature.append_to(m_buffer);
            }

            if (l_p.second) { // output as polygon
                JSONFeature feature;
                feature.add_polygon(way);
                feature.add_properties(way, "_osm_way_id");
                feature.append_to(m_buffer);
            }
        } catch (osmium::geometry_error&) {
            report_geometry_problem(way, "geometry_error");
        } catch (osmium::invalid_location&) {
            report_geometry_problem(way, "invalid_location");
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
              << "  -e, --error-file=FILE      Write errors to file\n" \
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
        {"error-file",           required_argument, 0, 'e'},
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
    std::string error_file;
    std::string tile_file_name;
    bool create_polygons = false;
    int zoom = 15;
    bool nodes_dense = false;

    while (true) {
        int c = getopt_long(argc, argv, "d:e:hl:Ln:pt:z:", long_options, 0);
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

    JSONHandler json_handler(create_polygons, tiles, zoom, error_file);

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

