## Improved directory mode for save file-dialog

When you save files into directories, ParaView presents a much cleaner and easy to understand dialog to select the output directory.

When the file dialog is in directory mode (`pqFileDialog::Directory`):
1. The `File name` label has been renamed to `Directory name` and the unnecessary `Files of type` combo box
does not appear.
2. Although the files are visible, they are neither selectable nor editable (rename). This is useful for when you want to see the saved files. (ex: extracts)

### API Changes:
- The `pqFileDialogModel::isDir()` now has a `const` qualifier since it is used to set the appropriate flags for directories and files in the tree view. `QAbstractItemModel::flags()` is const qualified.

![Improved directory mode for save file-dialog](../img/dev/save-file-dialog-directory-mode.png "Improved directory mode for save file-dialog")
