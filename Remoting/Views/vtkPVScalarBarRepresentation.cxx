/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkPVScalarBarRepresentation.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkViewport.h"

#include "vtkContext2DScalarBarActor.h"

//#define DEBUG_BOUNDING_BOX
#if defined(DEBUG_BOUNDING_BOX)
#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkContextDevice2D.h"
#include "vtkPen.h"
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVScalarBarRepresentation);

//-----------------------------------------------------------------------------
vtkPVScalarBarRepresentation::vtkPVScalarBarRepresentation()
{
  this->WindowLocation = vtkPVScalarBarRepresentation::AnyLocation;
}

//-----------------------------------------------------------------------------
vtkPVScalarBarRepresentation::~vtkPVScalarBarRepresentation()
{
}

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

#if defined(DEBUG_BOUNDING_BOX)
  vtkNew<vtkContext2D> context;
  vtkNew<vtkContextDevice2D> contextDevice;
  contextDevice->Begin(viewport);
  context->Begin(contextDevice.Get());
  vtkPen* pen = context->GetPen();
  pen->SetColor(255, 255, 255);
  vtkBrush* brush = context->GetBrush();
  brush->SetOpacityF(0.0);
  double xx = this->PositionCoordinate->GetValue()[0];
  double yy = this->PositionCoordinate->GetValue()[1];
  viewport->NormalizedViewportToViewport(xx, yy);
  viewport->ViewportToNormalizedDisplay(xx, yy);
  viewport->NormalizedDisplayToDisplay(xx, yy);
  context->DrawRect(xx + boundingRect.GetX(), yy + boundingRect.GetY(), boundingRect.GetWidth(),
    boundingRect.GetHeight());
  context->End();
  contextDevice->End();
#endif

  // Start with Lower Right corner.
  int* displaySize = viewport->GetSize();

  if (this->WindowLocation != AnyLocation)
  {
    double pad = 4.0;
    double x = 0.0;
    double y = 0.0;
    switch (this->WindowLocation)
    {
      case LowerLeftCorner:
        x = 0.0 + pad;
        y = 0.0 + pad;
        break;

      case LowerRightCorner:
        x = displaySize[0] - 1.0 - boundingRect.GetWidth() - pad;
        y = 0.0 + pad;
        break;

      case LowerCenter:
        x = 0.5 * (displaySize[0] - boundingRect.GetWidth());
        y = 0.0 + pad;
        break;

      case UpperLeftCorner:
        x = 0.0 + pad;
        y = displaySize[1] - 1.0 - boundingRect.GetHeight() - pad;
        break;

      case UpperRightCorner:
        x = displaySize[0] - 1.0 - boundingRect.GetWidth() - pad;
        y = displaySize[1] - 1.0 - boundingRect.GetHeight() - pad;
        break;

      case UpperCenter:
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

//-----------------------------------------------------------------------------
void vtkPVScalarBarRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "WindowLocation: ";
  switch (this->WindowLocation)
  {
    case AnyLocation:
      os << "AnyLocation";
      break;

    case LowerLeftCorner:
      os << "LowerLeftCorner";
      break;

    case LowerRightCorner:
      os << "LowerRightCorner";
      break;

    case LowerCenter:
      os << "LowerCenter";
      break;

    case UpperLeftCorner:
      os << "UpperLeftCorner";
      break;

    case UpperRightCorner:
      os << "UpperRightCorner";
      break;

    case UpperCenter:
      os << "UpperCenter";
      break;

    default:
      // Do nothing
      break;
  }
  os << endl;
}
