## Node editor improvements

The Node editor now has a few more handy features.

### Saving / Restoring the pipeline layout alongside the state file

When saving a state file with ParaView, the node editor will save another file called `.XXX.pvne`, where `XXX` is the name of the state file without extension.
This file contains all necessary informations to restore the layout when opening the state file.
If `auto layout` is enabled, no file will be written nor looked for when saving / loading a state file.

### Annotations node

You can now annotate part of your pipeline using the new annotations feature (see screenshot below).
To create an annotation select a few filters and hit the `N` key (for **N**ote).
To select an annotation and make the description visible and editable, Ctrl-Click an annotation node.
To delete the annotation, hit Ctrl-N when the annotation is selected.

![node editor annotation example](node-editor-annotation.png)
