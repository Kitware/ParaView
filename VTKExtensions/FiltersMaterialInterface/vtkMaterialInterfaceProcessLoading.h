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
 * @class   vtkMaterialInterfaceProcessLoading
 *
 * Data type to represent a node in a multiprocess job
 * and its current loading state.
*/

#ifndef vtkMaterialInterfaceProcessLoading_h
#define vtkMaterialInterfaceProcessLoading_h

#include "vtkPVVTKExtensionsFiltersMaterialInterfaceModule.h" //needed for exports

#include "vtkType.h"
#include <cassert>
#include <iostream>
#include <vector>

class VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT vtkMaterialInterfaceProcessLoading
{
public:
  enum
  {
    ID = 0,
    LOADING = 1,
    SIZE = 2
  };
  //
  vtkMaterialInterfaceProcessLoading() { this->Initialize(-1, 0); }
  //
  ~vtkMaterialInterfaceProcessLoading() { this->Initialize(-1, 0); }
  //@{
  /**
   * Set the id and load factor.
   */
  void Initialize(int id, vtkIdType loadFactor)
  {
    this->Data[ID] = id;
    this->Data[LOADING] = loadFactor;
  }
  //@}
  /**
   * Comparison of two objects loading.
   */
  bool operator<(const vtkMaterialInterfaceProcessLoading& rhs) const
  {
    return this->Data[LOADING] < rhs.Data[LOADING];
  }
  /**
   * Comparison of two objects loading.
   */
  bool operator<=(const vtkMaterialInterfaceProcessLoading& rhs) const
  {
    return this->Data[LOADING] <= rhs.Data[LOADING];
  }
  /**
   * Comparison of two objects loading.
   */
  bool operator>(const vtkMaterialInterfaceProcessLoading& rhs) const
  {
    return this->Data[LOADING] > rhs.Data[LOADING];
  }
  /**
   * Comparison of two objects loading.
   */
  bool operator>=(const vtkMaterialInterfaceProcessLoading& rhs) const
  {
    return this->Data[LOADING] >= rhs.Data[LOADING];
  }
  /**
   * Comparison of two objects loading.
   */
  bool operator==(const vtkMaterialInterfaceProcessLoading& rhs) const
  {
    return this->Data[LOADING] == rhs.Data[LOADING];
  }
  /**
   * Return the process id.
   */
  vtkIdType GetId() const { return this->Data[ID]; }
  /**
   * Return the load factor.
   */
  vtkIdType GetLoadFactor() const { return this->Data[LOADING]; }
  //@{
  /**
   * Add to the load factor.
   */
  vtkIdType UpdateLoadFactor(vtkIdType loadFactor)
  {
    assert("Update would make loading negative." && (this->Data[LOADING] + loadFactor) >= 0);
    return this->Data[LOADING] += loadFactor;
  }

private:
  vtkIdType Data[SIZE];
};
VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT
std::ostream& operator<<(std::ostream& sout, const vtkMaterialInterfaceProcessLoading& fp);
VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT
std::ostream& operator<<(
  std::ostream& sout, const std::vector<vtkMaterialInterfaceProcessLoading>& vfp);
#endif
//@}

// VTK-HeaderTest-Exclude: vtkMaterialInterfaceProcessLoading.h
