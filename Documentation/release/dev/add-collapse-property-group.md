## Add the possibility to collapse property groups in Properties, View and Display panels

It is now possible to collapse property groups in the different panels of ParaView (_View_, _Display_ and _Properties_).
By default, all property groups are expanded but some specific groups can be collapsed by some filters. A new setting **CollapsePropertyGroups** in **General Settings** is available to take precedence over the filter settings and expand or collapse all groups.
The properties are still displayed if we search for it, even if it is located in a collapsed group.

## Developer notes

When adding a new filter in ParaView, you can now add a parameter `collapse_panel` in the definition of the PropertyGroup in the `.xml` to set up a collapse state by default for a precise group.
