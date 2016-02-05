
#include "json_feature.hpp"

void JSONFeature::add_properties(const osmium::OSMObject& object, const char* id_name) {
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

void JSONFeature::append_to(std::string& buffer) {
    m_writer.EndObject();

    buffer.append(m_stream.GetString(), m_stream.GetSize());
    buffer.append(1, '\n');
}

