# Format specific options in save animation and screenshot dialogs

Dialogs for save-screenshot and save-animation now show file-format
specific options for quality, compression, etc. rather than a single **Image Quality**
parameter which was internally interpreted differently for each file format.
This allows the user finer controls based on the file format, in addition to avoiding
confusion.
