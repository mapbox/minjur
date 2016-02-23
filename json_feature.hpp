#pragma once

#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#pragma GCC diagnostic pop

#include <osmium/geom/rapid_geojson.hpp>
#include <osmium/osm.hpp>

using writer_type = rapidjson::Writer<rapidjson::StringBuffer>;

class JSONFeature {

    rapidjson::StringBuffer m_stream;
    writer_type m_writer;
    osmium::geom::RapidGeoJSONFactory<writer_type> m_factory;
    std::string m_attr_prefix;

public:

    JSONFeature(const std::string& attr_prefix) :
        m_stream(),
        m_writer(m_stream),
        m_factory(m_writer),
        m_attr_prefix(attr_prefix) {
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

    void add_multipolygon(const osmium::Area& area) {
        m_factory.create_multipolygon(area);
    }

    void add_properties(const osmium::OSMObject& object);

    void append_to(std::string& buffer);

}; // class JSONFeature

