/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredGridWriter.h
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
// .NAME vtkXMLUnstructuredGridWriter - Write image data in an XML format.
// .SECTION Description
// vtkXMLUnstructuredGridWriter 

#ifndef __vtkXMLUnstructuredGridWriter_h
#define __vtkXMLUnstructuredGridWriter_h

#include "vtkXMLUnstructuredDataWriter.h"

class vtkUnstructuredGrid;

class VTK_EXPORT vtkXMLUnstructuredGridWriter : public vtkXMLUnstructuredDataWriter
{
public:
  vtkTypeRevisionMacro(vtkXMLUnstructuredGridWriter,vtkXMLUnstructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLUnstructuredGridWriter* New();
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkUnstructuredGrid* input);
  vtkUnstructuredGrid* GetInput();  
  
  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();
  
protected:
  vtkXMLUnstructuredGridWriter();
  ~vtkXMLUnstructuredGridWriter();  
  
  const char* GetDataSetName();
  void SetInputUpdateExtent(int piece, int numPieces, int ghostLevel);
 
  void WriteInlinePieceAttributes();
  void WriteInlinePiece(vtkIndent indent);
  
  void WriteAppendedMode(vtkIndent indent);
  void WriteAppendedPieceAttributes(int index);
  void WriteAppendedPiece(int index, vtkIndent indent);
  void WriteAppendedPieceData(int index);  
  
  // Positions of attributes for each piece.
  unsigned long* NumberOfCellsPositions;
  unsigned long** CellsPositions;
  
private:
  vtkXMLUnstructuredGridWriter(const vtkXMLUnstructuredGridWriter&);  // Not implemented.
  void operator=(const vtkXMLUnstructuredGridWriter&);  // Not implemented.
};

#endif
