## SaveData: Fix writer selection when multiple writers share the same extension

Previously, when multiple writers supported the same file extension, ParaView would always select the first available
writer, regardless of the user's choice in the file dialog. This caused the `IOSSWriter` to be used instead of the
`LegacyExodusIIWriter` when saving Exodus files, even when `LegacyExodusIIWriter` was explicitly selected.

This has been resolved by properly passing the selected writer from the `FileDialog` to
`vtkSMWriterFactory::CreateWriter`, ensuring the user's choice is respected.

Additionally, an issue has been fixed where `LegacyExodusIIWriter` (the plugin-based wrapper for `ExodusIIWriter`) was
not being instantiated properly, preventing it from being used even when selected.
