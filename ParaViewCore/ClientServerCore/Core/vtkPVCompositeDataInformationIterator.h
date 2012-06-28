/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeDataInformationIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeDataInformationIterator - iterator used to iterate over
// data information for a composite data set.
// .SECTION Description
// vtkPVCompositeDataInformationIterator is an iterator used to iterate over
// data information for a composite data set.

#ifndef __vtkPVCompositeDataInformationIterator_h
#define __vtkPVCompositeDataInformationIterator_h

#include "vtkObject.h"

class vtkPVDataInformation;

class VTK_EXPORT vtkPVCompositeDataInformationIterator : public vtkObject
{
public:
  static vtkPVCompositeDataInformationIterator* New();
  vtkTypeMacro(vtkPVCompositeDataInformationIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the vtkPVDataInformation instance to iterate over. It is assumed
  // that the argument represents the data information for a composite dataset.
  void SetDataInformation(vtkPVDataInformation*);
  vtkGetObjectMacro(DataInformation, vtkPVDataInformation);

  // Description:
  // Initializes the traversal.
  void InitTraversal();

  // Description:
  // Returns true, if the iterator is finished with the traversal.
  bool IsDoneWithTraversal();

  // Description:
  // Goes to the next node.
  void GoToNextItem();

  // Description:
  // Returns the current nodes data-information. If current node is a piece
  // within a multi-piece dataset, then this will return NULL.
  vtkPVDataInformation* GetCurrentDataInformation();

  // Description:
  // Returns the name for the current node. Name may be valid only for a child
  // node. The root node has no name. Returns NULL when no name is provided.
  const char* GetCurrentName();

  // Description:
  // Returns the current flat index/composite index.
  // This is only valid is IsDoneWithTraversal() returns false.
  vtkGetMacro(CurrentFlatIndex, unsigned int);
//BTX
protected:
  vtkPVCompositeDataInformationIterator();
  ~vtkPVCompositeDataInformationIterator();

  unsigned int CurrentFlatIndex;
  vtkPVDataInformation* DataInformation;

private:
  vtkPVCompositeDataInformationIterator(const vtkPVCompositeDataInformationIterator&); // Not implemented
  void operator=(const vtkPVCompositeDataInformationIterator&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

