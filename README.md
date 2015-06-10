
# minjur

Osmium-based converter of OSM data to GeoJSON.


## Prerequisites

You need [http://github.com/osmcode/libosmium](libosmium) installed.


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


## Name

This project is named after the town of Minjur in India which I know nothing
about.

https://www.openstreetmap.org/#map=15/13.2781/80.2530

