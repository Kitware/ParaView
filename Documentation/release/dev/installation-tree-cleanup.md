# Installation tree cleanup

On Unix, ParaView no longer installs its libraries under
`<prefix>/lib/paraview-${paraview-version}`. Instead, it follows the standard practice of
installing them under `<prefix>/lib` directly.

Python packages built by ParaView are now built and installed under
`<prefix>/lib/python-$<python-version>/site-packages` on \*nixes, and
`<prefix>/bin/Lib/site-packages` on Windows. This is consistent with how
Python installs its packages on the two platforms. In macOS app bundle, the location
in unchanged i.e. `<app>/Contents/Python`.
