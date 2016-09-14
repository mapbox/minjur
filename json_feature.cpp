
#include <string>

#include <osmium/osm/tag.hpp>

#include "json_feature.hpp"

void JSONFeature::add_id(const std::string& prefix, osmium::object_id_type id) {
    m_writer.Key("id");
    m_writer.String(prefix + std::to_string(id));
}

void JSONFeature::add_properties(const osmium::OSMObject& object) {
    m_writer.Key("properties");

    m_writer.StartObject();

    m_writer.String(m_attr_names.id);
    m_writer.Int64(object.type() == osmium::item_type::area ? osmium::area_id_to_object_id(object.id()) : object.id());

    m_writer.String(m_attr_names.type);
    if (object.type() == osmium::item_type::area) {
        if (static_cast<const osmium::Area&>(object).from_way()) {
            m_writer.String("way");
        } else {
            m_writer.String("relation");
        }
    } else {
        m_writer.String(osmium::item_type_to_name(object.type()));
    }

    m_writer.String(m_attr_names.version);
    m_writer.Int(object.version());

    m_writer.String(m_attr_names.changeset);
    m_writer.Int(object.changeset());

    m_writer.String(m_attr_names.uid);
    m_writer.Int(object.uid());

    m_writer.String(m_attr_names.user);
    m_writer.String(object.user());

    m_writer.String(m_attr_names.timestamp);
    m_writer.Int(object.timestamp().seconds_since_epoch());

    for (const auto& tag : object.tags()) {
        m_writer.String(tag.key());
        m_writer.String(tag.value());
    }
    m_writer.EndObject();
}

void JSONFeature::append_to(std::string& buffer) {
    m_writer.EndObject();

    buffer.append(m_stream.GetString(), m_stream.GetSize());
    buffer.append(1, '\n');
}

