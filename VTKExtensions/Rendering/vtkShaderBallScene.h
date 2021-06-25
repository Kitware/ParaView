/*=========================================================================

   Program: ParaView
   Module: vtkShaderBallScene.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

#ifndef vtkShaderBallScene_h
#define vtkShaderBallScene_h

#include "vtkExtensionsRenderingModule.h" // needed for export macros
#include "vtkNew.h"                       // for ivars
#include "vtkObject.h"
#include "vtkSetGet.h" // for get/set macros

class vtkOSPRayPass;
class vtkGenericOpenGLRenderWindow;
class vtkActor;
class vtkRenderer;

/**
 * @class pqShaderBallWidget
 * Used as part of the pqMaterialEditor to display the current
 * selected material in a simple scene
 * containing a sphere and a plane.
 */
class VTKEXTENSIONSRENDERING_EXPORT vtkShaderBallScene : public vtkObject
{
public:
  static vtkShaderBallScene* New();
  vtkTypeMacro(vtkShaderBallScene, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the render window where the sceene is rendered
   */
  vtkGetObjectMacro(Window, vtkGenericOpenGLRenderWindow);

  /**
   * Returns the renderer for the simple scene
   */
  vtkGetObjectMacro(Renderer, vtkRenderer);

  /**
   * Updates the sphere material with the given material name
   */
  void SetMaterialName(const char*);

  /**
   * Renders the scene
   */
  void Render();

  /**
   * Resets the vtkOSPRayPass of the scene. This must be called when the widget containing this
   * class is pop upped, otherwise it crashes.
   *
   * The problems comes from vtkOSPRayPass.cxx::RenderInternal where the shader is destroyed
   * whenever the window is poped out
   */
  void ResetPass();

  void Modified() override;

  void SetNumberOfSamples(int numberOfSamples);

  /**
   * Set the value of visible and render the scene if value is true.
   */
  void SetVisible(bool visible);

protected:
  vtkShaderBallScene();
  ~vtkShaderBallScene();

private:
  vtkShaderBallScene(const vtkShaderBallScene&) = delete;
  void operator=(const vtkShaderBallScene&) = delete;

  vtkNew<vtkGenericOpenGLRenderWindow> Window;
  vtkNew<vtkActor> SphereActor;
  vtkNew<vtkRenderer> Renderer;

  vtkOSPRayPass* OSPRayPass = nullptr;

  bool NeedRender = true;
  bool Visible = false;
};

#endif // vtkShaderBallScene_h
