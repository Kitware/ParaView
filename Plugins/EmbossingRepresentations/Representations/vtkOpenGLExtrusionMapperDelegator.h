/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLExtrusionMapperDelegator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
