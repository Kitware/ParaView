/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiGroupDataExtractDataSets.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataExtractDataSets - DEPRECATED!!!
// Will be removed in the next release. Provided for backward-compatibility
// between 3.2 and 3.4
// .SECTION Description
// vtkMultiGroupDataExtractDataSets is mimicks the behaviour of
// vtkMultiGroupDataExtractDataSets in ParaView 3.2. This is only for
// backward-compatibility between 3.2 and 3.4 and will be removed before the
// next release.
//
// .SECTION Original Description 
// vtkMultiGroupDataExtractDataSets extracts the user specified list
// of datasets from a multi-group dataset.

#ifndef __vtkMultiGroupDataExtractDataSets_h
#define __vtkMultiGroupDataExtractDataSets_h

#include "vtkCompositeDataSetAlgorithm.h"

class VTK_EXPORT vtkMultiGroupDataExtractDataSets : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkMultiGroupDataExtractDataSets* New();
  vtkTypeRevisionMacro(vtkMultiGroupDataExtractDataSets, vtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a dataset to be extracted.
  void AddDataSet(unsigned int group, unsigned int idx);

  // Description:
  // Remove all entries from the list of datasets to be extracted.
  void ClearDataSetList();
//BTX
protected:
  vtkMultiGroupDataExtractDataSets();
  ~vtkMultiGroupDataExtractDataSets();

  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

private:
  vtkMultiGroupDataExtractDataSets(const vtkMultiGroupDataExtractDataSets&); // Not implemented
  void operator=(const vtkMultiGroupDataExtractDataSets&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif

