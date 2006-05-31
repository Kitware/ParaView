/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectDataSets.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectDataSets - A group of datasets resulting from an AreaPick.
// .SECTION Description
// This class initially produces an empty MultiBlockDataSet. When AddDataSet
// is called, it tries to cast the pointer it is given into something that
// it knows how to extract a vtkDataSet from, and adds the DataSet to its
// MultiBlockDataSet output. vtkPVAllPick passes in Algorithms that 
// produce something that was rubber band selected on the screen for example.

#ifndef __vtkSelectDataSets_h
#define __vtkSelectDataSets_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkSelectDataSetsInternals;

class VTK_EXPORT vtkSelectDataSets : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSelectDataSets,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor
  static vtkSelectDataSets *New();

  // Description:
  // Empties the output.
  void Initialize();

  // Description:
  // Tries to obtain a dataset from the pointer, and adds the dataset to the
  // output.
  void AddDataSet(vtkObject *obj);

  // Description:
  // Returns the number of datasets that are in the output.
  int GetNumProps();  

  virtual int FillOutputPortInformation(int, vtkInformation *);

  virtual int RequestInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*);

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

protected:
  vtkSelectDataSets();
  ~vtkSelectDataSets();

private:
  vtkSelectDataSetsInternals* Internal;

  vtkSelectDataSets(const vtkSelectDataSets&);  // Not implemented.
  void operator=(const vtkSelectDataSets&);  // Not implemented.
};

#endif
