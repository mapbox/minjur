
#include <cassert>
#include <unistd.h>

#include <osmium/osm.hpp>

#include "json_handler.hpp"

void JSONHandler::flush_to_output() {
    const auto written = write(1, m_buffer.data(), m_buffer.size());
    assert(written == long(m_buffer.size()));
    m_buffer.clear();
}

void JSONHandler::report_geometry_problem(const osmium::OSMObject& object, const char* error) {
    ++m_geometry_error_count;
    if (m_error_stream) {
        *m_error_stream << osmium::item_type_to_char(object.type()) << object.id() << ":" << error << "\n";
    }
}

