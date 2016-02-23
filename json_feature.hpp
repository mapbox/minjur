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

struct attribute_names {

    std::string id;
    std::string type;
    std::string version;
    std::string changeset;
    std::string uid;
    std::string user;
    std::string timestamp;

    attribute_names(const std::string& prefix) :
        id(prefix + "id"),
        type(prefix + "type"),
        version(prefix + "version"),
        changeset(prefix + "changeset"),
        uid(prefix + "uid"),
        user(prefix + "user"),
        timestamp(prefix + "timestamp") {
    }

}; // struct attribute_names


using writer_type = rapidjson::Writer<rapidjson::StringBuffer>;

class JSONFeature {

    rapidjson::StringBuffer m_stream;
    writer_type m_writer;
    osmium::geom::RapidGeoJSONFactory<writer_type> m_factory;
    const attribute_names& m_attr_names;

public:

    JSONFeature(const attribute_names& attr_names) :
        m_stream(),
        m_writer(m_stream),
        m_factory(m_writer),
        m_attr_names(attr_names) {
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

    void add_id(const std::string& prefix, osmium::object_id_type id);

    void add_properties(const osmium::OSMObject& object);

    void append_to(std::string& buffer);

}; // class JSONFeature

