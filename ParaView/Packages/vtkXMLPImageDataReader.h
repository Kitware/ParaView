/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPImageDataReader.h
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
// .NAME vtkXMLPImageDataReader
// .SECTION Description
// vtkXMLPImageDataReader

#ifndef __vtkXMLPImageDataReader_h
#define __vtkXMLPImageDataReader_h

#include "vtkXMLPStructuredDataReader.h"

class vtkImageData;

class VTK_IO_EXPORT vtkXMLPImageDataReader : public vtkXMLPStructuredDataReader
{
public:
  vtkTypeRevisionMacro(vtkXMLPImageDataReader,vtkXMLPStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPImageDataReader *New();
  
  // Description:
  // Get/Set the reader's output.
  void SetOutput(vtkImageData *output);
  vtkImageData *GetOutput();
  
protected:
  vtkXMLPImageDataReader();
  ~vtkXMLPImageDataReader();
  
  float Origin[3];
  float Spacing[3];
  
  vtkImageData* GetPieceInput(int index);
  
  const char* GetDataSetName();
  void SetOutputExtent(int* extent);
  void GetPieceInputExtent(int index, int* extent);
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary);
  void SetupOutputInformation();
  vtkXMLDataReader* CreatePieceReader();
  
private:
  vtkXMLPImageDataReader(const vtkXMLPImageDataReader&);  // Not implemented.
  void operator=(const vtkXMLPImageDataReader&);  // Not implemented.
};

#endif
