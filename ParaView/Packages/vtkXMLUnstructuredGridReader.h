/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredGridReader.h
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
// .NAME vtkXMLUnstructuredGridReader
// .SECTION Description
// vtkXMLUnstructuredGridReader

#ifndef __vtkXMLUnstructuredGridReader_h
#define __vtkXMLUnstructuredGridReader_h

#include "vtkXMLUnstructuredDataReader.h"

class vtkUnstructuredGrid;

class VTK_EXPORT vtkXMLUnstructuredGridReader : public vtkXMLUnstructuredDataReader
{
public:
  vtkTypeRevisionMacro(vtkXMLUnstructuredGridReader,vtkXMLUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);  
  static vtkXMLUnstructuredGridReader *New();
  
  // Description:
  // Get/Set the reader's output.
  void SetOutput(vtkUnstructuredGrid *output);
  vtkUnstructuredGrid *GetOutput();
  
protected:
  vtkXMLUnstructuredGridReader();
  ~vtkXMLUnstructuredGridReader();
  
  const char* GetDataSetName();
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel);
  void SetupOutputTotals();
  void SetupPieces(int numPieces);
  void DestroyPieces();
  
  void SetupOutputData();
  int ReadPiece(vtkXMLDataElement* ePiece);
  void SetupNextPiece();
  int ReadPieceData();
  
  // Read a data array whose tuples coorrespond to cells.
  int ReadArrayForCells(vtkXMLDataElement* da, vtkDataArray* outArray);
  
  // The index of the cell in the output where the current piece
  // begins.
  vtkIdType StartCell;
  
  // The Cells element for each piece.
  vtkXMLDataElement** CellElements;
  vtkIdType* NumberOfCells;
  
private:
  vtkXMLUnstructuredGridReader(const vtkXMLUnstructuredGridReader&);  // Not implemented.
  void operator=(const vtkXMLUnstructuredGridReader&);  // Not implemented.
};

#endif
