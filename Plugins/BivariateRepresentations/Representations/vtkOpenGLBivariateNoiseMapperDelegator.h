// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLBivariateNoiseMapperDelegator
 * @brief   Delegates rendering to a custom vtkOpenGLBatchedPolyDataMapper.
 *
 * @sa vtkOpenGLCompositePolyDataMapperDelegator vtkOpenGLBatchedPolyDataMapper
 */

#ifndef vtkOpenGLBivariateNoiseMapperDelegator_h
#define vtkOpenGLBivariateNoiseMapperDelegator_h

#include "vtkOpenGLCompositePolyDataMapperDelegator.h"

#include "vtkBivariateRepresentationsModule.h" // for export macro

class VTKBIVARIATEREPRESENTATIONS_EXPORT vtkOpenGLBivariateNoiseMapperDelegator
  : public vtkOpenGLCompositePolyDataMapperDelegator
{
public:
  static vtkOpenGLBivariateNoiseMapperDelegator* New();
  vtkTypeMacro(vtkOpenGLBivariateNoiseMapperDelegator, vtkOpenGLCompositePolyDataMapperDelegator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Copy over the reference to the input noise array
   */
  void ShallowCopy(vtkCompositePolyDataMapper* mapper) override;

protected:
  vtkOpenGLBivariateNoiseMapperDelegator();
  ~vtkOpenGLBivariateNoiseMapperDelegator() override;

private:
  vtkOpenGLBivariateNoiseMapperDelegator(const vtkOpenGLBivariateNoiseMapperDelegator&) = delete;
  void operator=(const vtkOpenGLBivariateNoiseMapperDelegator&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkOpenGLBivariateNoiseMapperDelegator.h
