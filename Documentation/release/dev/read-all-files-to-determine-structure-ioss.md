## IOSSReader: Add ReadAllFilesToDetermineStructure flag

IOSSReader now has a flag to read all files to determine the structure of the dataset.
When set to true (default), the reader will read all files to determine structure of the
dataset because some files might have certain blocks that other files don't have.
Set to false if you are sure that all files have the same structure, i.e. same blocks and sets.
When set to false, the reader will only read the first file to determine the structure.
which is faster than reading all files.
