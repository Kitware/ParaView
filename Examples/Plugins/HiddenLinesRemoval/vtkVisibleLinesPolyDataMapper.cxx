/*=========================================================================

  Program:   ParaView
  Module:    vtkVisibleLinesPolyDataMapper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVisibleLinesPolyDataMapper.h"

#include "vtkDefaultPainter.h"
#include "vtkObjectFactory.h"
#include "vtkVisibleLinesPainter.h"

vtkStandardNewMacro(vtkVisibleLinesPolyDataMapper);
//----------------------------------------------------------------------------
vtkVisibleLinesPolyDataMapper::vtkVisibleLinesPolyDataMapper()
{
  // Replace the representation painter to the vtkVisibleLinesPainter,
  // so when rendering as wireframe, we remove the hidden lines.
  vtkVisibleLinesPainter* painter = vtkVisibleLinesPainter::New();

  vtkDefaultPainter* dfPainter = vtkDefaultPainter::SafeDownCast(this->GetPainter());
  dfPainter->SetRepresentationPainter(painter);
  painter->Delete();

  // Disable display lists, since vtkVisibleLinesPainter is not designed to work
  // with display lists.
  dfPainter->SetDisplayListPainter(0);
}

//----------------------------------------------------------------------------
vtkVisibleLinesPolyDataMapper::~vtkVisibleLinesPolyDataMapper()
{
}

//----------------------------------------------------------------------------
void vtkVisibleLinesPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
