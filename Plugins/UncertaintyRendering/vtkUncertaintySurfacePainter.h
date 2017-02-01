/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUncertaintySurfacePainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkUncertaintySurfacePainter_h
#define vtkUncertaintySurfacePainter_h

#include "vtkLightingHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPainter.h"
#include "vtkPiecewiseFunction.h"
#include "vtkShaderProgram2.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

class VTK_EXPORT vtkUncertaintySurfacePainter : public vtkPainter
{
public:
  static vtkUncertaintySurfacePainter* New();
  vtkTypeMacro(vtkUncertaintySurfacePainter, vtkPainter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  void ReleaseGraphicsResources(vtkWindow* window);

  // Description:
  // Process information values.
  void ProcessInformation(vtkInformation* info);

  // Description:
  // Get the output data object from this painter.
  vtkDataObject* GetOutput();

  // Description:
  // Enable/Disable this painter.
  vtkSetMacro(Enabled, int) vtkGetMacro(Enabled, int) vtkBooleanMacro(Enabled, int)

    // Description:
    // Set/get the uncertainty array name.
    vtkGetStringMacro(UncertaintyArrayName) vtkSetStringMacro(UncertaintyArrayName)

    // Description:
    // Set/get the uncertainty transfer function.
    vtkGetObjectMacro(TransferFunction, vtkPiecewiseFunction)
      vtkSetObjectMacro(TransferFunction, vtkPiecewiseFunction)

    // Description:
    // Set/get the uncertainty scale factor.
    vtkSetClampMacro(UncertaintyScaleFactor, float, 0.0f, 50.0f)
      vtkGetMacro(UncertaintyScaleFactor, float)

    // Description:
    // Set/get the scalar value range of the array used for coloring.
    vtkSetMacro(ScalarValueRange, float) vtkGetMacro(ScalarValueRange, float)

      protected : vtkUncertaintySurfacePainter();
  ~vtkUncertaintySurfacePainter();

  // Description:
  // Prepare to render.
  void PrepareForRendering(vtkRenderer* renderer, vtkActor* actor);

  // Description:
  // Performs the actual rendering.
  void RenderInternal(
    vtkRenderer* renderer, vtkActor* actor, unsigned long typeFlags, bool forceCompileOnly);

  // Description:
  // Passes information.
  void PassInformation(vtkPainter* toPainter);

  // Description:
  // Prepares the output data object.
  bool PrepareOutput();

  // Description:
  // Add the uncertainties array to the output.
  void GenerateUncertaintiesArray(vtkDataObject* input, vtkDataObject* output);

private:
  int Enabled;
  vtkDataObject* Output;
  vtkSmartPointer<vtkShaderProgram2> Shader;
  vtkWeakPointer<vtkOpenGLRenderWindow> LastRenderWindow;
  vtkSmartPointer<vtkLightingHelper> LightingHelper;
  vtkPiecewiseFunction* TransferFunction;
  char* UncertaintyArrayName;
  int RenderingPreparationSuccess;
  float UncertaintyScaleFactor;
  float ScalarValueRange;
  GLuint PermTextureId;
  GLuint SimplexTextureId;
  GLuint GradTextureId;
};

#endif // vtkUncertaintySurfacePainter_h
