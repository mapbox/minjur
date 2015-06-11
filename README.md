
# minjur

Osmium-based converter of OSM data to GeoJSON.


## Prerequisites

You need [libosmium](https://github.com/osmcode/libosmium) installed.


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

    minjur OSMFILE >out.geojson

Call with `--help` to see command line options.


## Working with updates

If you are working with a planet file or very large extract (a large continent)
set:

    export INDEX_TYPE=dense

If you are working with a small (city-sized) to medium (country-sized) extract
set:

    export INDEX_TYPE=sparse

On Linux set

    export INDEX_MAP=mmap

On OS/X or Windows set:

    export INDEX_MAP=map

Then run like this on the old OSM data file:

    minjur -d -l ${INDEX_TYPE}_${INDEX_MAP}_array OLD_OSMFILE >out.geojson

This will create a file `locations.dump` in addition to `out.geojson`. With
the change file (usually with suffix `.osc.gz`) you run the following to
create the tile list:

    minjur-generate-tilelist -l ${INDEX_TYPE}_file_array,locations.dump CHANGE_FILE >tiles.list

Then run `minjur` again with the tile list to create a GeoJSON file with only
the changes:

    minjur -d -l ${INDEX_TYPE}_${INDEX_MAP}_array -t tiles.list NEW_OSMFILE >changes.geojson

Repeat the last two lines for every change file.

For planet updates, you'll need at least 25GB RAM for the node location cache,
on OS/X and Windows it could be twice that!


## Name

This project is named after the town of Minjur in India which I know nothing
about.

https://www.openstreetmap.org/#map=15/13.2781/80.2530

