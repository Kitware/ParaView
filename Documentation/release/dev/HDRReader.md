## Expose HDR reader

ParaView now supports the reading of Radiance HDR files with a .hdr extension.
HDR images stand for High Dynamic Range images that are stored with 32 bits per channel.
HDR files can be opened directly in the pipeline browser, or as a background texture that can be used in PBR shading and OSPRay pathtracer.
These 32 bits background textures improves greatly the contrast of the rendering.
