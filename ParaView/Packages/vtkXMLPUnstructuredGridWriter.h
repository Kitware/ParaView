/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredGridWriter.h
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
// .NAME vtkXMLPUnstructuredGridWriter - Write image data in a parallel XML format.
// .SECTION Description
// vtkXMLPUnstructuredGridWriter

#ifndef __vtkXMLPUnstructuredGridWriter_h
#define __vtkXMLPUnstructuredGridWriter_h

#include "vtkXMLPUnstructuredDataWriter.h"

class vtkUnstructuredGrid;

class VTK_EXPORT vtkXMLPUnstructuredGridWriter : public vtkXMLPUnstructuredDataWriter
{
public:
  static vtkXMLPUnstructuredGridWriter* New();
  vtkTypeRevisionMacro(vtkXMLPUnstructuredGridWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkUnstructuredGrid* input);
  vtkUnstructuredGrid* GetInput();
  
protected:
  vtkXMLPUnstructuredGridWriter();
  ~vtkXMLPUnstructuredGridWriter();
  
  const char* GetDataSetName();
  vtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter(); 
  
private:
  vtkXMLPUnstructuredGridWriter(const vtkXMLPUnstructuredGridWriter&);  // Not implemented.
  void operator=(const vtkXMLPUnstructuredGridWriter&);  // Not implemented.
};

#endif
