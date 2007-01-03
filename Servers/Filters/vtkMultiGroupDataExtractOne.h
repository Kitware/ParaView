/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataExtractOne.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataExtractOne - extract one dataset
// .SECTION Description
// vtkMultiGroupDataExtractOne extracts the first simple dataset
// from a multi-group dataset. Use this filter before a filter
// that does not work with composite datasets. This filter is a
// temporary workaround for ParaView 2.6.

#ifndef __vtkMultiGroupDataExtractOne_h
#define __vtkMultiGroupDataExtractOne_h

#include "vtkDataSetAlgorithm.h"

class VTK_EXPORT vtkMultiGroupDataExtractOne : public vtkDataSetAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkMultiGroupDataExtractOne,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkMultiGroupDataExtractOne *New();

protected:
  vtkMultiGroupDataExtractOne();
  ~vtkMultiGroupDataExtractOne();

  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);
  virtual int RequestInformation (vtkInformation *, vtkInformationVector **, 
                                  vtkInformationVector *);
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual vtkExecutive* CreateDefaultExecutive();

private:
  vtkMultiGroupDataExtractOne(const vtkMultiGroupDataExtractOne&);  // Not implemented.
  void operator=(const vtkMultiGroupDataExtractOne&);  // Not implemented.
};

#endif


