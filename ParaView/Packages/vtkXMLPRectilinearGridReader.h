/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPRectilinearGridReader.h
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
// .NAME vtkXMLPRectilinearGridReader
// .SECTION Description
// vtkXMLPRectilinearGridReader

#ifndef __vtkXMLPRectilinearGridReader_h
#define __vtkXMLPRectilinearGridReader_h

#include "vtkXMLPStructuredDataReader.h"

class vtkRectilinearGrid;

class VTK_IO_EXPORT vtkXMLPRectilinearGridReader : public vtkXMLPStructuredDataReader
{
public:
  vtkTypeRevisionMacro(vtkXMLPRectilinearGridReader,vtkXMLPStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPRectilinearGridReader *New();
  
  // Description:
  // Get/Set the reader's output.
  void SetOutput(vtkRectilinearGrid *output);
  vtkRectilinearGrid *GetOutput();
  
protected:
  vtkXMLPRectilinearGridReader();
  ~vtkXMLPRectilinearGridReader();
  
  vtkRectilinearGrid* GetPieceInput(int index);
  
  const char* GetDataSetName();
  void SetOutputExtent(int* extent);
  void GetPieceInputExtent(int index, int* extent);
  void SetupOutputInformation();
  void SetupOutputData();
  int ReadPieceData();
  vtkXMLDataReader* CreatePieceReader();
  void CopySubCoordinates(int* inBounds, int* outBounds, int* subBounds,
                          vtkDataArray* inArray, vtkDataArray* outArray);
  
private:
  vtkXMLPRectilinearGridReader(const vtkXMLPRectilinearGridReader&);  // Not implemented.
  void operator=(const vtkXMLPRectilinearGridReader&);  // Not implemented.
};

#endif
