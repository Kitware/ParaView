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
#include "vtkCTHExtractAMRPart.h"
#include "vtkCTHOutlineFilter.h"
#include "vtkCTHDataToPolyDataFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

int main()
{
  vtkCTHFractal *fractal = vtkCTHFractal::New();
  fractal->SetDimensions( 10 );
  fractal->SetFractalValue( 9.5 );
  fractal->SetMaximumLevel( 5 );
  fractal->SetGhostLevels( 0 );
  
  vtkCTHExtractAMRPart *extract = vtkCTHExtractAMRPart::New();
  extract->SetInput( fractal->GetOutput());
  
  vtkCTHOutlineFilter *outline = vtkCTHOutlineFilter::New();
  outline->SetInput( fractal->GetOutput());
  
  vtkCTHDataToPolyDataFilter *geometry = vtkCTHDataToPolyDataFilter::New();
  geometry->SetInput( fractal->GetOutput());
  
  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
  outlineMapper->SetInput( outline->GetOutput() );
  
  vtkActor *outlineActor = vtkActor::New();
  outlineActor->SetMapper( outlineMapper );

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput( geometry->GetOutput() );
  
  vtkActor *actor = vtkActor::New();
  actor->SetMapper( mapper );

  vtkRenderer *ren = vtkRenderer::New();
  ren->AddActor( actor );
  ren->AddActor( outlineActor );
  
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer( ren );

  renWin->Render();
  
  fractal->Delete();
  extract->Delete();
  outline->Delete();
  geometry->Delete();
  outlineMapper->Delete();
  outlineActor->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();

  return 0;
}


