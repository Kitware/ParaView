/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDReader - ParaView-specific vtkXMLCollectionReader subclass
// .SECTION Description
// vtkPVDReader subclasses vtkXMLCollectionReader to add
// ParaView-specific methods.

#ifndef __vtkPVDReader_h
#define __vtkPVDReader_h

#include "vtkXMLCollectionReader.h"

class VTK_EXPORT vtkPVDReader : public vtkXMLCollectionReader
{
public:
  static vtkPVDReader* New();
  vtkTypeRevisionMacro(vtkPVDReader,vtkXMLCollectionReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the required value for the timestep attribute.  The value
  // should be referenced by its index.  Only data sets matching this
  // value will be read.  An out-of-range index will remove the
  // restriction.
  void SetTimestepAsIndex(int index);
  int GetTimestepAsIndex();

  // Description:
  // Get the range of index values valid for the TimestepAsIndex
  // setting.
  vtkGetVector2Macro(TimestepIndexRange, int);
protected:
  vtkPVDReader();
  ~vtkPVDReader();

  // Set TimestepIndexRange during ExecuteAttributes.
  virtual void ExecuteAttributes();

  // Save the range of valid timestep index values.
  int TimestepIndexRange[2];

private:
  vtkPVDReader(const vtkPVDReader&);  // Not implemented.
  void operator=(const vtkPVDReader&);  // Not implemented.
};

#endif
