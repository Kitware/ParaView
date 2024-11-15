## Delimited text reader improvements

The **DelimitedTextReader** (aka CSV reader) supports new options:
* A textual **Preview** of the first lines of the selected file. This is useful to be able to configure the reader before the first _apply_.
* The first **SkippedRecords** of the file are not parsed. This is useful as some file format may contain non standard header lines. See for instance the [gslib](http://www.gslib.com/gslib_help/format.html) format.
* first of **CommentsCharacters** found mark the end of the data for the current line: following characters are considered as comments and thus are not parsed.

The reader also befinits from different performance improvements: it runs faster and requires less memory during the parsing.
