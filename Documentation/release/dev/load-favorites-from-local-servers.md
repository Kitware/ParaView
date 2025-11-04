## Load favorites from local servers

ParaView now provides a way to load all favorites accessible from any local server. This option
can be enable with the `Load All Favorites` in the General section in the Settings.

A small non retro-compatible change has been made in pqFileDialog::pqFileDialogFavoriteModelFileInfo
to be able to store the Origin. It's necessary as we want to be able to modify an existing favorite
from where it's located even when `Lod All Favorites` is enabled.
