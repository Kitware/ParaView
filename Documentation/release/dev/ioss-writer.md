# IOSS writer for Exodus files

An initial version of IOSS writer that uses the IOSS library to write Exodus
files is now available. The writer is used when writing files with extensions
`.exo`. The writer is not fully functional yet, for example, it does not support
serializing other entity blocks / sets besides element blocks, node sets
and side sets. Once these capabilities are added and the writer goes though some
aggressive testing, this will replace the Exodus writer for all Exodus files.
Until then, the current version should be treated as an beta-release.
