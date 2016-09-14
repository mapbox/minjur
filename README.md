
# minjur

Osmium-based converter of OSM data to GeoJSON.

[![Build Status](https://secure.travis-ci.org/mapbox/minjur.png)](https://travis-ci.org/mapbox/minjur)

## Prerequisites

You need [libosmium](https://github.com/osmcode/libosmium) and Boost installed.


## Compile

To setup:

    mkdir build
    cd build
    cmake ..

Usually this should do the trick. If you need more configuration call

    ccmake .

Compile with

    make


## Run

Run like this:

    minjur [OPTIONS] OSMFILE >out.geojson

Call with `--help` to see command line options.

Options:

    -d, --dump=FILE            Dump location cache to file after run
    -e, --error-file=FILE      Write errors to file
    -h, --help                 This help message
    -i, --with-id              Add unique id to each feature
    -l, --location-store=TYPE  Set location store
    -L, --list-location-stores Show available location stores
    -n, --nodes=sparse|dense   Are node IDs sparse or dense?
    -p, --polygons             Create polygons from closed ways
    -t, --tilefile=FILE        File with tiles to filter
    -z, --zoom=ZOOM            Zoom level for tiles (default: 15)
    -a, --attr-prefix=PREFIX   Optional prefix for attributes, defaults to '@'

## Output

The output will be a (possibly rather large file) with one GeoJSON object
per line.

Nodes without any tags will not appear in the output. Ways with zero or a
single node will not appear in the output.

All tags will appear as GeoJSON properties, the object `type`, `id`, `version`,
`changeset`, `uid`, `user`, and `timestamp` will also be in the properties
using a leading `@` in the key indicating that this is an attribute, not a
tag. You can use the `--attr-prefix` or `-a` option to change this prefix.


## Working with updates

If you are working with a planet file or very large extract (a large continent)
set:

    export INDEX_TYPE=dense

If you are working with a small (city-sized) to medium (country-sized) extract
set:

    export INDEX_TYPE=sparse

Then run like this on the old OSM data file:

    minjur -d locations.dump -n ${INDEX_TYPE} OLD_OSMFILE >out.geojson

This will create a file `locations.dump` in addition to `out.geojson`. With
the change file (usually with suffix `.osc.gz`) you run the following to
create the tile list:

    minjur-generate-tilelist -l ${INDEX_TYPE}_file_array,locations.dump CHANGE_FILE >tiles.list

Then run `minjur` again with the tile list to create a GeoJSON file with only
the changes:

    minjur -d locations.dump -n ${INDEX_TYPE} -t tiles.list NEW_OSMFILE >changes.geojson

Repeat the last two lines for every change file.

For planet updates, you'll need at least 25GB RAM for the node location cache,
on OS/X and Windows it could be twice that!

## minjur-generate-tilelist

Run like this:

    minjur-generate-tilelist [OPTIONS] OSM-CHANGE-FILE

Output is always to stdout.

Options:

    -h, --help                 This help message
    -l, --location_store=TYPE  Set location store
    -L, --list-location-stores Show available location stores
    -n, --nodes=sparse|dense   Are node IDs sparse or dense?
    -z, --zoom=ZOOM            Zoom level for tiles (default: 15)

## Experimental version with multipolygon support

There is an experimental version called `minjur-mp` that has multipolygon
support. It does not support updates, that's why it doesn't have all the
options the `minjur` program has.

Run like this:

    minjur-mp [OPTIONS] OSMFILE >out.geojson

Call with `--help` to see command line options.

Options:

    -e, --error-file=FILE      Write errors to file
    -h, --help                 This help message
    -i, --with-id              Add unique id to each feature
    -l, --location-store=TYPE  Set location store
    -L, --list-location-stores Show available location stores
    -n, --nodes=sparse|dense   Are node IDs sparse or dense?
    -a, --attr-prefix=PREFIX   Optional prefix for attributes, defaults to '@'

The output will have GeoJSON objects for all the tagged nodes first and then,
in no particular order, the GeoJSON LineString objects (generated from ways)
and the MultiPolygon objects (generated from multipolygon relations and closed
ways).

Note that this version of the program will run much longer, generating the
multipolygons is rather slow.


## Name

This project is named after the town of Minjur in India which I know nothing
about.

https://www.openstreetmap.org/#map=15/13.2781/80.2530

