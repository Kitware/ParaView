<!-- Replace X.Y.Z with the ParaView version you are writing notes for -->

Instructions to create release notes

 Create release notes for ParaView X.Y.Z. The notes will be stored in a file named ParaView-X.Y.Z.md in the same directory as the file ParaView-X.Y.Z.md. We will create release notes by appending each .md file in the directory Documentation/release/dev into the ParaView-X.Y.Z.md file. Organize these notes based on how they fit into these categories: Rendering enhancements, Plugin updates, Filter changes, Changes in readers and writers, Interface improvements, Python scripting improvements. Virtual reality. Catalyst. Developer notes.

Some notes about the categories:

* "Rendering enhancements" covers changes involving rendering, widgets, views, or representations.
* "Plugin updates" covers changes in any note mentioning a plugin
* "Filter changes" covers changes in ParaView filters that are not plugins.
* "Changes in readers and writers" covers changes mentioning readers or writers.
* "Interface improvements" covers anything having to do with changes to the GUI, panels, windows, dialogs, etc.
* "Python scripting improvements" covers any changes mentioning Python.
* "Virtual reality" covers any changes mentioning the XRInterface or CAVEInteraction plugins. Any mention of CAVE should go into this category, even if the note mentions rendering or plugins.
* "Catalyst" covers any changes mentioning ParaView Catalyst or Catalyst.
* "Developer notes" covers any changes mentioning source code files and classes starting with vtk* or pq*. Build changes or CMake configuration changes also fall into this category.

Title the document "ParaView X.Y.Z Release Notes". Categories names should be first-level headers (#) consisting of the category name, and each release note below it should be a second-level header. Each
release note should start with a level-two subheader, but if it does not, change it to a second-level header. Be sure to copy all the body text from the .md files into the release notes, including quoted image references. Each note should be placed in only one category.

Add a table of contents with links to each category at the start of the document. Do not specifically label it as a table of contents.

General rules for formatting:

* proxy names should be marked up as **bold**. If unsure of whether a name is a proxy, look in the source code XML definition to verify that the name is in a "name" or "label" attribute ofthe XML. If still uncertain, ask for confirmation.
* source code class names in the ParaView repository or VTK submodule should be marked as up as `code` (text between backticks)
* user interface elements should be marked up as _italics_
* Refer to the file 00-sample-topic.md for other guidelines.

Edit the document for consistency in a few passes:

* Make all second-level headers past tense to indicate actions that have happened
* Body text tense should not be changed because it typically reflects the new and current state
