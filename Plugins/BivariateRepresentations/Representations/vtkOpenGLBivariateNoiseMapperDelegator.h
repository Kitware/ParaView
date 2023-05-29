/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLBivariateNoiseMapperDelegator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

VTK_ABI_NAMESPACE_BEGIN

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

VTK_ABI_NAMESPACE_END
#endif
