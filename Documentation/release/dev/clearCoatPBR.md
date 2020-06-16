# Simulate clear coat layer in the PBR represention

When using the PBR representation under the lighting panel, you can now
add a second layer on top of the base layer. This is useful to simulate
a coating on top of a material. This layer is dielectric (as opposed with
metallic), and can be configured with various parameters. You can set the
coat strength to control the presence of the coat layer (1.0 means
strongest coating), the coat roughness and the coat color. You can
also choose the index of refraction of the coat layer as well as the
base layer. The more the index of refraction goes up, the more specular
reflections there are.
You can also use a texture to normal map the coating.
These parameters are supported by OSPRay pathtracer.
