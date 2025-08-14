## Replace unsafe c from/to string functions

ParaView has been using a set of either unsafe or slow C/C++ functions to convert numbers to string or vice versa.
The exhaustive list of functions is given below. And all of them have been replaced with safer alternatives/faster
alternatives provided by scnlib, fmt, and fast_float libraries and exposed though the vtk:: namespace.

C/C++ has the following functions to convert one/many char or string to a number.

1. atof, atoi, atol, atoll,
2. std::stof, std::stod, std::stold, std::stoi, std::stol, std::stoll, std::stoul, std::stoull
3. std::strtof, std::strtod, std::strtold, std::strtol, std::strtoll/_strtoi64, std::strtoul,
   std::strtoull
4. sscanf, sscanf_s, vsscanf, vsscanf_s
5. std::from_chars (This is slow because it does not use fast_float under the hood)

These functions should be replaced by:

1. vtk::from_chars, vtk::scan_int, vtk::scan_value, if one number needs to be converted
2. vtk::scan, if one/many numbers need to be converted (optionally with a specific format)

C/C++ has the following functions to scan one/many numbers from a stdin/file.

1. scanf, scanf_s, vscanf, vscanf_s,
2. fscanf, fscanf_s, vfscanf, vfscanf_s,

These functions should be replaced by:

1. vtk::scan_value, if one number needs to be converted
2. vtk::input, vtk::scan, if one/many numbers need to be converted (optionally with a specific
   format)

C/C++ has the following functions to convert one/many numbers to a char or string.

1. itoa/_itoa, ltoa/_ltoa, lltoa/_i64toa, ultoa/_ultoa, ulltoa/_ulltoa/_ui64toa
2. sprintf, sprintf_s, vsprintf, vsprintf_s,
3. snprintf, snprintf_s, vsnprintf, vsnprintf_s,
4. strftime
5. std::to_chars, std::to_string

These functions should be replaced by:

1. vtk::to_chars or vtk::to_string, if one number needs to be converted
2. vtk::format, vtk::format_to, or vtk::format_to_n, if one/many
   numbers need to be converted with a specific format

C/C++ has the following functions to print one/many numbers to stdout/file.

1. printf, printf_s, vprintf, vprintf_s,
2. fprintf, fprintf_s, vfprintf, vfprintf_s,

These functions should be replaced by:

1. vtk::print, vtk::println

It should also be noted that the following functions (including subclasses that use them)
need to be provided with strings that use the std::format style format instead of the printf one:

1. `void vtkAnnotateGlobalDataFilter::SetFormat(const char* formatArg)`
2. `void vtkCGNSWriter::SetFileNameSuffix(const char* suffix)`
3. `void vtkContext2DScalarBarActor::SetRangeLabelFormat(const char* formatArg)`
4. `void vtkCSVWriter::SetFileNameSuffix(const char* suffix)`
5. `void vtkFileSeriesWriter::SetFileNameSuffix(const char* suffix)`
6. `void vtkParallelSerialWriter::SetFileNameSuffix(const char* suffix)`
7. `void vtkParticlePipeline::SetFilename(const char* filename)`
8. `void SceneImageWriterImageSeries::SetSuffixFormat(const char* suffix)`
9. `void vtkStringList::AddFormattedString(const char* EventString, T&&... args)`

Finally, the following string properties now use the std::format style format instead of the printf one:

1. `ScalarBarActor.LabelFormat`
2. `ScalarBarActor.RangeLabelFormat`
3. `TexturedScalarBarActor.LabelFormat`
4. `TexturedScalarBarActor.RangeLabelFormat`
5. `RulerSourceRepresentation.LabelFormat`
6. `ProtractorRepresentation.LabelFormat`
7. `ExodusIIReaderCore.FilePattern`
8. `PNGWriter.FilePattern`
9. `TIFFWriter.FilePattern`
10. `JPEGWriter.FilePattern`
11. `PolarAxesRepresentation.PolarLabelFormat`
12. `DateToNumeric.DateFormat`
13. `XYChartViewBase.TooltipLabelFormat`
14. `JPEG.SuffixFormat`
15. `PNG.SuffixFormat`
16. `TIFF.SuffixFormat`
17. `BPM.SuffixFormat`
18. `FileSeriesWriter.FileNameSuffix`
19. `FileSeriesWriterComposite.FileNameSuffix`
20. `ParallelFileSeriesWriter.FileNameSuffix`
21. `ParallelSerialWriter.FileNameSuffix`
22. `CGNSWriter.FileNameSuffix`
23. `CSVWriter.FileNameSuffix`
