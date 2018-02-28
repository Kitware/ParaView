# Improve support of dependent components for volume representation

When volume representation is selected and **Map Scalars** and selected data
array has 2 or 4 components, a new property called **Mutliple Components Mapping**
is available.

When this new feature is enabled:
* for a two-components array, the first component is mapped to the color, the
  second component is mapped to the opacity.
* for a four-components array, the first, second and third components are used
  as RGB values and the fourth component is mapped to the opacity.

Note that:
* this feature is only available when **MapScalars** is *ON* ;
* this feature forces **Use Separate Color Map** ;
* it scales the color and the opacity range with the correct component.
