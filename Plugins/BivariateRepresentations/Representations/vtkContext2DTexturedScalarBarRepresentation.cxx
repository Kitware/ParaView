// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkContext2DTexturedScalarBarRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkViewport.h"

#include "vtkContext2DTexturedScalarBarActor.h"

/**
 * NOTE FOR DEVELOPERS
 * This class can be merged with vtkPVScalarBarRepresentation
 * if we create a superclass for all context 2D scalar bar actors.
 *
 * See vtkContext2DScalarBarActor & vtkContext2DTexturedScalarBarActor
 */

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkContext2DTexturedScalarBarRepresentation);

//-----------------------------------------------------------------------------
int vtkContext2DTexturedScalarBarRepresentation::RenderOverlay(vtkViewport* viewport)
{
  // Query scalar bar size given the viewport
  vtkContext2DTexturedScalarBarActor* actor =
    vtkContext2DTexturedScalarBarActor::SafeDownCast(this->GetScalarBarActor());
  if (!actor)
  {
    vtkErrorMacro(<< "Actor expected to be of type vtkContext2DTexturedScalarBarActor");
    return 0;
  }

  vtkRectf boundingRect = actor->GetBoundingRect();

  // Start with Lower Right corner.
  int* displaySize = viewport->GetSize();

  if (this->WindowLocation != vtkBorderRepresentation::AnyLocation)
  {
    double pad = 4.0;
    double x = 0.0;
    double y = 0.0;
    switch (this->WindowLocation)
    {
      case vtkBorderRepresentation::LowerLeftCorner:
        x = 0.0 + pad;
        y = 0.0 + pad;
        break;

      case vtkBorderRepresentation::LowerRightCorner:
        x = displaySize[0] - 1.0 - boundingRect.GetWidth() - pad;
        y = 0.0 + pad;
        break;

      case vtkBorderRepresentation::LowerCenter:
        x = 0.5 * (displaySize[0] - boundingRect.GetWidth());
        y = 0.0 + pad;
        break;

      case vtkBorderRepresentation::UpperLeftCorner:
        x = 0.0 + pad;
        y = displaySize[1] - 1.0 - boundingRect.GetHeight() - pad;
        break;

      case vtkBorderRepresentation::UpperRightCorner:
        x = displaySize[0] - 1.0 - boundingRect.GetWidth() - pad;
        y = displaySize[1] - 1.0 - boundingRect.GetHeight() - pad;
        break;

      case vtkBorderRepresentation::UpperCenter:
        x = 0.5 * (displaySize[0] - boundingRect.GetWidth());
        y = displaySize[1] - 1.0 - boundingRect.GetHeight() - pad;

      default:
        break;
    }

    x -= boundingRect.GetX();
    y -= boundingRect.GetY();

    viewport->DisplayToNormalizedDisplay(x, y);

    this->PositionCoordinate->SetValue(x, y);
  }

  return this->Superclass::RenderOverlay(viewport);
}

//------------------------------------------------------------------------------
void vtkContext2DTexturedScalarBarRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
