## OpenFOAMReader: option to load restart files (filename ending with _0)

The `OpenFOAM` reader now allows result files which filename ends with `_0`. Previously, these files were ignored because "restart files" use the same naming pattern.
The reader still ignores these files by default to keep previous behavior.
