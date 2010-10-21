/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAdiosReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of TestAdiosReader
// .SECTION Description
//

#include "vtkAdiosReader.h"
#include "vtkDebugLeaks.h"

#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"

#include "vtkRectilinearGrid.h"
#include "vtkDataArray.h"

#include "vtkCompositePolyDataMapper.h"
#include "vtkCompositeDataGeometryFilter.h"

int main( int argc, char *argv[] )
{
  // Read file name.
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/adios/pixie3d.bp"); // xgc.fieldp.0250.bp  xgc.mesh.bp  xgc.oneddiag.bp

  // Create the reader.
  vtkAdiosReader* reader = vtkAdiosReader::New();
  reader->SetFileName(fname);
  reader->Update();
  delete [] fname;

  // Geometry filter to get polydata
  vtkCompositeDataGeometryFilter* toPoly = vtkCompositeDataGeometryFilter::New();
  toPoly->SetInputConnection(reader->GetOutputPort());

  // Create a mapper.
  vtkCompositePolyDataMapper* mapper = vtkCompositePolyDataMapper::New();
  mapper->SetInputConnection(toPoly->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Basic visualisation.
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  vtkRenderer* ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkInteractorStyleTrackballCamera* style  = vtkInteractorStyleTrackballCamera::New();
  iren->SetInteractorStyle(style);
  style->Delete();
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(0.3,0.3,0.3);
  renWin->SetSize(300,300);

  // interact with data
  ren->ResetCamera();
  renWin->Render();

//  int retVal = vtkRegressionTestImage( renWin );

//  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
//    {
//    iren->Start();
//    }

  iren->Start();


  actor->Delete();
  mapper->Delete();
  reader->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  return EXIT_SUCCESS;
}
