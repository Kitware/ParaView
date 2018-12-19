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
#include "vtkCleanUnstructuredGrid.h"
#include "vtkClipDataSet.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkGlyphSource2D.h"
#include "vtkMergeArrays.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVLinearExtrusionFilter.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRibbonFilter.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkWarpScalar.h"

int TestPVFilters(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/blow.vtk");

  vtkUnstructuredGridReader* reader = vtkUnstructuredGridReader::New();
  reader->SetFileName(fname);
  reader->SetScalarsName("thickness9");
  reader->SetVectorsName("displacement9");

  delete[] fname;

  vtkCleanUnstructuredGrid* clean = vtkCleanUnstructuredGrid::New();
  clean->SetInputConnection(reader->GetOutputPort());

  vtkGlyphSource2D* gs = vtkGlyphSource2D::New();
  gs->SetGlyphTypeToThickArrow();
  gs->SetScale(1);
  gs->FilledOff();
  gs->CrossOff();

  vtkContourFilter* contour = vtkContourFilter::New();
  contour->SetInputConnection(clean->GetOutputPort());
  contour->SetValue(0, 0.5);

  vtkPlane* plane = vtkPlane::New();
  plane->SetOrigin(0.25, 0, 0);
  plane->SetNormal(-1, -1, 0);

  vtkClipDataSet* clip = vtkClipDataSet::New();
  clip->SetInputConnection(clean->GetOutputPort());
  clip->SetClipFunction(plane);
  clip->GenerateClipScalarsOn();
  clip->SetValue(0.5);

  vtkPVGeometryFilter* geometry = vtkPVGeometryFilter::New();
  geometry->SetInputConnection(contour->GetOutputPort());

  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(geometry->GetOutputPort());

  vtkPVLODActor* actor = vtkPVLODActor::New();
  actor->SetMapper(mapper);

  // Now for the PVPolyData part:
  vtkRibbonFilter* ribbon = vtkRibbonFilter::New();
  ribbon->SetInputConnection(contour->GetOutputPort());
  ribbon->SetWidth(0.1);
  ribbon->SetWidthFactor(5);

  vtkThreshold* threshold = vtkThreshold::New();
  threshold->SetInputConnection(ribbon->GetOutputPort());
  threshold->ThresholdBetween(0.25, 0.75);

  vtkWarpScalar* warp = vtkWarpScalar::New();
  warp->SetInputConnection(threshold->GetOutputPort());
  warp->XYPlaneOn();
  warp->SetScaleFactor(0.5);

  vtkMergeArrays* merge = vtkMergeArrays::New();
  merge->AddInputConnection(warp->GetOutputPort());
  merge->AddInputConnection(clip->GetOutputPort());
  merge->Update(); // discard

  vtkDataSetMapper* warpMapper = vtkDataSetMapper::New();
  warpMapper->SetInputConnection(warp->GetOutputPort());

  vtkActor* warpActor = vtkActor::New();
  warpActor->SetMapper(warpMapper);

  vtkRenderer* ren = vtkRenderer::New();
  ren->AddActor(actor);
  ren->AddActor(warpActor);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);

  renWin->Render();

  reader->Delete();
  clean->Delete();
  gs->Delete();
  contour->Delete();
  plane->Delete();
  clip->Delete();
  geometry->Delete();
  mapper->Delete();
  actor->Delete();

  ribbon->Delete();
  threshold->Delete();
  merge->Delete();
  warp->Delete();
  warpMapper->Delete();
  warpActor->Delete();

  ren->Delete();
  renWin->Delete();

  return 0;
}
