//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
// .NAME vtkDataSetRegionSurfaceFilter - Extract surface of materials.
// .SECTION Description
// This filter extracts surfaces of materials such that a surface
// could have a material on each side of it. It also stores a
// mapping of the original cells and their sides back to the original grid
// so that we can output boundary information for those cells given
// only surfaces.

#ifndef __vtkDataSetRegionSurfaceFilter_h
#define __vtkDataSetRegionSurfaceFilter_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

#include "vtkDataSetSurfaceFilter.h"

class vtkCharArray;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkDataSetRegionSurfaceFilter : public vtkDataSetSurfaceFilter
{
public:
  static vtkDataSetRegionSurfaceFilter* New();
  vtkTypeMacro(vtkDataSetRegionSurfaceFilter, vtkDataSetSurfaceFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetStringMacro(RegionArrayName);
  vtkGetStringMacro(RegionArrayName);

  virtual int UnstructuredGridExecute(vtkDataSet *input,
                                      vtkPolyData *output);

  //make it clear we want all the recordOrigCellId signatures from our parent
  using vtkDataSetSurfaceFilter::RecordOrigCellId;

  //override one of the signatures
  virtual void RecordOrigCellId(vtkIdType newIndex, vtkFastGeomQuad *quad);

  //Description:
  //Whether to return single sided material interfaces or double sided
  //Default is single
  vtkSetMacro(SingleSided, bool);
  vtkGetMacro(SingleSided, bool);
//BTX
protected:
  vtkDataSetRegionSurfaceFilter();
  ~vtkDataSetRegionSurfaceFilter();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  /// Implementation of the algorithm.
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  virtual void InsertQuadInHash(vtkIdType a, vtkIdType b, vtkIdType c,
                                vtkIdType d, vtkIdType sourceId, vtkIdType faceId = -1);
  virtual void InsertTriInHash(vtkIdType a, vtkIdType b, vtkIdType c,
                       vtkIdType sourceId, vtkIdType faceId = -1);

  virtual vtkFastGeomQuad *GetNextVisibleQuadFromHash();

private:
  vtkDataSetRegionSurfaceFilter(const vtkDataSetRegionSurfaceFilter&); // Not implemented.
  void operator=(const vtkDataSetRegionSurfaceFilter&); // Not implemented.
//ETX

  char *RegionArrayName;
  vtkIntArray    *RegionArray;
  vtkIdTypeArray *OrigCellIds;
  vtkCharArray   *CellFaceIds;
  bool SingleSided;

  class Internals;
  Internals *Internal;
};

#endif
