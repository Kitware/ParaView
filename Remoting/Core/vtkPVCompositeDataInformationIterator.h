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
/**
 * @class   vtkPVCompositeDataInformationIterator
 * @brief   iterator used to iterate over
 * data information for a composite data set.
 *
 * vtkPVCompositeDataInformationIterator is an iterator used to iterate over
 * data information for a composite data set.
*/

#ifndef vtkPVCompositeDataInformationIterator_h
#define vtkPVCompositeDataInformationIterator_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class vtkPVDataInformation;

class VTKREMOTINGCORE_EXPORT vtkPVCompositeDataInformationIterator : public vtkObject
{
public:
  static vtkPVCompositeDataInformationIterator* New();
  vtkTypeMacro(vtkPVCompositeDataInformationIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the vtkPVDataInformation instance to iterate over. It is assumed
   * that the argument represents the data information for a composite dataset.
   */
  void SetDataInformation(vtkPVDataInformation*);
  vtkGetObjectMacro(DataInformation, vtkPVDataInformation);
  //@}

  /**
   * Initializes the traversal.
   */
  void InitTraversal();

  /**
   * Returns true, if the iterator is finished with the traversal.
   */
  bool IsDoneWithTraversal();

  /**
   * Goes to the next node.
   */
  void GoToNextItem();

  /**
   * Returns the current nodes data-information. If current node is a piece
   * within a multi-piece dataset, then this will return NULL.
   */
  vtkPVDataInformation* GetCurrentDataInformation();

  /**
   * Returns the name for the current node. Name may be valid only for a child
   * node. The root node has no name. Returns NULL when no name is provided.
   */
  const char* GetCurrentName();

  //@{
  /**
   * Returns the current flat index/composite index.
   * This is only valid is IsDoneWithTraversal() returns false.
   */
  vtkGetMacro(CurrentFlatIndex, unsigned int);
  //@}

protected:
  vtkPVCompositeDataInformationIterator();
  ~vtkPVCompositeDataInformationIterator() override;

  unsigned int CurrentFlatIndex;
  vtkPVDataInformation* DataInformation;

private:
  vtkPVCompositeDataInformationIterator(const vtkPVCompositeDataInformationIterator&) = delete;
  void operator=(const vtkPVCompositeDataInformationIterator&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
