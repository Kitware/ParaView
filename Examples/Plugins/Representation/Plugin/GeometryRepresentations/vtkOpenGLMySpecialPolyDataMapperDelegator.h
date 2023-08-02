// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLMySpecialPolyDataMapperDelegator
 * @brief   Delegates rendering to a batched mapper.
 */

#ifndef vtkOpenGLMySpecialPolyDataMapperDelegator_h
#define vtkOpenGLMySpecialPolyDataMapperDelegator_h

#include "vtkCompositePolyDataMapperDelegator.h"
#include "vtkOpenGLCompositePolyDataMapperDelegator.h"

#include "GeometryRepresentationsModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN

class GEOMETRYREPRESENTATIONS_EXPORT vtkOpenGLMySpecialPolyDataMapperDelegator
  : public vtkOpenGLCompositePolyDataMapperDelegator
{
public:
  static vtkOpenGLMySpecialPolyDataMapperDelegator* New();
  vtkTypeMacro(
    vtkOpenGLMySpecialPolyDataMapperDelegator, vtkOpenGLCompositePolyDataMapperDelegator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Composite mapper will invoke this for each delegator.
   * You could pass on the InputArrayToProcess to the delegate here.
   */
  void ShallowCopy(vtkCompositePolyDataMapper* cpdm) override;

protected:
  vtkOpenGLMySpecialPolyDataMapperDelegator();
  ~vtkOpenGLMySpecialPolyDataMapperDelegator() override;

private:
  vtkOpenGLMySpecialPolyDataMapperDelegator(
    const vtkOpenGLMySpecialPolyDataMapperDelegator&) = delete;
  void operator=(const vtkOpenGLMySpecialPolyDataMapperDelegator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
