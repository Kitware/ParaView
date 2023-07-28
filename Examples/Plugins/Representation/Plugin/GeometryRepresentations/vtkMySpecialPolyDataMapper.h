// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkMySpecialPolyDataMapper - dummy special polydata mapper.
// .SECTION Description
// This is a place holder for a poly data mapper. This example simply uses the
// standard polydata mapper
// Note that it's essential that the mapper can handle composite datasets. If
// your mapper cannot, then simply use an append filter internally to
// merge the blocks into a single polydata.
// The mapper is your representation's entry point into rendering.
// For OpenGL, you should ideally develop and create a delegator inherited from
// vtkOpenGLCompositePolyDataMapperDelegator. This delegator should in turn create
// a delegate class that derives vtkOpenGLBatchedPolyDataMapper, in which you can implement
// custom graphics by replacing GLSL declarations and implementations of the form
// "//VTK::Feature::Dec" and "//VTK::Feature::Impl"
// Please do not clutter the composite mapper subclass with GLSL or VTK OpenGL code.

#ifndef vtkMySpecialPolyDataMapper_h
#define vtkMySpecialPolyDataMapper_h

#include "GeometryRepresentationsModule.h" // for export macro
#include "vtkCompositePolyDataMapper.h"

class GEOMETRYREPRESENTATIONS_EXPORT vtkMySpecialPolyDataMapper : public vtkCompositePolyDataMapper
{
public:
  static vtkMySpecialPolyDataMapper* New();
  vtkTypeMacro(vtkMySpecialPolyDataMapper, vtkCompositePolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkMySpecialPolyDataMapper();
  ~vtkMySpecialPolyDataMapper();

  vtkCompositePolyDataMapperDelegator* CreateADelegator();

private:
  vtkMySpecialPolyDataMapper(const vtkMySpecialPolyDataMapper&) = delete;
  void operator=(const vtkMySpecialPolyDataMapper&) = delete;
};

#endif
