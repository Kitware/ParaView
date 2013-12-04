/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __DataArrayCopier_h
#define __DataArrayCopier_h

#include "IdBlock.h" // for IdBlock

#include "vtkType.h"

class vtkDataArray;

/// Copy id ranges from one vtk data array to another of the same type.
/**
Copy id ranges from one vtk data array to another of the
same type. Abstract interface to templated data array copier.
This class hides the template types from the compiler so that
we can have stl collections of aggregate coppiers.
*/
class DataArrayCopier
{
public:
  virtual ~DataArrayCopier(){}

  /**
  Initialize the copier for transfers from the given array
  to a new array of the same type and name. Initializes
  both input and output arrays.
  */
  virtual void Initialize(vtkDataArray *in)=0;

  /**
  Set the array to copy from.
  */
  virtual void SetInput(vtkDataArray *in)=0;
  virtual vtkDataArray *GetInput()=0;

  /**
  Set the array to copy to.
  */
  virtual void SetOutput(vtkDataArray *out)=0;
  virtual vtkDataArray *GetOutput()=0;

  /**
  Copy the range from the input array to the end of then
  output array.
  */
  virtual void Copy(IdBlock &ids)=0;
  virtual void Copy(vtkIdType id)=0;
};

#endif

// VTK-HeaderTest-Exclude: DataArrayCopier.h
