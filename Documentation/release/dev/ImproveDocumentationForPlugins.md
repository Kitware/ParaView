## Improve documentation mecanism for ParaView plugins

Add 3 new CMake options to the `paraview_add_plugin` function :

 - **DOCUMENTATION_ADD_PATTERNS**: If specified, add patterns for the documentation files within `DOCUMENTATION_DIR` other than the default ones (i.e. `*.html`, `*.css`, `*.png`, `*.js` and `*.jpg`). This can be used to add new patterns (ex: `*.txt`) or even subdirectories (ex: `subDir/*.*`). Subdirectory hierarchy is kept so if you store all of your images in a `img/` sub directory and if your html file is at the root level of your documentation directory, then you should reference them using `<img src="img/my_image.png"/>` in the html file.
 - **DOCUMENTATION_TOC**: If specified, the function will use the given string to describe the table of content for the documentation. A TOC is diveded into sections. Every section point to a specific file (`ref` keyword) that is accessed when double-clicked in the UI. A section that contains other sections can be folded into the UI. An example of such a string is :
```html
<toc>
  <section title="Top level section title" ref="page1.html">
    <section title="Page Title 1" ref="page1.html"/>
    <section title="Sub section Title" ref="page2.html">
      <section title="Page Title 2" ref="page2.html"/>
      <section title="Page Title 3" ref="page3.html"/>
    </section>
  </section>
</toc>
```
 - **DOCUMENTATION_DEPENDENCIES**: Targets that are needed to be built before actually building the documentation. This can be useful when the plugin developer relies on a third party documentation generator like Doxygen for example.

See `Examples/Plugins/ElevationFilter` for an example of how to use these features.
