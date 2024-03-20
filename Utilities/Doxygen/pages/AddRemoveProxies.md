# Adding and Removing Proxies      {#AddRemoveProxies}

## Introduction

The proxies name and label have a major role in ParaView :
they are used to create graphical and python interface.
This has a big impact on how the users will discover features
but it is also important at deprecation time.

## Naming
When adding a new proxy to ParaView you should give it a name (and label).
You should try to find a good name at first to make it discoverable
and also because it is not so easy to change it afterward (see below).

So here are some guidelines:

  1. make it concise: easier to read and remember
  2. make it meaningful: describe what it does (that other filters do not)
  3. avoid generic words: they can apply to too many things

For instance:
`AppendLocationAttributes` was renamed to `Coordinates`
and `GenerateProcessIds` simply to `ProcessIds`.

## XML attributes
The `<Proxy>` and the `<Property>` tags (and derivative, such as `SourceProxy` etc)
have two important attributes, `name` and `label`.

The `name` is used as an internal reference. It is the key for pvsm, the xml based statefile.

The `label` is used for user interfaces. This includes obviously the Graphical Interface,
through menus, but also the python wrapping.
In the python case, a sanitized version of the label is used, to be a valid function name.

If `label` is not defined, `name` is used instead.

The label is also translatable for the Graphical Interface (not for python).

## Deprecation
When a proxy is intended for deprecation (for a future removal for instance),
different things should be done, specially to ensure backward compatibility.

When a proxy is deprecated, the following files should probably be updated:
 * `*.xml`: the servermanager file declaring the proxy. Add a `<Deprecated>` tag with explanation.
 * `ParaViewFilters.xml`: to expose new proxy in ParaView.
 * `vtkSMStateVersionController.cxx` for pvsm backward compatibility. It uses proxy **name**.
 * `_backwardscompatibilityhelper.py` for python backward compatibility. It uses proxy **label**.
 * `Documentation/release/dev/<my-change>.md` to document the change in release note
 * `Testing/XML/*.xml`: the XML test(s) using this proxy
 * `Testing/Python/*.py`: python tests using this proxy

### Python
To enable backwards compatibility in a python script, use the following lines
at the top of the script:
```
import paraview
paraview.compatibility.major = 5
paraview.comptatbility.minor = 6
```
