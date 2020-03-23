# Python trace without rendering components

Python trace generation is an often used capability to understand
how to script ParaView pipelines using Python. These traces, however, can
get quite verbose. If you are not interested in the rendering parts of the script
and are only using the trace to setup the data processing pipeline, it is handy to not
including any code for the rendering components in the generated trace to minimize complexity.
To support that, we have added an ability to skip all rendering components including views, representations, color transfer functions, scalar bar etc. when generating Python trace or
saving Python state files. A checkbox, **Skip Rendering Components**, is now available on
the Python trace (or state) options dialog that you can check to skip all rendering components.
