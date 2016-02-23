#pragma once

#include <fstream>
#include <memory>
#include <unistd.h>

#include <osmium/handler.hpp>

#include "json_feature.hpp"

class JSONHandler : public osmium::handler::Handler {

    std::string m_buffer;
    attribute_names m_attr_names;
    std::unique_ptr<std::ofstream> m_error_stream;
    int m_geometry_error_count;
    bool m_with_id;

    void flush_to_output();

protected:

    std::string& buffer() noexcept {
        return m_buffer;
    }

    const attribute_names& attr_names() const noexcept {
         return m_attr_names;
    }

    bool with_id() const noexcept {
        return m_with_id;
    }

    void maybe_flush() {
        if (m_buffer.size() > 1024*1024) {
            flush_to_output();
        }
    }

    void report_geometry_problem(const osmium::OSMObject& object, const char* error);

    JSONHandler(const std::string& error_file, const std::string& attr_prefix, bool with_id) :
        m_buffer(),
        m_attr_names(attr_prefix),
        m_error_stream(nullptr),
        m_geometry_error_count(0),
        m_with_id(with_id) {
        if (!error_file.empty()) {
            m_error_stream.reset(new std::ofstream(error_file));
        }
    }

    ~JSONHandler() {
        flush_to_output();
    }

public:

    int geometry_error_count() const {
        return m_geometry_error_count;
    }

}; // class JSONHandler

