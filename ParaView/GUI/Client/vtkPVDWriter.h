/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDWriter - Wraps a VTK file writer.
// .SECTION Description
// vtkPVDWriter provides functionality for writers similar to that
// provided by vtkPVReaderModule for readers.  An instance of this
// class is configured by an XML ModuleInterface specification and
// knows how to create and use a single VTK file writer object.

#ifndef __vtkPVDWriter_h
#define __vtkPVDWriter_h

#include "vtkPVWriter.h"

class VTK_EXPORT vtkPVDWriter : public vtkPVWriter
{
public:
  static vtkPVDWriter* New();
  vtkTypeRevisionMacro(vtkPVDWriter,vtkPVWriter);
  void PrintSelf(ostream& os, vtkIndent indent);  
  
  // Description:
  // Check whether this writer supports the given VTK data set's type.
  virtual int CanWriteData(vtkDataObject* data, int parallel, int numParts);
  
  // Description:
  // Write the current source's data to the collection file with the
  // given name.
  void Write(const char* fileName, vtkPVSource* pvs, int numProcs,
             int ghostLevel, int timeSeries);
  
protected:
  vtkPVDWriter();
  ~vtkPVDWriter();
  
private:
  vtkPVDWriter(const vtkPVDWriter&); // Not implemented
  void operator=(const vtkPVDWriter&); // Not implemented
};

#endif
