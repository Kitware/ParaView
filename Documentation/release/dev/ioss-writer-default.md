## IOSSWriter: The new default exodus writer

The `IOSSWriter` which was introduced in ParaView 5.12 is now the default Exodus writer in ParaView.
The old `ExodusIIWriter` is now available as `LegacyExodusIIWriter` once you load the `LegacyExodusWriter`
plugin. Finally, the `IOSSWriter` now supports the following extensions: `g e ex2 ex2v2 gen exoII exoii exo`.
