/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __PolyDataCellCopier_h
#define __PolyDataCellCopier_h

#include "CellCopier.h"
#include "IdBlock.h"

class vtkCellArray;
class vtkFloatArray;

/// Copy geometry and data between unstructured datasets.
/**
Copies specific cells and associated geometry from/to a
pair of unstructured grid datasets.
*/
class PolyDataCellCopier : public CellCopier
{
public:
  PolyDataCellCopier()
        :
    SourcePts(0),
    SourceCells(0),
    OutPts(0),
    OutCells(0),
    CellType(NONE)
      {  }

  virtual ~PolyDataCellCopier();

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
  virtual int Copy(IdBlock &block);
  using CellCopier::Copy;

private:
  void ClearSource();
  void ClearOutput();

private:
  // source
  vtkFloatArray *SourcePts;
  vtkCellArray *SourceCells;
  // output
  vtkFloatArray *OutPts;
  vtkCellArray *OutCells;
  // cell type
  enum {
    NONE=0,
    POLY=1,
    VERT=2,
    STRIP=3,
    LINE=4
    };
  int CellType;
};

#endif
