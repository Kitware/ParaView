/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredGridReader.h
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
// .NAME vtkXMLPStructuredGridReader
// .SECTION Description
// vtkXMLPStructuredGridReader

#ifndef __vtkXMLPStructuredGridReader_h
#define __vtkXMLPStructuredGridReader_h

#include "vtkXMLPStructuredDataReader.h"

class vtkStructuredGrid;

class VTK_IO_EXPORT vtkXMLPStructuredGridReader : public vtkXMLPStructuredDataReader
{
public:
  vtkTypeRevisionMacro(vtkXMLPStructuredGridReader,vtkXMLPStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPStructuredGridReader *New();
  
  // Description:
  // Get/Set the reader's output.
  void SetOutput(vtkStructuredGrid *output);
  vtkStructuredGrid *GetOutput();
  
protected:
  vtkXMLPStructuredGridReader();
  ~vtkXMLPStructuredGridReader();
  
  vtkStructuredGrid* GetPieceInput(int index);
  
  const char* GetDataSetName();
  void SetOutputExtent(int* extent);
  void GetPieceInputExtent(int index, int* extent);
  void SetupOutputInformation();
  void SetupOutputData();
  int ReadPieceData();
  vtkXMLDataReader* CreatePieceReader();
  
private:
  vtkXMLPStructuredGridReader(const vtkXMLPStructuredGridReader&);  // Not implemented.
  void operator=(const vtkXMLPStructuredGridReader&);  // Not implemented.
};

#endif
