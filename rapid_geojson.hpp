#ifndef RAPID_GEOJSON_HPP
#define RAPID_GEOJSON_HPP

#include <cassert>
#include <string>
#include <utility>

#include <osmium/geom/coordinates.hpp>
#include <osmium/geom/factory.hpp>

namespace osmium {

    namespace geom {

        namespace detail {

            template <class TWriter>
            class RapidGeoJSONFactoryImpl {

                TWriter& m_writer;

            public:

                typedef void point_type;
                typedef void linestring_type;
                typedef void polygon_type;
                typedef void multipolygon_type;
                typedef void ring_type;

                RapidGeoJSONFactoryImpl(TWriter& writer) :
                    m_writer(writer) {
                }

                /* Point */

                // { "type": "Point", "coordinates": [100.0, 0.0] }
                point_type make_point(const osmium::geom::Coordinates& xy) const {
                    m_writer.String("geometry");
                    m_writer.StartObject();
                    m_writer.String("type");
                    m_writer.String("Point");
                    m_writer.String("coordinates");
                    m_writer.StartArray();
                    m_writer.Double(xy.x);
                    m_writer.Double(xy.y);
                    m_writer.EndArray();
                    m_writer.EndObject();
                }

                /* LineString */

                // { "type": "LineString", "coordinates": [ [100.0, 0.0], [101.0, 1.0] ] }
                void linestring_start() {
                    m_writer.String("geometry");
                    m_writer.StartObject();
                    m_writer.String("type");
                    m_writer.String("LineString");
                    m_writer.String("coordinates");
                    m_writer.StartArray();
                }

                void linestring_add_location(const osmium::geom::Coordinates& xy) {
                    m_writer.StartArray();
                    m_writer.Double(xy.x);
                    m_writer.Double(xy.y);
                    m_writer.EndArray();
                }

                linestring_type linestring_finish(size_t /* num_points */) {
                    m_writer.EndArray();
                    m_writer.EndObject();
                }

                /* MultiPolygon */

                void multipolygon_start() {
                    m_writer.String("geometry");
                    m_writer.StartObject();
                    m_writer.String("type");
                    m_writer.String("MultiPolygon");
                    m_writer.String("coordinates");
                    m_writer.StartArray();
                }

                void multipolygon_polygon_start() {
                    m_writer.StartArray();
                }

                void multipolygon_polygon_finish() {
                    m_writer.EndArray();
                }

                void multipolygon_outer_ring_start() {
                    m_writer.StartArray();
                }

                void multipolygon_outer_ring_finish() {
                    m_writer.EndArray();
                }

                void multipolygon_inner_ring_start() {
                    m_writer.StartArray();
                }

                void multipolygon_inner_ring_finish() {
                    m_writer.EndArray();
                }

                void multipolygon_add_location(const osmium::geom::Coordinates& xy) {
                    m_writer.StartArray();
                    m_writer.Double(xy.x);
                    m_writer.Double(xy.y);
                    m_writer.EndArray();
                }

                multipolygon_type multipolygon_finish() {
                    m_writer.EndArray();
                    m_writer.EndObject();
                }

            }; // class RapidGeoJSONFactoryImpl

        } // namespace detail

        template <class TWriter, class TProjection = IdentityProjection>
        using RapidGeoJSONFactory = GeometryFactory<detail::RapidGeoJSONFactoryImpl<TWriter>, TProjection>;

    } // namespace geom

} // namespace osmium

#endif // RAPID_GEOJSON_HPP
