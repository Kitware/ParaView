/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiSliceView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMultiSliceView
// .SECTION Description
// vtkPVMultiSliceView extends vtkPVRenderView but add meta-data informations
// used by SliceRepresentation as a data model.

#ifndef __vtkPVMultiSliceView_h
#define __vtkPVMultiSliceView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVRenderView.h"

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVMultiSliceView : public vtkPVRenderView
{
public:
  static vtkPVMultiSliceView* New();
  vtkTypeMacro(vtkPVMultiSliceView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // ====== For Slices along X axis ========================================

  // Description:
  // Return the number of offset values that are used for slices along X axis
  int GetNumberOfSliceX() const;

  // Description:
  // Set the number of offset values that are used for slices along X axis
  void SetNumberOfSliceX(int size);

  // Description:
  // Set an offset value for slice along X axis
  void SetSliceX(int index, double value);

  // Description:
  // Return a pointer to a list of offset values for slice along X axis.
  // The contents of the list will be garbage if the number of offset <= 0.
  const double* GetSliceX() const;

  // Description:
  // Set the origin of the cut for slices along Z axis.
  void SetSliceXOrigin(double x, double y, double z);

  // Description:
  // Set the origin of the cut for slices along Z axis.
  const double* GetSliceXOrigin() const;

  // Description:
  // Set the normal of the cut for slices along Z axis.
  void SetSliceXNormal(double x, double y, double z);

  // Description:
  // Set the normal of the cut for slices along Z axis.
  const double* GetSliceXNormal() const;

  // ====== For Slices along Y axis ========================================

  // Description:
  // Return the number of offset values that are used for slices along X axis
  int GetNumberOfSliceY() const;

  // Description:
  // Set the number of offset values that are used for slices along X axis
  void SetNumberOfSliceY(int size);

  // Description:
  // Set an offset value for slice along X axis
  void SetSliceY(int index, double value);

  // Description:
  // Return a pointer to a list of offset values for slice along X axis.
  // The contents of the list will be garbage if the number of offset <= 0.
  const double* GetSliceY() const;

  // Description:
  // Set the origin of the cut for slices along Z axis.
  void SetSliceYOrigin(double x, double y, double z);

  // Description:
  // Set the origin of the cut for slices along Z axis.
  const double* GetSliceYOrigin() const;

  // Description:
  // Set the normal of the cut for slices along Z axis.
  void SetSliceYNormal(double x, double y, double z);

  // Description:
  // Set the normal of the cut for slices along Z axis.
  const double* GetSliceYNormal() const;

  // ====== For Slices along Z axis ========================================

  // Description:
  // Return the number of offset values that are used for slices along X axis
  int GetNumberOfSliceZ() const;

  // Description:
  // Set the number of offset values that are used for slices along X axis
  void SetNumberOfSliceZ(int size);

  // Description:
  // Set an offset value for slice along X axis
  void SetSliceZ(int index, double value);

  // Description:
  // Return a pointer to a list of offset values for slice along X axis.
  // The contents of the list will be garbage if the number of offset <= 0.
  const double* GetSliceZ() const;

  // Description:
  // Set the origin of the cut for slices along Z axis.
  void SetSliceZOrigin(double x, double y, double z);

  // Description:
  // Set the origin of the cut for slices along Z axis.
  const double* GetSliceZOrigin() const;

  // Description:
  // Set the normal of the cut for slices along Z axis.
  void SetSliceZNormal(double x, double y, double z);

  // Description:
  // Set the normal of the cut for slices along Z axis.
  const double* GetSliceZNormal() const;

  // ==== Generic methods where sliceIndex represent the slice [X, Y, Z]
  int GetNumberOfSlice(int sliceIndex) const;
  void SetNumberOfSlice(int sliceIndex, int size);
  void SetSlice(int sliceIndex, int index, double value);
  const double* GetSlice(int sliceIndex) const;
  void SetSliceOrigin(int sliceIndex, double x, double y, double z);
  double* GetSliceOrigin(int sliceIndex) const;
  void SetSliceNormal(int sliceIndex, double x, double y, double z);
  double* GetSliceNormal(int sliceIndex) const;

  // Description:
  // Show/hide the bounding box outline
  vtkSetMacro(ShowOutline, int);
  vtkGetMacro(ShowOutline, int);

//BTX
protected:
  vtkPVMultiSliceView();
  ~vtkPVMultiSliceView();

  int ShowOutline;
private:
  vtkPVMultiSliceView(const vtkPVMultiSliceView&); // Not implemented
  void operator=(const vtkPVMultiSliceView&); // Not implemented

  class vtkSliceInternal;
  vtkSliceInternal* Internal;
//ETX
};

#endif
