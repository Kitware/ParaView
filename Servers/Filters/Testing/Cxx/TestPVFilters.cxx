/*=========================================================================

  Program:   ParaView
  Module:    TestPVFilters.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridReader.h"
#include "vtkTesting.h"
#include "vtkCleanUnstructuredGrid.h"
#include "vtkGlyphSource2D.h"
#include "vtkPVGlyphFilter.h"
#include "vtkPVThresholdFilter.h"
#include "vtkPVGeometryFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPVContourFilter.h"
#include "vtkPlane.h"
#include "vtkPVClipDataSet.h"
#include "vtkPolyData.h"
#include "vtkPVLODActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

int main(int argc, char* argv[])
{
  char* fname = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/blow.vtk");

  vtkUnstructuredGridReader *reader = vtkUnstructuredGridReader::New();
  reader->SetFileName( fname );
  reader->SetScalarsName("thickness9");
  reader->SetVectorsName("displacement9");

  delete [] fname;
  
  vtkCleanUnstructuredGrid* clean = vtkCleanUnstructuredGrid::New();
  clean->SetInput( reader->GetOutput() );
  
  vtkGlyphSource2D *gs = vtkGlyphSource2D::New();
  gs->SetGlyphTypeToThickArrow();
  gs->SetScale( 1 );
  gs->FilledOff();
  gs->CrossOff();

  vtkPVGlyphFilter *glyph = vtkPVGlyphFilter::New();
  glyph->SetInput( clean->GetOutput() );
  glyph->SetSource( gs->GetOutput());
  glyph->SetScaleFactor( 0.75 );
  glyph->Update();  //discard

  vtkPVContourFilter *contour = vtkPVContourFilter::New();
  contour->SetInput( clean->GetOutput() );
  contour->SetValue( 0, 0.5 );
  
  vtkPlane *plane = vtkPlane::New();
  plane->SetOrigin(0.25, 0, 0);
  plane->SetNormal(-1, -1, 0);

  vtkPVClipDataSet *clip = vtkPVClipDataSet::New();
  clip->SetInput( clean->GetOutput() );
  clip->SetClipFunction( plane );
  clip->GenerateClipScalarsOn();
  clip->SetValue( 0.5 );
  clip->Update(); //discard
  
  vtkPVGeometryFilter *geometry = vtkPVGeometryFilter::New();
  geometry->SetInput( contour->GetOutput() );
  
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput( geometry->GetOutput() );
  
  vtkPVLODActor *actor = vtkPVLODActor::New();
  actor->SetMapper( mapper );
  
  vtkRenderer *ren = vtkRenderer::New();
  ren->AddActor( actor );
  
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer( ren );
  
  renWin->Render();
  
  reader->Delete();
  clean->Delete();
  gs->Delete();
  glyph->Delete();
  contour->Delete();
  plane->Delete();
  clip->Delete();
  geometry->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();

  return 0;
}


