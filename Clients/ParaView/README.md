Making icon files
-----------------

# macOS

The files in `pvIcon.iconset` make up the set of icons used on macOS.
To make a new icon, replace the files in that directory with image
files of the same size. Then, in a terminal, run the maOS utility

```
iconutil -c icns pvIcon.iconset
```

This will produce a new file `pvIcon.icns`.
