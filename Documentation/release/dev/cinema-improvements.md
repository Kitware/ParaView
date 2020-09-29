## Changes to Cinema support

Extractor introduced in this release also impact support for generation
of Cinema databases. Cinema Science project has undergone several changes over
the years with latest specification called Spec D, or just Cinema specification
for short. In this release, we simply ability to export Cinema specification by
integrating it with extractors.

Image Extractors such as PNG, JPEG now support generation of images from
multiple camera angles in addition the one used during the visualization setup.
By setting the **Camera Mode** to **Phi-Theta** on the **Properties** panel for
the extractor, you can make the extractor save out multiple
images per timestep, each done from a different camera position along a sphere
centered at the focal point with a radius set to the focal length.

Tools provided by the Cinema Science project are intended to be used to explore
artifacts produced by analysis apps like ParaView. To use those tools, you have
to generate a summary database for all generated extracts. To generate this
summary, simply check **Generate Cinema Specification** checkbox either when
using **Save Extracts** to generate extracts immediately in application or
when using **Save Catalyst State** to generate extracts in situ.

Cinema Science project is developing a new format for defining a composable
image set. This will support the ability of recoloring rendering results, and
combining multiple layers. Support for these composable-image-sets (CIS) will
be added in the future as a new type of Image Extractor.

Also removed in this release is the ability to import Cinema databases. The
importer was designed for legacy Cinema specifications and hence removed.
