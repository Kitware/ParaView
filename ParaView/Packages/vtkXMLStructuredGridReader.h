/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStructuredGridReader.h
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
// .NAME vtkXMLStructuredGridReader
// .SECTION Description
// vtkXMLStructuredGridReader

#ifndef __vtkXMLStructuredGridReader_h
#define __vtkXMLStructuredGridReader_h

#include "vtkXMLStructuredDataReader.h"

class vtkStructuredGrid;

class VTK_EXPORT vtkXMLStructuredGridReader : public vtkXMLStructuredDataReader
{
public:
  vtkTypeRevisionMacro(vtkXMLStructuredGridReader,vtkXMLStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLStructuredGridReader *New();
  
  // Description:
  // Get/Set the reader's output.
  void SetOutput(vtkStructuredGrid *output);
  vtkStructuredGrid *GetOutput();
  
protected:
  vtkXMLStructuredGridReader();
  ~vtkXMLStructuredGridReader();  
  
  const char* GetDataSetName();
  void SetOutputExtent(int* extent);
  
  void SetupPieces(int numPieces);
  void DestroyPieces();
  void SetupOutputInformation();
  void SetupOutputData();
  
  int ReadPiece(vtkXMLDataElement* ePiece);
  int ReadPieceData();
  
  // The elements representing the points for each piece.
  vtkXMLDataElement** PointElements;
  
private:
  vtkXMLStructuredGridReader(const vtkXMLStructuredGridReader&);  // Not implemented.
  void operator=(const vtkXMLStructuredGridReader&);  // Not implemented.
};

#endif
