
#include <osmium/geom/util.hpp>

#include <math.h>
#include "tile.hpp"

//const double M_PI = osmium::geom::PI / 2;

// https://github.com/mapbox/tippecanoe/blob/master/projection.c
// http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
std::pair<unsigned int, unsigned int>
latlon2tile(double lat, double lon, int zoom) {
    double lat_rad = lat * M_PI / 180;
    long long n = 1LL << zoom;

    long long llx = n * ((lon + 180) / 360);
    long long lly = n * (1 - (log(tan(lat_rad) + 1 / cos(lat_rad)) / M_PI)) / 2;

    if (lat >= 85.0511) {
        lly = 0;
    }
    if (lat <= -85.0511) {
        lly = n - 1;
    }

    if (llx < 0) {
        llx = 0;
    }
    if (lly < 0) {
        lly = 0;
    }
    if (llx >= n) {
        llx = n - 1;
    }
    if (lly >= n) {
        lly = n - 1;
    }

    return std::make_pair(llx, lly);
}

#if 0
// http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
void tile2latlon(unsigned int x, unsigned int y, int zoom, double *lat, double *lon) {
    unsigned long long n = 1LL << zoom;
    *lon = 360.0 * x / n - 180.0;
    float lat_rad = atan(sinh(M_PI * (1 - 2.0 * y / n)));
    *lat = lat_rad * 180 / M_PI;
}
#endif

