/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStructuredGridWriter.h
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
// .NAME vtkXMLStructuredGridWriter - Write rectilinear grid in an XML format.
// .SECTION Description
// vtkXMLStructuredGridWriter 

#ifndef __vtkXMLStructuredGridWriter_h
#define __vtkXMLStructuredGridWriter_h

#include "vtkXMLStructuredDataWriter.h"

class vtkStructuredGrid;

class VTK_EXPORT vtkXMLStructuredGridWriter : public vtkXMLStructuredDataWriter
{
public:
  static vtkXMLStructuredGridWriter* New();
  vtkTypeRevisionMacro(vtkXMLStructuredGridWriter,vtkXMLStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkStructuredGrid* input);
  vtkStructuredGrid* GetInput();
  
protected:
  vtkXMLStructuredGridWriter();
  ~vtkXMLStructuredGridWriter();  
  
  void WriteAppendedMode(vtkIndent indent);
  void WriteAppendedPiece(int index, vtkIndent indent);
  void WriteAppendedPieceData(int index);
  void WriteInlinePiece(int index, vtkIndent indent);
  void GetInputExtent(int* extent);
  const char* GetDataSetName();
  
  // The position of the appended data offset attribute for the points
  // array.
  unsigned long* PointsPosition;
  
private:
  vtkXMLStructuredGridWriter(const vtkXMLStructuredGridWriter&);  // Not implemented.
  void operator=(const vtkXMLStructuredGridWriter&);  // Not implemented.
};

#endif
