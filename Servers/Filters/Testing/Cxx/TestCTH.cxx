/*=========================================================================

  Program:   ParaView
  Module:    TestCTH.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHFractal.h"
#include "vtkCTHData.h"
#include "vtkPlane.h"
#include "vtkCTHExtractAMRPart.h"
#include "vtkCTHOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkClipDataSet.h"
#include "vtkCutter.h"

int main(int argc, char * argv[])
{
  vtkCTHFractal *fractal = vtkCTHFractal::New();
  fractal->SetDimensions( 10 );
  fractal->SetFractalValue( 9.5 );
  fractal->SetMaximumLevel( 3 );
  fractal->SetGhostLevels( 0 );
  fractal->Update();  //this seems to be needed
  
  vtkCTHData* data = vtkCTHData::New();
  data->ShallowCopy( fractal->GetOutput() );
  
  vtkPlane *clipPlane = vtkPlane::New();
  clipPlane->SetNormal ( 1, 1, 1);
  clipPlane->SetOrigin ( -0.5, 0, 1 );

  vtkClipDataSet* clip = vtkClipDataSet::New();
  clip->SetInput( data );
  clip->SetClipFunction( clipPlane );
  clip->Update(); //discard

  vtkCutter *cutter = vtkCutter::New();
  cutter->SetInput( data );
  cutter->SetCutFunction (clipPlane );
  cutter->Update(); //discard

  vtkCTHExtractAMRPart *extract = vtkCTHExtractAMRPart::New();
  extract->SetInput( fractal->GetOutput());
  extract->SetClipPlane (clipPlane);
  extract->GetNumberOfVolumeArrayNames ();
  extract->GetVolumeArrayName ( 0 );
  
  vtkCTHOutlineFilter *outline = vtkCTHOutlineFilter::New();
  outline->SetInput( fractal->GetOutput());
  
  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
  outlineMapper->SetInput( outline->GetOutput() );
  
  vtkActor *outlineActor = vtkActor::New();
  outlineActor->SetMapper( outlineMapper );

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput( extract->GetOutput() );
  mapper->SelectColorArray( "Fractal Volume Fraction" );
  
  vtkActor *actor = vtkActor::New();
  actor->SetMapper( mapper );
  vtkProperty *p = actor->GetProperty ();
  p->SetColor (0.5, 0.5, 0.5);

  vtkRenderer *ren = vtkRenderer::New();
  ren->AddActor( actor );
  ren->AddActor( outlineActor );
  
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer( ren );

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

   // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
 
  if ( retVal == vtkTesting::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up
  fractal->Delete();
  data->Delete();
  clipPlane->Delete();
  clip->Delete();
  cutter->Delete();
  extract->Delete();
  outline->Delete();
  outlineMapper->Delete();
  outlineActor->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
