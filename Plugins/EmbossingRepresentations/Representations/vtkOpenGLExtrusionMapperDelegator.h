// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLExtrusionMapperDelegator
 * @brief   Delegates rendering to a custom vtkOpenGLBatchedPolyDataMapper.
 *
 * @sa vtkOpenGLCompositePolyDataMapperDelegator vtkOpenGLBatchedPolyDataMapper
 */

#ifndef vtkOpenGLExtrusionMapperDelegator_h
#define vtkOpenGLExtrusionMapperDelegator_h

#include "vtkOpenGLCompositePolyDataMapperDelegator.h"

#include "vtkEmbossingRepresentationsModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKEMBOSSINGREPRESENTATIONS_EXPORT vtkOpenGLExtrusionMapperDelegator
  : public vtkOpenGLCompositePolyDataMapperDelegator
{
public:
  static vtkOpenGLExtrusionMapperDelegator* New();
  vtkTypeMacro(vtkOpenGLExtrusionMapperDelegator, vtkOpenGLCompositePolyDataMapperDelegator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void GetDataRange(double range[2]);
  void SetNeedRebuild(bool value);
  void ShallowCopy(vtkCompositePolyDataMapper* cpdm) override;

protected:
  vtkOpenGLExtrusionMapperDelegator();
  ~vtkOpenGLExtrusionMapperDelegator() override;

private:
  vtkOpenGLExtrusionMapperDelegator(const vtkOpenGLExtrusionMapperDelegator&) = delete;
  void operator=(const vtkOpenGLExtrusionMapperDelegator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
