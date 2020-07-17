/*=========================================================================

  Program:   ParaView
  Module:    vtkConvertPolyhedraFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2020 Menno Deij - van Rijswijk (MARIN)
-------------------------------------------------------------------------*/

/**
 * @class   vtkConvertPolyhedraFilter
 * @brief   Converts polyhedral and polygonal cells to simple cells if possible
 *
 *
 * vtkConvertPolyhedraFilter takes an unstructured grid comprised of
 * cells of any cell type and attempts to convert polyhedral cells and polygonal
 * cels to simple cells if possible.
 */

#ifndef vtkConvertPolyhedraFilter_h
#define vtkConvertPolyhedraFilter_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkUnstructuredGridBase;
class vtkIdList;
class TestConvertPolyhedra; // for testing purposes

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkConvertPolyhedraFilter
  : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkConvertPolyhedraFilter* New();
  vtkTypeMacro(vtkConvertPolyhedraFilter, vtkUnstructuredGridAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkConvertPolyhedraFilter() = default;
  ~vtkConvertPolyhedraFilter() = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkConvertPolyhedraFilter(const vtkConvertPolyhedraFilter&) = delete;
  void operator=(const vtkConvertPolyhedraFilter&) = delete;

  // forward declared test class is friend so that it can call
  // the two functions below without having to expose them as public
  friend class TestConvertPolyhedra;

  void InsertNextPolyhedralCell(vtkUnstructuredGridBase*, vtkIdList*) const;
  void InsertNextPolygonalCell(vtkUnstructuredGridBase*, vtkIdList*) const;
};

#endif
