/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTextSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTextSource - source that generates a 1x1 vtkTable with a single
// string data.
// .SECTION Description
// vtkPVTextSource is used to generate a table with a single string. 

#ifndef __vtkPVTextSource_h
#define __vtkPVTextSource_h

#include "vtkTableAlgorithm.h"

class VTK_EXPORT vtkPVTextSource : public vtkTableAlgorithm
{
public:
  static vtkPVTextSource* New();
  vtkTypeMacro(vtkPVTextSource, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the text string to generate in the output.
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);
  
// BTX
protected:
  vtkPVTextSource();
  ~vtkPVTextSource();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  char* Text;
private:
  vtkPVTextSource(const vtkPVTextSource&); // Not implemented
  void operator=(const vtkPVTextSource&); // Not implemented
//ETX
};

#endif

