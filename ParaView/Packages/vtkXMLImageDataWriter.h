/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLImageDataWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLImageDataWriter - Write image data in an XML format.
// .SECTION Description
// vtkXMLImageDataWriter 

#ifndef __vtkXMLImageDataWriter_h
#define __vtkXMLImageDataWriter_h

#include "vtkXMLStructuredDataWriter.h"

class vtkImageData;

class VTK_IO_EXPORT vtkXMLImageDataWriter : public vtkXMLStructuredDataWriter
{
public:
  static vtkXMLImageDataWriter* New();
  vtkTypeRevisionMacro(vtkXMLImageDataWriter,vtkXMLStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkImageData* input);
  vtkImageData* GetInput();
  
protected:
  vtkXMLImageDataWriter();
  ~vtkXMLImageDataWriter();  
  
  void WritePrimaryElementAttributes();
  void GetInputExtent(int* extent);
  const char* GetDataSetName();
  
private:
  vtkXMLImageDataWriter(const vtkXMLImageDataWriter&);  // Not implemented.
  void operator=(const vtkXMLImageDataWriter&);  // Not implemented.
};

#endif
