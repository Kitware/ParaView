/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMaterialInterfaceIdList
 *
 * Class that facilitates efficient operation on
 * lists fragment ids. This class is introduced to
 * deal with the fact that local to global id search
 * is a constant time operation, while its inverse
 * glooabl to local id search is not.
*/

#ifndef vtkMaterialInterfaceIdList_h
#define vtkMaterialInterfaceIdList_h

#include "vtkMaterialInterfaceIdListItem.h"
#include "vtkPVVTKExtensionsFiltersMaterialInterfaceModule.h" //needed for exports

#include <vector> // needed for Initialize method

class VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT vtkMaterialInterfaceIdList
{
public:
  vtkMaterialInterfaceIdList();
  ~vtkMaterialInterfaceIdList();
  /**
   * Return the container to an empty state.
   */
  void Clear();
  /**
   * Initialize the container with a list of id's
   * these must be in ascending order.
   */
  void Initialize(const std::vector<int>& ids, bool preSorted = false);
  //@{
  /**
   * Given a global id, get the local id, or -1 if the
   * global id is not in the list.
   */
  int GetLocalId(int globalId);

private:
  bool IsInitialized;
  class IdListContainer;
  IdListContainer* IdList;
};
#endif
//@}

// VTK-HeaderTest-Exclude: vtkMaterialInterfaceIdList.h
