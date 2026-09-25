## Connection dialog improvements

Server configuration files (.pvsc) can now group related options together in the connection options dialog using an `<OptionGroup>` tag. Each group appears as a collapsible section, with a label and an expand/collapse arrow icon, containing the `<Option>`s listed in the group; options not listed in any group are shown below the groups. An `<OptionGroup>`'s optional `collapsed` attribute controls whether the group starts collapsed or expanded (the default).

Server configuration files can also include descriptive Markdown text in the connection options dialog using a `<Documentation>` tag:

* As the first child of `<Options>`, the text is shown at the top of the dialog.
* As the first child of an `<Option>`, the text is shown as a tooltip on that option's control.
* As the first child of an `<OptionGroup>`, the text is shown just below the group's label.

Finally, the connection options dialog no longer grows arbitrarily large to accommodate its content, for example when several `<OptionGroup>`s are expanded at once. Instead, it resizes itself, when first shown, to fit its content without a scroll bar whenever that content fits on screen; otherwise, its content scrolls while the **OK** and **Cancel** buttons remain visible at the bottom of the dialog.
