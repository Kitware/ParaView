# Add a ShaderReplacements property to the Geometry representation

This new property allows the user to customize the OpenGL shaders by providing
some replacement strings for the shader produced by VTK. The replacements are
provided through a Json string like this:

```json
[
  {
    "type": "fragment",
    "original": "//VTK::Light::Impl",
    "replacement": "gl_FragData[0]=vec4(1.0,0.0,0.0,1.0);\n"
  },
  // other replacements can follow
]
```

In the UI, the property is represented with a custom widget that allows to load
Json from a file and save its path as a preset which is saved in the SMSettings
(thus in user's configuration file). It is possible to select a preset, remove
it from the list and edit the content of the Json string in a text edit widget.
