/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPRectilinearGridWriter.h
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
// .NAME vtkXMLPRectilinearGridWriter - Write image data in a parallel XML format.
// .SECTION Description
// vtkXMLPRectilinearGridWriter

#ifndef __vtkXMLPRectilinearGridWriter_h
#define __vtkXMLPRectilinearGridWriter_h

#include "vtkXMLPStructuredDataWriter.h"

class vtkRectilinearGrid;

class VTK_EXPORT vtkXMLPRectilinearGridWriter : public vtkXMLPStructuredDataWriter
{
public:
  static vtkXMLPRectilinearGridWriter* New();
  vtkTypeRevisionMacro(vtkXMLPRectilinearGridWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkRectilinearGrid* input);
  vtkRectilinearGrid* GetInput();
  
  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();
  
protected:
  vtkXMLPRectilinearGridWriter();
  ~vtkXMLPRectilinearGridWriter();
  
  const char* GetDataSetName();
  vtkXMLStructuredDataWriter* CreateStructuredPieceWriter(); 
  
private:
  vtkXMLPRectilinearGridWriter(const vtkXMLPRectilinearGridWriter&);  // Not implemented.
  void operator=(const vtkXMLPRectilinearGridWriter&);  // Not implemented.
};

#endif
