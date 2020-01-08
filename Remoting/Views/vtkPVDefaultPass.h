/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVDefaultPass
 * @brief   encapsulates the traditional OpenGL pipeline
 * (minus the camera).
 *
 * vtkPVDefaultPass is a simple render pass that encapsulates the traditional
 * OpenGL pipeline (minus the camera).
*/

#ifndef vtkPVDefaultPass_h
#define vtkPVDefaultPass_h

#include "vtkRemotingViewsModule.h" // needed for export macro
#include "vtkRenderPass.h"

class VTKREMOTINGVIEWS_EXPORT vtkPVDefaultPass : public vtkRenderPass
{
public:
  static vtkPVDefaultPass* New();
  vtkTypeMacro(vtkPVDefaultPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actual rendering code.
   */
  void Render(const vtkRenderState* render_state) override;

protected:
  vtkPVDefaultPass();
  ~vtkPVDefaultPass() override;

private:
  vtkPVDefaultPass(const vtkPVDefaultPass&) = delete;
  void operator=(const vtkPVDefaultPass&) = delete;
};

#endif
