// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkShaderBallScene_h
#define vtkShaderBallScene_h

#include "vtkObject.h"

#include "vtkExtensionsShaderBallModule.h" // needed for export macros
#include "vtkNew.h"                        // for ivars
#include "vtkSetGet.h"                     // for get/set macros

class vtkActor;
class vtkGenericOpenGLRenderWindow;
class vtkRenderer;

/**
 * @class vtkShaderBallScene
 * Used as part of the pqMaterialEditor to display the current
 * selected material in a simple scene
 * containing a sphere and a plane.
 */
class VTKEXTENSIONSSHADERBALL_EXPORT vtkShaderBallScene : public vtkObject
{
public:
  static vtkShaderBallScene* New();
  vtkTypeMacro(vtkShaderBallScene, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the render window where the scene is rendered
   */
  vtkGetObjectMacro(Window, vtkGenericOpenGLRenderWindow);

  /**
   * Returns the renderer for the simple scene
   */
  vtkGetObjectMacro(Renderer, vtkRenderer);

  ///@{
  /**
   * Updates the sphere material with the given material name
   * Default is empty
   */
  void SetMaterialName(const char*);
  const char* GetMaterialName() const;
  ///@}

  ///@{
  /**
   * Setup the number of samples to use in the Shader ball scene.
   * Default is 2.
   */
  void SetNumberOfSamples(int numberOfSamples);
  int GetNumberOfSamples() const;
  ///@}

  ///@{
  /**
   * Set the value of visible and render the scene if value is true.
   * Default is false.
   */
  void SetVisible(bool visible);
  vtkGetMacro(Visible, bool);
  vtkBooleanMacro(Visible, bool);
  ///@}

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
  void ResetOSPrayPass();

  void Modified() override;

protected:
  vtkShaderBallScene();
  virtual ~vtkShaderBallScene();

private:
  vtkShaderBallScene(const vtkShaderBallScene&) = delete;
  void operator=(const vtkShaderBallScene&) = delete;

  vtkNew<vtkGenericOpenGLRenderWindow> Window;
  vtkNew<vtkActor> SphereActor;
  vtkNew<vtkRenderer> Renderer;

  bool NeedRender = true;
  bool Visible = false;
};

#endif // vtkShaderBallScene_h
