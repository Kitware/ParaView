/*=========================================================================

 Program:   Visualization Toolkit
 Module:    PointSpriteDemo.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

// .NAME Deno of vtkPointSpriteProperty
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>
// .SECTION Description
// this program tests the point sprite support by vtkPointSpriteProperty.


#include "vtkActor.h"
#include "vtkBrownianPoints.h"
#include "vtkCamera.h"
#include "vtkProperty.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkPointSpriteProperty.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"

#include "Testing/Cxx/vtkTestUtilities.h"
#include "Testing/Cxx/vtkRegressionTestImage.h"

int main(int argc, char *argv[])
{
  vtkSphereSource * sphere = vtkSphereSource::New();
  sphere->SetRadius(5);
  sphere->SetPhiResolution(20);
  sphere->SetThetaResolution(20);

  vtkBrownianPoints * randomVector = vtkBrownianPoints::New();
  randomVector->SetMinimumSpeed(0);
  randomVector->SetMaximumSpeed(1);
  randomVector->SetInputConnection(sphere->GetOutputPort());

  vtkPainterPolyDataMapper *mapper = vtkPainterPolyDataMapper::New();
  mapper->SetInputConnection(randomVector->GetOutputPort());
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("BrownianVectors");
  mapper->SetInterpolateScalarsBeforeMapping(0);

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkPointSpriteProperty* property = vtkPointSpriteProperty::New();
  property->SetRadiusMode(1);
  property->SetRenderMode(0);
  property->SetRadiusArrayName("BrownianVectors");
  property->SetRadiusRange(0.1, 2);
  property->SetRepresentationToPoints();
  actor->SetProperty(property);

  vtkRenderer *renderer = vtkRenderer::New();
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);

  vtkRenderWindowInteractor *interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow(renWin);

  renWin->SetSize(400, 400);
  renWin->Render();
  interactor->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold(renWin,18);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    interactor->Start();
    }

  sphere->Delete();
  randomVector->Delete();
  mapper->Delete();
  actor->Delete();
  property->Delete();
  renderer->Delete();
  renWin->Delete();
  interactor->Delete();

  return !retVal;
}

