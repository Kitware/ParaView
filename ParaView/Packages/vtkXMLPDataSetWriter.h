/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataSetWriter.h
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
// .NAME vtkXMLPDataSetWriter - write any type of vtk dataset to an XML file
// .SECTION Description
// vtkXMLPDataSetWriter 

#ifndef __vtkXMLPDataSetWriter_h
#define __vtkXMLPDataSetWriter_h

#include "vtkXMLPDataWriter.h"

class VTK_EXPORT vtkXMLPDataSetWriter : public vtkXMLPDataWriter
{
public:
  vtkTypeRevisionMacro(vtkXMLPDataSetWriter,vtkXMLPDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPDataSetWriter* New();
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkDataSet* input);
  vtkDataSet* GetInput();
  
  // Description:
  // Invoke the writer.  Returns 1 for success, 0 for failure.
  virtual int Write();
  
protected:
  vtkXMLPDataSetWriter();
  ~vtkXMLPDataSetWriter();
  
  // Dummies to satisfy pure virtuals from superclass.
  const char* GetDataSetName();
  vtkXMLWriter* CreatePieceWriter(int index);
  
private:
  vtkXMLPDataSetWriter(const vtkXMLPDataSetWriter&);  // Not implemented.
  void operator=(const vtkXMLPDataSetWriter&);  // Not implemented.
};

#endif
