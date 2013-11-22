/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __UnstructuredGridCellCopier_h
#define __UnstructuredGridCellCopier_h

#include "CellCopier.h" // for CellCopier
#include "IdBlock.h" // for IdBlock

class vtkCellArray;
class vtkFloatArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;

/// Copy geometry and data between unstructured datasets.
/**
Copies specific cells and associated geometry from/to a
pair of unstructured grid datasets.
*/
class UnstructuredGridCellCopier : public CellCopier
{
public:
  UnstructuredGridCellCopier()
        :
    SourcePts(0),
    SourceCells(0),
    SourceTypes(0),
    OutPts(0),
    OutCells(0),
    OutTypes(0),
    OutLocs(0)
      {  }

  virtual ~UnstructuredGridCellCopier();

  /**
  Initialize the object with an input and an output. This
  will set up array copiers. Derived classes must make sure
  this method gets called if they override it.
  */
  virtual void Initialize(vtkDataSet *in, vtkDataSet *out);

  /**
  Free resources used by the object.
  */
  virtual void Clear();

  /**
  Copy a contiguous block of cells and their associated point
  and cell data from iniput to output.
  */
  virtual vtkIdType Copy(IdBlock &block);
  using CellCopier::Copy;


private:
  void ClearSource();
  void ClearOutput();

private:
  vtkFloatArray *SourcePts;
  vtkCellArray *SourceCells;
  vtkUnsignedCharArray *SourceTypes;

  vtkFloatArray *OutPts;
  vtkCellArray *OutCells;
  vtkUnsignedCharArray *OutTypes;
  vtkIdTypeArray *OutLocs;
};

#endif

// VTK-HeaderTest-Exclude: UnstructuredGridCellCopier.h
