/*=========================================================================

  Program:   ParaView
  Module:    vtkCVGeometryCache.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCVGeometryCache - caches geometry assigned to a mapper
// .SECTION Description
// vtkCVGeometryCache is a source that provides a cached geometry
// (shallow) copied from the input of a mapper. It is used in
// comparative visualization. This is a multiple output source (one
// output for each added geometry)

#ifndef __vtkCVGeometryCache_h
#define __vtkCVGeometryCache_h

#include "vtkPolyDataAlgorithm.h"

class vtkPVGeometryFilter;

//BTX
struct vtkCVGeometryCacheInternal;
//ETX

class VTK_EXPORT vtkCVGeometryCache : public vtkPolyDataAlgorithm
{
public:
  static vtkCVGeometryCache *New();

  vtkTypeRevisionMacro(vtkCVGeometryCache,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Shallow copies the input of the mapper into the internal cache.
  void AddGeometry(vtkPVGeometryFilter* filter);

  // Description:
  // Removes all cached geometry.
  void RemoveAllGeometry();

protected:
  vtkCVGeometryCache();
  ~vtkCVGeometryCache();

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);
  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);
  virtual int RequestDataObject(vtkInformation *request,
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector);

  vtkCVGeometryCacheInternal* Internal;
private:
  vtkCVGeometryCache(const vtkCVGeometryCache&);  // Not implemented.
  void operator=(const vtkCVGeometryCache&);  // Not implemented.
};


#endif


