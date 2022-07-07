## Fix catalyst deadlock if not used on all MPI ranks

Fix a bug that causes a deadlock if catalyst is not used on all MPI ranks.
This MR fixes the issue: https://gitlab.kitware.com/paraview/paraview/-/issues/21414
