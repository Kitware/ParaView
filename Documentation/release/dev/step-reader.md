## Updated STEP/IGES reader

ParaView now uses a rewritten STEP reader from VTK:

1. Reads should be much faster.
2. The output of the reader is a partitioned dataset collection with a data assembly.
   The assembly better reflects the CAD assembly structure because it does not
   produce *unnamed* blocks that the original reader produced; only nodes with a
   corresponding OpenCascade assembly label are added; unnamed labels have their
   named children added to their corresponding parents.
3. Field data now enumerates lists of assembly nodes that had to be renamed because
   sibling nodes were given the same name.
4. Colors/materials in STEP files should be more reliably read and available in ParaView.
   If a shape does not have color directly associated with it, the reader will see if the
   color is inherited from one of its ancestors. Also, transparency is now supported for
   imported colors.
5. The reader provides a **CanReadFile** method that allows ParaView to better test whether the
   reader will be able to load a file.
