# Incubator modules

Modules in this directory are incubating towards stability. Any Python
wrappings appear under the `paraview.incubator` package. All API guarantees are
disclaimed here. Modules may appear or disappear without affecting ParaView's
stability guarantees.

## Internal Usage

Only plugins may require incubator modules.

## External Usage

From C++, the `INCUBATOR` component must be requested in order for these
modules to "exist":

```cmake
find_package(ParaView
  COMPONENTS
    INCUBATOR
    IncubatorModule)
```
