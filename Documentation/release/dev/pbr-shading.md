# Physically Based Rendering and normal mapping

ParaView now supports physically based rendering.

Under the `Lighting` properties group of the `Surface` representation, a new interpolation type
can be selected.\
When selected, new properties are available:
- Roughness: define how glossy a material is
- Metallic: if a material is metallic or not (you can set any value but for most realistic materials, it should be either 0 or 1)
- Base Color Texture: use a texture to define the color
- Material Texture: this texture encodes ambient **O**cclusion, **R**oughness, **M**etallic on the Red, Green, Blue channels respectively. This texture is also called ORM.
- Emissive Texture: this texture defines which parts of the model emit light.

This new interpolation properties are mapped to the OSPRay `Principled` material when the pathtracer is enabled.

PBR also supports reflections. Select your skybox (in equirectangular projection) and tick `Use As Environment Lighting`.

Normal mapping (also called Bump mapping) is now supported by both Gouraud and PBR interpolation. The texture can be selected with the `Normal Texture` property.\
This texture can be selected if the surface contains texture coordinates and tangents.
The tangents can be computed by the new filter `Generate Surface Tangents` (the surface must be triangulated and contain normals and texture coordinates)
