# Simulate anisotropic materials in the PBR represention

When using the PBR representation under the lighting panel, you can now
set the anisotropy strength and anisotropy rotation for a material.
The anisotropy strength controls the amount of light reflected along the
anisotropy direction (ie. the tangent). The anisotropy rotation rotate the
tangent around the normal. Notice that the object must have normals and tangents
defined to work. You can also use a texture to hold the anisotropy strength in
the red channel, and the anisotropy rotation in the green channel.
These parameters are supported by OSPRay pathtracer.
