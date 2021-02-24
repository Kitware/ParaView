/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractiveViewLinkRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInteractiveViewLinkRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

#include <cmath>

vtkStandardNewMacro(vtkPVInteractiveViewLinkRepresentation);

//----------------------------------------------------------------------------
vtkPVInteractiveViewLinkRepresentation::vtkPVInteractiveViewLinkRepresentation() = default;

//----------------------------------------------------------------------------
vtkPVInteractiveViewLinkRepresentation::~vtkPVInteractiveViewLinkRepresentation() = default;

//----------------------------------------------------------------------------
void vtkPVInteractiveViewLinkRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------
void vtkPVInteractiveViewLinkRepresentation::WidgetInteraction(double eventPos[2])
{
  double posX = eventPos[0];
  double posY = eventPos[1];

  // convert to normalized viewport coordinates
  this->Renderer->DisplayToNormalizedDisplay(posX, posY);
  this->Renderer->NormalizedDisplayToViewport(posX, posY);
  this->Renderer->ViewportToNormalizedViewport(posX, posY);

  // there are four parameters that can be adjusted
  double* fpos1 = this->PositionCoordinate->GetValue();
  double* fpos2 = this->Position2Coordinate->GetValue();
  double par1[2];
  double par2[2];
  par1[0] = fpos1[0];
  par1[1] = fpos1[1];
  par2[0] = fpos1[0] + fpos2[0];
  par2[1] = fpos1[1] + fpos2[1];

  double delX = posX - this->StartEventPosition[0];
  double delY = posY - this->StartEventPosition[1];
  double delX2 = 0.0, delY2 = 0.0;

  // Based on the state, adjust the representation. Note that we force a
  // uniform scaling of the widget when tugging on the corner points (and
  // when proportional resize is on). This is done by finding the maximum
  // movement in the x-y directions and using this to scale the widget.
  if (this->ProportionalResize && !this->Moving)
  {
    double sx = fpos2[0] / fpos2[1];
    double sy = fpos2[1] / fpos2[0];
    if (fabs(delX) > fabs(delY))
    {
      delY = sy * delX;
      delX2 = delX;
      delY2 = -delY;
    }
    else
    {
      delX = sx * delY;
      delY2 = delY;
      delX2 = -delX;
    }
  }
  else
  {
    delX2 = delX;
    delY2 = delY;
  }

  // The previous "if" statement has taken care of the proportional resize
  // for the most part. However, tugging on edges has special behavior, which
  // is to scale the box about its center.
  // Prevent the widget for going over the edges of the viewport
  switch (this->InteractionState)
  {
    case vtkBorderRepresentation::AdjustingP0:
      if (par1[0] + delX < 0)
      {
        delX = -par1[0];
      }
      if (par1[1] + delY < 0)
      {
        delY = -par1[1];
      }
      par1[0] = par1[0] + delX;
      par1[1] = par1[1] + delY;
      break;
    case vtkBorderRepresentation::AdjustingP1:
      if (par2[0] + delX2 > 1)
      {
        delX2 = 1 - par2[0];
      }
      if (par1[1] + delY2 < 0)
      {
        delY2 = -par1[1];
      }

      par2[0] = par2[0] + delX2;
      par1[1] = par1[1] + delY2;
      break;
    case vtkBorderRepresentation::AdjustingP2:
      if (par2[0] + delX > 1)
      {
        delX = 1 - par2[0];
      }
      if (par2[1] + delY > 1)
      {
        delY = 1 - par2[1];
      }

      par2[0] = par2[0] + delX;
      par2[1] = par2[1] + delY;
      break;
    case vtkBorderRepresentation::AdjustingP3:
      if (par1[0] + delX2 < 0)
      {
        delX2 = -par1[0];
      }
      if (par2[1] + delY2 > 1)
      {
        delY2 = 1 - par2[1];
      }

      par1[0] = par1[0] + delX2;
      par2[1] = par2[1] + delY2;
      break;
    case vtkBorderRepresentation::AdjustingE0:
      if (par1[1] + delY < 0)
      {
        delY = -par1[1];
      }
      if (this->ProportionalResize)
      {
        if (par2[1] - delY > 1)
        {
          delY = par2[1] - 1;
        }
        if (par1[0] + delX < 0)
        {
          delX = -par1[0];
        }
        if (par2[0] - delX < 0)
        {
          delX = par1[0];
        }
      }
      par1[1] = par1[1] + delY;
      if (this->ProportionalResize)
      {
        par2[1] = par2[1] - delY;
        par1[0] = par1[0] + delX;
        par2[0] = par2[0] - delX;
      }
      break;
    case vtkBorderRepresentation::AdjustingE1:
      if (par2[0] + delX > 1)
      {
        delX = 1 - par2[0];
      }
      if (this->ProportionalResize)
      {
        if (par1[0] - delX < 0)
        {
          delX = par1[0];
        }
        if (par1[1] - delY < 0)
        {
          delY = par1[1];
        }
        if (par2[1] + delY > 1)
        {
          delY = 1 - par2[1];
        }
      }

      par2[0] = par2[0] + delX;
      if (this->ProportionalResize)
      {
        par1[0] = par1[0] - delX;
        par1[1] = par1[1] - delY;
        par2[1] = par2[1] + delY;
      }
      break;
    case vtkBorderRepresentation::AdjustingE2:
      if (par2[1] + delY > 1)
      {
        delY = 1 - par2[1];
      }
      if (this->ProportionalResize)
      {
        if (par1[1] - delY < 0)
        {
          delY = par1[1];
        }
        if (par1[0] - delX < 0)
        {
          delX = par1[0];
        }
        if (par2[0] + delX > 1)
        {
          delX = 1 - par2[0];
        }
      }

      par2[1] = par2[1] + delY;
      if (this->ProportionalResize)
      {
        par1[1] = par1[1] - delY;
        par1[0] = par1[0] - delX;
        par2[0] = par2[0] + delX;
      }
      break;
    case vtkBorderRepresentation::AdjustingE3:
      if (par1[0] + delX < 0)
      {
        delX = -par1[0];
      }
      if (this->ProportionalResize)
      {
        if (par2[0] - delX < 0)
        {
          delX = par1[0];
        }
        if (par1[1] + delY < 0)
        {
          delY = -par1[1];
        }
        if (par2[1] - delY > 1)
        {
          delY = par2[1] - 1;
        }
      }

      par1[0] = par1[0] + delX;
      if (this->ProportionalResize)
      {
        par2[0] = par2[0] - delX;
        par1[1] = par1[1] + delY;
        par2[1] = par2[1] - delY;
      }
      break;
    case vtkBorderRepresentation::Inside:
      if (this->Moving)
      {
        if (par1[0] + delX < 0)
        {
          delX = -par1[0];
        }
        if (par2[0] + delX > 1)
        {
          delX = 1 - par2[0];
        }
        if (par1[1] + delY < 0)
        {
          delY = -par1[1];
        }
        if (par2[1] + delY > 1)
        {
          delY = 1 - par2[1];
        }
        par1[0] = par1[0] + delX;
        par1[1] = par1[1] + delY;
        par2[0] = par2[0] + delX;
        par2[1] = par2[1] + delY;
      }
      break;
  }

  // Modify the representation
  if (par2[0] > par1[0] && par2[1] > par1[1])
  {
    this->PositionCoordinate->SetValue(par1[0], par1[1]);
    this->Position2Coordinate->SetValue(par2[0] - par1[0], par2[1] - par1[1]);
    this->StartEventPosition[0] = posX;
    this->StartEventPosition[1] = posY;
  }

  this->Modified();
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
inline void vtkPVInteractiveViewLinkRepresentation::AdjustImageSize(
  double vtkNotUsed(o)[2], double borderSize[2], double imageSize[2])
{
  // Force image to fit
  imageSize[0] = borderSize[0];
  imageSize[1] = borderSize[1];
}
