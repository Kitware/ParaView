/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLMySpecialPolyDataMapperDelegator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
