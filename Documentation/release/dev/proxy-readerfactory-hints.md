# Proxy XML for readers now supports multiple hints for file types

The <Hints> element in a reader proxy can now contain multiple entries to allow
a reader to suport differnt filetypes with differnt descriptions.

An example of this use case is in the ADIOS2CoreImageReader:
```xml
<Hints>
  <ReaderFactory extensions="bp"
                 file_description="ADIOS2 BP3 File (CoreImage)" />
  <ReaderFactory filename_patterns="md.idx"
                 file_description="ADIOS2 BP4 Metadata File (CoreImage)" />
  <ReaderFactory extensions="bp"
                 is_directory="1"
                 file_description="ADIOS2 BP4 Directory (CoreImage)" />
</Hints>
```
THis allows a single reader to present multiple entries in the "File -> Open"
dialog box rather than a single combined entry.
