/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __vtkSQCellGenerator_h
#define __vtkSQCellGenerator_h

#include "vtkObject.h"

class vtkInformationObjectBaseKey;

/// Interface to sources that provide data on demand
/**

*/
class vtkSQCellGenerator : public vtkObject
{
public:
  /**
  Key that provides access to the source object.
  */
  static vtkInformationObjectBaseKey *CELL_GENERATOR();

  /**
  Return the total number of cells available.
  */
  virtual vtkIdType GetNumberOfCells()=0;

  /**
  Return the cell type of the cell at id. 
  */
  virtual int GetCellType(vtkIdType id)=0;

  /**
  Return the number of points required for the named
  cell. For homogeneous datasets its always the same.
  */
  virtual int GetNumberOfCellPoints(vtkIdType id)=0;

  /**
  Copy the points from a cell into the provided buffer,
  buffer is expected to be large enought to hold all of the
  points.
  */
  virtual int GetCellPoints(vtkIdType id, float *pts)=0;

  /**
  Copy the point's indexes into the provided bufffer,
  buffer is expected to be large enough. Return the 
  number of points coppied. The index is unique across
  all processes but is not the same as the point id
  in a VTK dataset.
  */
  virtual int GetCellPointIndexes(vtkIdType cid, vtkIdType *idx)=0;

public:
  vtkTypeRevisionMacro(vtkSQCellGenerator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);


protected:
  vtkSQCellGenerator(){}
  virtual ~vtkSQCellGenerator(){}

};

#endif
