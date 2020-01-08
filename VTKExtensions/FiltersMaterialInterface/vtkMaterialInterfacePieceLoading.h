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
 * @class   vtkMaterialInterfacePieceLoading
 *
 * Data structure that describes a fragment's loading.
 * Holds its id and its loading factor.
*/

#ifndef vtkMaterialInterfacePieceLoading_h
#define vtkMaterialInterfacePieceLoading_h

#include "vtkPVVTKExtensionsFiltersMaterialInterfaceModule.h" //needed for exports
#include "vtkSystemIncludes.h"

#include "vtkType.h"
#include <cassert>
#include <iostream>
#include <vector>

class VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT vtkMaterialInterfacePieceLoading
{
public:
  enum
  {
    ID = 0,
    LOADING = 1,
    SIZE = 2
  };
  vtkMaterialInterfacePieceLoading() { this->Initialize(-1, 0); }
  ~vtkMaterialInterfacePieceLoading() { this->Initialize(-1, 0); }
  void Initialize(int id, vtkIdType loading)
  {
    this->Data[ID] = id;
    this->Data[LOADING] = loading;
  }
  //@{
  /**
   * Place into a buffer (id, loading)
   */
  void Pack(vtkIdType* buf)
  {
    buf[ID] = this->Data[ID];
    buf[LOADING] = this->Data[LOADING];
  }
  //@}
  //@{
  /**
   * Initialize from a buffer (id, loading)
   */
  void UnPack(vtkIdType* buf)
  {
    this->Data[ID] = buf[ID];
    this->Data[LOADING] = buf[LOADING];
  }
  //@}
  /**
   * Set/Get
   */
  vtkIdType GetId() const { return this->Data[ID]; }
  vtkIdType GetLoading() const { return this->Data[LOADING]; }
  void SetLoading(vtkIdType loading) { this->Data[LOADING] = loading; }
  //@{
  /**
   * Adds to loading and returns the updated loading.
   */
  vtkIdType UpdateLoading(vtkIdType update)
  {
    assert("Update would make loading negative." && (this->Data[LOADING] + update) >= 0);
    return this->Data[LOADING] += update;
  }
  //@}
  //@{
  /**
   * Comparison are made by id.
   */
  bool operator<(const vtkMaterialInterfacePieceLoading& other) const
  {
    return this->Data[ID] < other.Data[ID];
  }
  bool operator==(const vtkMaterialInterfacePieceLoading& other) const
  {
    return this->Data[ID] == other.Data[ID];
  }

private:
  vtkIdType Data[SIZE];
};
VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT
std::ostream& operator<<(std::ostream& sout, const vtkMaterialInterfacePieceLoading& fp);
VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT
void PrintPieceLoadingHistogram(std::vector<std::vector<vtkIdType> >& pla);
#endif
//@}

// VTK-HeaderTest-Exclude: vtkMaterialInterfacePieceLoading.h
