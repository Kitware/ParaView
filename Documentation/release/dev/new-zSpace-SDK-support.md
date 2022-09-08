## New zSpace SDK support

Make the zSpace plugin support the newer zSpace SDK (zSpace Core Compatibility API). \
This API is compatible with all currently supported zSpace hardware models, including the latest ones (like the Inspire).

The legacy SDK (zSpace Core API) remains supported as well. \
The choice of the SDK to use is done during the CMake Configuration step, by setting the ZSPACE_USE_COMPAT_SDK cache variable (true by default).
