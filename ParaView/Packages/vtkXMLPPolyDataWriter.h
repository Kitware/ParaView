/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPPolyDataWriter.h
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
// .NAME vtkXMLPPolyDataWriter - Write image data in a parallel XML format.
// .SECTION Description
// vtkXMLPPolyDataWriter

#ifndef __vtkXMLPPolyDataWriter_h
#define __vtkXMLPPolyDataWriter_h

#include "vtkXMLPUnstructuredDataWriter.h"

class vtkPolyData;

class VTK_EXPORT vtkXMLPPolyDataWriter : public vtkXMLPUnstructuredDataWriter
{
public:
  static vtkXMLPPolyDataWriter* New();
  vtkTypeRevisionMacro(vtkXMLPPolyDataWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkPolyData* input);
  vtkPolyData* GetInput();
  
  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();
  
protected:
  vtkXMLPPolyDataWriter();
  ~vtkXMLPPolyDataWriter();
  
  const char* GetDataSetName();
  vtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter(); 
  
private:
  vtkXMLPPolyDataWriter(const vtkXMLPPolyDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPPolyDataWriter&);  // Not implemented.
};

#endif
