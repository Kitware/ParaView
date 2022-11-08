## Configure Reader pipeline name

When loading a file, the pipeline source created used to be named according to the file, including extensions.
This is not always the best choice, so ParaView now check for `RegistrationName` information string property
in the reader proxy. If found, the pipeline name is the one given by the reader. Otherwise, it falls back
to the file name as it did before.
