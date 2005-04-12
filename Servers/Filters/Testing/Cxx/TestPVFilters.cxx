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
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkCleanUnstructuredGrid.h"
#include "vtkGlyphSource2D.h"
#include "vtkPVGlyphFilter.h"
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
#include "vtkMergeArrays.h"
#include "vtkPVArrowSource.h"
#include "vtkPVRibbonFilter.h"
#include "vtkPVThresholdFilter.h"
#include "vtkPVLinearExtrusionFilter.h"
#include "vtkWarpScalar.h"
#include "vtkDataSetMapper.h"

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
  
  vtkPVGeometryFilter *geometry = vtkPVGeometryFilter::New();
  geometry->SetInput( contour->GetOutput() );
  
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput( geometry->GetOutput() );
  
  vtkPVLODActor *actor = vtkPVLODActor::New();
  actor->SetMapper( mapper );
  
  //Now for the PVPolyData part:
  vtkPVRibbonFilter *ribbon = vtkPVRibbonFilter::New();
  ribbon->SetInput( contour->GetOutput() );
  ribbon->SetWidth( 0.1 );
  ribbon->SetWidthFactor(5);

  vtkPVArrowSource *arrow = vtkPVArrowSource::New();

  vtkPVLinearExtrusionFilter *extrude = vtkPVLinearExtrusionFilter::New();
  extrude->SetInput( arrow->GetOutput() );
  extrude->SetScaleFactor(1);
  extrude->SetExtrusionTypeToNormalExtrusion();
  extrude->SetVector(1, 0, 0);
  
  vtkPVThresholdFilter *threshold = vtkPVThresholdFilter::New();
  threshold->SetInput( ribbon->GetOutput() );
  threshold->ThresholdBetween(0.25, 0.75);

  vtkWarpScalar *warp = vtkWarpScalar::New();
  warp->SetInput( threshold->GetOutput() );
  warp->XYPlaneOn();
  warp->SetScaleFactor(0.5);

  vtkMergeArrays *merge = vtkMergeArrays::New();
  merge->AddInput( warp->GetOutput() );
  merge->AddInput( extrude->GetOutput() );
  merge->AddInput( clip->GetOutput() );
  merge->AddInput( glyph->GetOutput() );
  merge->Update();  //discard
  merge->GetNumberOfOutputs ();

  vtkDataSetMapper *warpMapper = vtkDataSetMapper::New();
  warpMapper->SetInput( warp->GetOutput() );
  
  vtkActor *warpActor = vtkActor::New();
  warpActor->SetMapper( warpMapper );

  vtkRenderer *ren = vtkRenderer::New();
  ren->AddActor( actor );
  ren->AddActor( warpActor );

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

  arrow->Delete();
  ribbon->Delete();
  extrude->Delete();
  threshold->Delete();
  merge->Delete();
  warp->Delete();
  warpMapper->Delete();
  warpActor->Delete();

  ren->Delete();
  renWin->Delete();

  return 0;
}


