/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredGridReader.h
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
// .NAME vtkXMLPUnstructuredGridReader
// .SECTION Description
// vtkXMLPUnstructuredGridReader

#ifndef __vtkXMLPUnstructuredGridReader_h
#define __vtkXMLPUnstructuredGridReader_h

#include "vtkXMLPUnstructuredDataReader.h"

class vtkUnstructuredGrid;

class VTK_IO_EXPORT vtkXMLPUnstructuredGridReader : public vtkXMLPUnstructuredDataReader
{
public:
  vtkTypeRevisionMacro(vtkXMLPUnstructuredGridReader,vtkXMLPUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);  
  static vtkXMLPUnstructuredGridReader *New();
  
  // Description:
  // Get/Set the reader's output.
  void SetOutput(vtkUnstructuredGrid *output);
  vtkUnstructuredGrid *GetOutput();
  
protected:
  vtkXMLPUnstructuredGridReader();
  ~vtkXMLPUnstructuredGridReader();
  
  const char* GetDataSetName();
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel);
  void SetupOutputTotals();
  
  void SetupOutputData();
  void SetupNextPiece();
  int ReadPieceData();
  
  void CopyArrayForCells(vtkDataArray* inArray, vtkDataArray* outArray);
  vtkXMLDataReader* CreatePieceReader();
  
  // The index of the cell in the output where the current piece
  // begins.
  vtkIdType StartCell;  
  
private:
  vtkXMLPUnstructuredGridReader(const vtkXMLPUnstructuredGridReader&);  // Not implemented.
  void operator=(const vtkXMLPUnstructuredGridReader&);  // Not implemented.
};

#endif
