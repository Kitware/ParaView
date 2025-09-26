## Deprecate "process" in .pvx files

The "process" argument is now deprecated in .pvx files
and support for it will be removed in a later release.

Examples have been updated accordingly.

A trivial .pvx file looks like this:

```xml
<?xml version="1.0" ?>
<pvx>
  <Machine Name="localhost"
           Environment="DISPLAY=:0"
           Geometry="300x300+300+0"/>
</pvx>
```
