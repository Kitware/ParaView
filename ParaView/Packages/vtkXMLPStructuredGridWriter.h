/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredGridWriter.h
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
// .NAME vtkXMLPStructuredGridWriter - Write image data in a parallel XML format.
// .SECTION Description
// vtkXMLPStructuredGridWriter

#ifndef __vtkXMLPStructuredGridWriter_h
#define __vtkXMLPStructuredGridWriter_h

#include "vtkXMLPStructuredDataWriter.h"

class vtkStructuredGrid;

class VTK_IO_EXPORT vtkXMLPStructuredGridWriter : public vtkXMLPStructuredDataWriter
{
public:
  static vtkXMLPStructuredGridWriter* New();
  vtkTypeRevisionMacro(vtkXMLPStructuredGridWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkStructuredGrid* input);
  vtkStructuredGrid* GetInput();
  
protected:
  vtkXMLPStructuredGridWriter();
  ~vtkXMLPStructuredGridWriter();
  
  const char* GetDataSetName();
  vtkXMLStructuredDataWriter* CreateStructuredPieceWriter(); 
  
private:
  vtkXMLPStructuredGridWriter(const vtkXMLPStructuredGridWriter&);  // Not implemented.
  void operator=(const vtkXMLPStructuredGridWriter&);  // Not implemented.
};

#endif
