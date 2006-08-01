/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVEWriter - Wraps a VTK file writer.
// .SECTION Description
// vtkPVEWriter provides functionality for writing changes to a data file made by the Attribute Editor filter

#ifndef __vtkPVEWriter_h
#define __vtkPVEWriter_h

#include "vtkPVWriter.h"

class VTK_EXPORT vtkPVEWriter : public vtkPVWriter
{
public:
  static vtkPVEWriter* New();
  vtkTypeRevisionMacro(vtkPVEWriter,vtkPVWriter);
  void PrintSelf(ostream& os, vtkIndent indent);  

  
  // Description:
  // Check whether this writer supports the given VTK data set's type.
  virtual int CanWriteData(vtkDataObject* data, int parallel, int numParts);
  
protected:
  vtkPVEWriter();
  ~vtkPVEWriter();

  int WriteOneFile(const char* fileName, vtkPVSource* pvs,
                   int numProcs, int ghostLevel);

private:
  vtkPVEWriter(const vtkPVEWriter&); // Not implemented
  void operator=(const vtkPVEWriter&); // Not implemented
};

#endif
