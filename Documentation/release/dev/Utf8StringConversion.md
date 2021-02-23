## Force QString conversion to/from Utf8 bytes

The practice of converting QString data to a user's current locale has been replaced by an explicit conversion to Utf8 encoding. This change integrates neatly with VTK's utf8 everywhere policy and is in line with Qt5 string handling, whereby C++ strings/char* are assumed to be utf8 encoded. Any legacy text files containing extended character sets should be saved as utf8 documents, in order to load them in the latest version of Paraview.

## Use wide string Windows API for directory listings

Loading paths and file names containing non-ASCII characters on Windows is now handled via the wide string API and uses vtksys for the utf8 <-> utf16 conversions. Thus the concept of converting text to the system's current locale has been completely eliminated.
