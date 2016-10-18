/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeometryRepresentationWithHiddenLinesRemoval.h"

#include "vtkCompositePolyDataMapper2.h"
#include "vtkDefaultPainter.h"
#include "vtkObjectFactory.h"
#include "vtkVisibleLinesPainter.h"

vtkStandardNewMacro(vtkGeometryRepresentationWithHiddenLinesRemoval);
//----------------------------------------------------------------------------
vtkGeometryRepresentationWithHiddenLinesRemoval::vtkGeometryRepresentationWithHiddenLinesRemoval()
{
  if (this->Mapper)
  {
    // Replace the representation painter to the vtkVisibleLinesPainter,
    // so when rendering as wireframe, we remove the hidden lines.

    vtkCompositePolyDataMapper2* compositeMapper =
      vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper);

    vtkVisibleLinesPainter* painter = vtkVisibleLinesPainter::New();
    vtkDefaultPainter* dfPainter = vtkDefaultPainter::SafeDownCast(compositeMapper->GetPainter());
    dfPainter->SetRepresentationPainter(painter);
    painter->Delete();

    // Disable display lists, since vtkVisibleLinesPainter is not designed to work
    // with display lists.
    dfPainter->SetDisplayListPainter(0);
  }

  if (this->LODMapper)
  {
    // Replace the representation painter to the vtkVisibleLinesPainter,
    // so when rendering as wireframe, we remove the hidden lines.

    vtkCompositePolyDataMapper2* compositeMapper =
      vtkCompositePolyDataMapper2::SafeDownCast(this->LODMapper);

    vtkVisibleLinesPainter* painter = vtkVisibleLinesPainter::New();
    vtkDefaultPainter* dfPainter = vtkDefaultPainter::SafeDownCast(compositeMapper->GetPainter());
    dfPainter->SetRepresentationPainter(painter);
    painter->Delete();

    // Disable display lists, since vtkVisibleLinesPainter is not designed to work
    // with display lists.
    dfPainter->SetDisplayListPainter(0);
  }
}

//----------------------------------------------------------------------------
vtkGeometryRepresentationWithHiddenLinesRemoval::~vtkGeometryRepresentationWithHiddenLinesRemoval()
{
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentationWithHiddenLinesRemoval::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
