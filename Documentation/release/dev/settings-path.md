## Site Settings

ParaView now supports a site settings file.
It uses the same syntax and has the same purpose as the `ParaView-<version>.ini` file
that can be found under the standard user configuration.
This site settings file is used as fallback.

The looked paths are relative to the ParaView installation and depends on
the operating system:
- the exectutable directory
- the <install> dir: the executable dir or its parent if executable is under a "bin" dir.
- "<install>/lib" "<install>/share/paraview-<version>"
- MacOs specific:
  -  package <root>: "<install>/../../.."
  - "lib", "lib-paraview-<version>", "Support" as <root> subdirs.
