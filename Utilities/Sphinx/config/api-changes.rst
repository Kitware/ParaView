API Changes between ParaView versions
=====================================

Changes in 4.2
--------------

Changes to defaults
~~~~~~~~~~~~~~~~~~~
This version includes major overhaul of the ParaView's Python internals towards
one main goal: making the results consistent between ParaView UI and Python
applications e.g. when you create a RenderView, the defaults are setup
consistently irrespective of which interface you are using. Thus, scripts
generated from ParaView's trace which don't describe all property values may
produce different results from previous versions.


Choosing data array for scalar coloring
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
In previous versions, the recommended method for selecting an array to color
with was as follows:

    disp = GetDisplayProperties(...)
    disp.ColorArrayName = ("POINT_DATA", "Pressure")
    # OR
    disp.ColorArrayName = ("CELL_DATA", "Pressure")

However, scripts generated from ParaView's trace, would result in the following:

    disp.ColorArrayName = "Pressure"
    disp.ColorAttributeType = "POINT_DATA"

The latter is no longer supported i.e. ``ColorAttributeType`` property is no
longer available. You uses ColorArrayName to specify both the array
association and the array name, similar to the API for selecting arrays on
filters. Additionally, the attribute type strings ``POINT_DATA`` and
``CELL_DATA`` while still supported are deprecated. For consistency, you use
``POINTS``, ``CELLS``, etc. instead.

   disp.ColorArrayName = ("POINTS", "Pressure")
