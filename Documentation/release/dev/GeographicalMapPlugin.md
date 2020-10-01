## Geographical Map Plugin

We introduced a new plugin that allows geographical map fetch
from two different providers: *GoogleMap* and *MapQuest*.

In order to fetch the image, the user can use a new source called `GeoMapFetcher`.\
This source can be configured to fetch a map with a specific coordinate center
and a zoom level.\
The dimension of the image can be specified as well and the output is a 2D image
correctly positioned in latitude and longitude.

A filter called `vtkGeoMapConvertFilter` is provided to convert this image to a
structured grid with a different projection supported by PROJ library.

In order to build this plugin, CURL library is used as an external dependency.
