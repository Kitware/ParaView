## Add OpenStreetMap support in GeographicalMap plugin

You can now fetch OpenStreetMap (OSM) basemaps with the GeographicalMap plugin. In Bounding Box mode, the filter assembles a tile mosaic covering the requested latitude/longitude extent and georeferences the result using exact OSM tile edges, producing correct origin and spacing in degrees. The Zoom and Center mode preserves the previous single-tile behavior, and an attribution overlay is added to comply with OSM usage policy.
