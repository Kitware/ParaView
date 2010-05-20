/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSpriteDefaultPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPointSpriteDefaultPainter
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

#include "vtkPointSpriteDefaultPainter.h"

#include "vtkObjectFactory.h"
#include "vtkScalarsToColorsPainter.h"
#include "vtkGarbageCollector.h"
#include "vtkTwoScalarsToColorsPainter.h"
#include "vtkClipPlanesPainter.h"
#include "vtkPointsPainter.h"
#include "vtkStandardPolyDataPainter.h"
#include "vtkPointSpriteCoincidentTopologyResolutionPainter.h"
#include "vtkDepthSortPainter.h"
#include "vtkMapper.h"

vtkStandardNewMacro(vtkPointSpriteDefaultPainter)

vtkCxxSetObjectMacro(vtkPointSpriteDefaultPainter, DepthSortPainter, vtkDepthSortPainter)

vtkPointSpriteDefaultPainter::vtkPointSpriteDefaultPainter()
{
  this->DepthSortPainter = vtkDepthSortPainter::New();

  // create a vtkTwoScalarsToColorsPainter instead of the usual vtkScalarsToColorsPainter
  // This painter can blend the colors with a second array for opacity
  vtkTwoScalarsToColorsPainter* twoScalarsToColorsPainter = vtkTwoScalarsToColorsPainter::New();
  this->SetScalarsToColorsPainter(twoScalarsToColorsPainter);
  twoScalarsToColorsPainter->Delete();

  // create a vtkPointSpriteCoincidentTopologyResolutionPainter instead of the usual vtkOpenGLCoincidentTopologyResolutionPainter
  vtkPointSpriteCoincidentTopologyResolutionPainter* topologyPainter = vtkPointSpriteCoincidentTopologyResolutionPainter::New();
  this->SetCoincidentTopologyResolutionPainter(topologyPainter);
  topologyPainter->Delete();
}

vtkPointSpriteDefaultPainter::~vtkPointSpriteDefaultPainter()
{
  this->SetDepthSortPainter(NULL);
}

void vtkPointSpriteDefaultPainter::BuildPainterChain()
{
  // build the superclass painter chain
  this->Superclass::BuildPainterChain();

  // insert the point sprite painter chain :
  // ScalarsToColorsPainter -> DepthSortPainter -> ...
  this->DepthSortPainter->SetDelegatePainter( this->ScalarsToColorsPainter->GetDelegatePainter());
  this->ScalarsToColorsPainter->SetDelegatePainter( this->DepthSortPainter);
}

void vtkPointSpriteDefaultPainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->DepthSortPainter,
      "DepthSortPainter");
}

void vtkPointSpriteDefaultPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DepthSortPainter: "
      << this->DepthSortPainter << endl;
}
