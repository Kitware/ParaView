// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
  ///@{
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
