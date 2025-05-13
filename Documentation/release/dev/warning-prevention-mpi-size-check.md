# Warning prevention due to VTK MPI size check fix

Some changes were made in ParaView to prevent warning messages when using the application with MPI as well.

1. VTK MPI has been reworked to fix a potential freeze of the application when using ParaView with MPI. The new fix can throw a warning if some functions are not used properly.
2. Some minor refactoring were made improve the memory management.
