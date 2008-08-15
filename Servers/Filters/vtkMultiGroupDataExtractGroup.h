/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiGroupDataExtractGroup.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataExtractGroup - DEPRECATED!!!
// Will be removed in the next release. Provided for backward-compatibility
// between 3.2 and 3.4
// .SECTION Description
// vtkMultiGroupDataExtractGroup is mimicks the behaviour of
// vtkMultiGroupDataExtractGroup in ParaView 3.2. This is only for
// backward-compatibility between 3.2 and 3.4 and will be removed before the
// next release.
//
// .SECTION Original Description 
// vtkMultiGroupDataExtractGroup is a filter that extracts groups
// between user specified min and max.

#ifndef __vtkMultiGroupDataExtractGroup_h
#define __vtkMultiGroupDataExtractGroup_h

#include "vtkCompositeDataSetAlgorithm.h"

class VTK_EXPORT vtkMultiGroupDataExtractGroup : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkMultiGroupDataExtractGroup* New();
  vtkTypeRevisionMacro(vtkMultiGroupDataExtractGroup, vtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Minimum group to be extacted
  vtkSetMacro(MinGroup, unsigned int);
  vtkGetMacro(MinGroup, unsigned int);

  // Description:
  // Maximum group to be extacted
  vtkSetMacro(MaxGroup, unsigned int);
  vtkGetMacro(MaxGroup, unsigned int);

  // Description:
  // Sets the min and max groups
  void SetGroupRange(unsigned int min, unsigned int max)
    {
    this->SetMinGroup(min);
    this->SetMaxGroup(max);
    }
//BTX
protected:
  vtkMultiGroupDataExtractGroup();
  ~vtkMultiGroupDataExtractGroup();

  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

  unsigned int MinGroup;
  unsigned int MaxGroup;
private:
  vtkMultiGroupDataExtractGroup(const vtkMultiGroupDataExtractGroup&); // Not implemented
  void operator=(const vtkMultiGroupDataExtractGroup&); // Not implemented
//ETX
};

#endif

