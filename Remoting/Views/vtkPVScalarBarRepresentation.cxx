// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkPVScalarBarRepresentation.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkViewport.h"

#include "vtkContext2DScalarBarActor.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVScalarBarRepresentation);

//-----------------------------------------------------------------------------
int vtkPVScalarBarRepresentation::RenderOverlay(vtkViewport* viewport)
{
  // Query scalar bar size given the viewport
  vtkContext2DScalarBarActor* actor =
    vtkContext2DScalarBarActor::SafeDownCast(this->GetScalarBarActor());
  if (!actor)
  {
    vtkErrorMacro(<< "Actor expected to be of type vtkContext2DScalarBarActor");
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
void vtkPVScalarBarRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
