/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCornerAnnotation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkCornerAnnotation.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkCornerAnnotation* vtkCornerAnnotation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCornerAnnotation");
  if(ret)
    {
    return (vtkCornerAnnotation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCornerAnnotation;
}




vtkCornerAnnotation::vtkCornerAnnotation()
{
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.2,0.85);

  this->LastSize[0] = 0;
  this->LastSize[1] = 0;

  this->MaximumLineHeight = 1.0;
  this->MinimumFontSize = 6;
  
  for (int i = 0; i < 4; i++)
    {
    this->TextMapper[i] = vtkTextMapper::New();
    this->TextMapper[i]->SetFontSize(15);  
    this->TextMapper[i]->ShadowOff();  
    this->TextActor[i] = vtkActor2D::New();
    this->TextActor[i]->SetMapper(this->TextMapper[i]);
    }
  
  this->TextMapper[0]->SetJustificationToLeft();
  this->TextMapper[0]->SetVerticalJustificationToBottom();

  this->TextMapper[1]->SetJustificationToRight();
  this->TextMapper[1]->SetVerticalJustificationToBottom();
  
  this->TextMapper[2]->SetJustificationToLeft();
  this->TextMapper[2]->SetVerticalJustificationToTop();
  
  this->TextMapper[3]->SetJustificationToRight();
  this->TextMapper[3]->SetVerticalJustificationToTop();
}

vtkCornerAnnotation::~vtkCornerAnnotation()
{
  for (int i = 0; i < 4; i++)
    {
    this->TextMapper[i]->Delete();
    this->TextActor[i]->Delete();
    }
}

// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkCornerAnnotation::ReleaseGraphicsResources(vtkWindow *win)
{
  this->vtkActor2D::ReleaseGraphicsResources(win);
  for (int i = 0; i < 4; i++)
    {
    this->TextActor[i]->ReleaseGraphicsResources(win);
    }
}

int vtkCornerAnnotation::RenderOverlay(vtkViewport *viewport)
{
  // Everything is built, just have to render
  // only render if font is at least minimum font
  if (this->FontSize >= this->MinimumFontSize)
    {
    for (int i = 0; i < 4; i++)
      {
      this->TextActor[i]->RenderOverlay(viewport);
      }
    }
  return 1;
}

int vtkCornerAnnotation::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int fontSize;
  int i;
  
  // Check to see whether we have to rebuild everything
  if (viewport->GetMTime() > this->BuildTime ||
      ( viewport->GetVTKWindow() && 
        viewport->GetVTKWindow()->GetMTime() > this->BuildTime ) )
    {
    // if the viewport has changed we may - or may not need
    // to rebuild, it depends on if the projected coords chage
    int *vSize = viewport->GetSize();
    if (this->LastSize[0] != vSize[0] || this->LastSize[1] != vSize[1])
      {
      this->Modified();
      }
    }
  
  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime)
    {
    int *vSize = viewport->GetSize();
	int maxX, maxY;
    vtkDebugMacro(<<"Rebuilding text");
    
    // get the viewport size in display coordinates
    this->LastSize[0] = vSize[0];
    this->LastSize[1] = vSize[1];

    // Update all the composing objects tofind the best size for the font
    // use the last size as a first guess
    int tempi[8];
    fontSize = this->TextMapper[0]->GetFontSize();
    for (i = 0; i < 4; i++)
      {
      this->TextMapper[i]->GetSize(viewport,tempi+i*2);
      }
    maxY = (tempi[1] + tempi[5]) > (tempi[3] + tempi[7]) ?
      tempi[1] + tempi[5] : tempi[3] + tempi[7];
    maxX = (tempi[0] + tempi[2]) > (tempi[4] + tempi[6]) ?
      tempi[0] + tempi[2] : tempi[4] + tempi[6];
      
    int numLines1 = this->TextMapper[0]->GetNumberOfLines() + 
      this->TextMapper[2]->GetNumberOfLines();
    int numLines2 = this->TextMapper[1]->GetNumberOfLines() + 
      this->TextMapper[3]->GetNumberOfLines();
    
    int lineMax = (int)(vSize[1]*this->MaximumLineHeight) * 
                        (numLines1 > numLines2 ? numLines1 : numLines2);
    
    // target size is to use 90% of x and y
    int tSize[2];
    tSize[0] = (int)(0.9*vSize[0]);
    tSize[1] = (int)(0.9*vSize[1]);    
    
    // while the size is too small increase it
    while (maxY < tSize[1] && 
           maxX < tSize[0] && 
           maxY < lineMax &&
           fontSize < 100)
      {
      fontSize++;
      for (i = 0; i < 4; i++)
        {
        this->TextMapper[i]->SetFontSize(fontSize);
        this->TextMapper[i]->GetSize(viewport,tempi+i*2);
        }
      maxY = (tempi[1] + tempi[5]) > (tempi[3] + tempi[7]) ?
        tempi[1] + tempi[5] : tempi[3] + tempi[7];
      maxX = (tempi[0] + tempi[2]) > (tempi[4] + tempi[6]) ?
        tempi[0] + tempi[2] : tempi[4] + tempi[6];
      }
    // while the size is too large decrease it
    while ((maxY > tSize[1] || maxX > tSize[0] || 
            maxY > lineMax) && fontSize > 0)
      {
      fontSize--;
      for (i = 0; i < 4; i++)
        {
        this->TextMapper[i]->SetFontSize(fontSize);
        this->TextMapper[i]->GetSize(viewport,tempi+i*2);
        }
      maxY = (tempi[1] + tempi[5]) > (tempi[3] + tempi[7]) ?
        tempi[1] + tempi[5] : tempi[3] + tempi[7];
      maxX = (tempi[0] + tempi[2]) > (tempi[4] + tempi[6]) ?
        tempi[0] + tempi[2] : tempi[4] + tempi[6];
      }
    this->FontSize = fontSize;
    
    // now set the position of the TextActors
    this->TextActor[0]->SetPosition(5,5);
    this->TextActor[1]->SetPosition(vSize[0]-5,5);
    this->TextActor[2]->SetPosition(5,vSize[1]-5);
    this->TextActor[3]->SetPosition(vSize[0] - 5, vSize[1] - 5);

    for (i = 0; i < 4; i++)
      {
      this->TextActor[i]->SetProperty(this->GetProperty());
      }
    this->BuildTime.Modified();
    }

  // Everything is built, just have to render
  if (this->FontSize >= this->MinimumFontSize)
    {
    for (i = 0; i < 4; i++)
      {
      this->TextActor[i]->RenderOpaqueGeometry(viewport);
      }
    }
  return 1;
}

void vtkCornerAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor2D::PrintSelf(os,indent);

  os << indent << "MaximumLineHeight: " << this->MaximumLineHeight << endl;
}

void vtkCornerAnnotation::SetText(int i, const char *text)
{
  this->TextMapper[i]->SetInput(text);
  this->Modified();
}
