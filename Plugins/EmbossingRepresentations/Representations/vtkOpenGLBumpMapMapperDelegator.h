// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLBivariateNoiseMapperDelegator
 * @brief   Delegates rendering to a custom vtkOpenGLBatchedPolyDataMapper.
 *
 * @sa vtkOpenGLCompositePolyDataMapperDelegator vtkOpenGLBatchedPolyDataMapper
 */

#ifndef vtkOpenGLBumpMapMapperDelegator_h
#define vtkOpenGLBumpMapMapperDelegator_h

#include "vtkOpenGLCompositePolyDataMapperDelegator.h"

#include "vtkEmbossingRepresentationsModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKEMBOSSINGREPRESENTATIONS_EXPORT vtkOpenGLBumpMapMapperDelegator
  : public vtkOpenGLCompositePolyDataMapperDelegator
{
public:
  static vtkOpenGLBumpMapMapperDelegator* New();
  vtkTypeMacro(vtkOpenGLBumpMapMapperDelegator, vtkOpenGLCompositePolyDataMapperDelegator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Copy over the reference to the input array
   * that gets used by internal OpenGL bump map mapper.
   */
  void ShallowCopy(vtkCompositePolyDataMapper* mapper) override;

protected:
  vtkOpenGLBumpMapMapperDelegator();
  ~vtkOpenGLBumpMapMapperDelegator() override;

private:
  vtkOpenGLBumpMapMapperDelegator(const vtkOpenGLBumpMapMapperDelegator&) = delete;
  void operator=(const vtkOpenGLBumpMapMapperDelegator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
