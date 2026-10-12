## Customized filter categories follow application and plugin changes

Only the changes made using *Tools > Configure Filter Categories* are now saved in the settings, and applied over the categories defined by ParaView and plugins. Previously, a copy of all categories was saved, so categories added or modified afterwards, e.g. by plugins loaded at startup, were not shown for users who customized categories. Customizations saved by previous versions are merged into the categories, and categories removed with previous versions are shown again.

### Developer notes

`pqProxyCategory::writeSettings(resourceTag)` and `pqProxyCategory::loadSettings(resourceTag)` are deprecated in favor of overloads also taking the base categories, the customizations being saved as differences with them.
