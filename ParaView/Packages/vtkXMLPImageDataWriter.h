/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPImageDataWriter.h
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
// .NAME vtkXMLPImageDataWriter - Write image data in a parallel XML format.
// .SECTION Description
// vtkXMLPImageDataWriter

#ifndef __vtkXMLPImageDataWriter_h
#define __vtkXMLPImageDataWriter_h

#include "vtkXMLPStructuredDataWriter.h"

class vtkImageData;

class VTK_IO_EXPORT vtkXMLPImageDataWriter : public vtkXMLPStructuredDataWriter
{
public:
  static vtkXMLPImageDataWriter* New();
  vtkTypeRevisionMacro(vtkXMLPImageDataWriter,vtkXMLPStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkImageData* input);
  vtkImageData* GetInput();
  
protected:
  vtkXMLPImageDataWriter();
  ~vtkXMLPImageDataWriter();
  
  const char* GetDataSetName();
  void WritePrimaryElementAttributes();
  vtkXMLStructuredDataWriter* CreateStructuredPieceWriter(); 
  
private:
  vtkXMLPImageDataWriter(const vtkXMLPImageDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPImageDataWriter&);  // Not implemented.
};

#endif
