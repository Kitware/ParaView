/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCornerAnnotation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkCornerAnnotation.h"
#include "vtkObjectFactory.h"


//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkCornerAnnotation);

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
    this->CornerText[i] = NULL;
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

  this->ImageActor = NULL;
  this->LastImageActor = 0;
  this->WindowLevel = NULL;
}

vtkCornerAnnotation::~vtkCornerAnnotation()
{
  for (int i = 0; i < 4; i++)
    {
    delete [] this->CornerText[i];
    this->TextMapper[i]->Delete();
    this->TextActor[i]->Delete();
    }
  
  this->SetWindowLevel(NULL);
  this->SetImageActor(NULL);
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

void vtkCornerAnnotation::ReplaceText(vtkImageActor *ia,
                                      vtkImageMapToWindowLevelColors *wl)
{
  int i;
  char *text, *text2;
  int image=0;
  char *rpos, *tmp;
  float window=0, level=0;
    
  if (ia)
    {
    image = ia->GetSliceNumber();
    }
  if (wl)
    {
    window = wl->GetWindow();
    level = wl->GetLevel();    
    }
  
  // search for tokens, replace and then assign to TextMappers
  for (i = 0; i < 4; i++)
    {
    if (this->CornerText[i] && strlen(this->CornerText[i]))
      {
      text = new char [strlen(this->CornerText[i])+1000];
      text2 = new char [strlen(this->CornerText[i])+1000];
      strcpy(text,this->CornerText[i]);
      // now do the replacements
      rpos = strstr(text,"<image>");
      while (rpos)
        {
        *rpos = '\0';
        if (ia)
          {
          sprintf(text2,"%sImage: %i%s",text,image,rpos+7);
          }
        else
          {
          sprintf(text2,"%s%s",text,rpos+7);
          }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<image>");
        }
      rpos = strstr(text,"<slice>");
      while (rpos)
        {
        *rpos = '\0';
        if (ia)
          {
          sprintf(text2,"%sSlice: %i%s",text,image,rpos+7);
          }
        else
          {
          sprintf(text2,"%s%s",text,rpos+7);
          }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<slice>");
        }
      rpos = strstr(text,"<window>");
      while (rpos)
        {
        *rpos = '\0';
        if (ia)
          {
          sprintf(text2,"%sWindow: %.2f%s",text,window,rpos+8);
          }
        else
          {
          sprintf(text2,"%s%s",text,rpos+8);
          }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<window>");
        }
      rpos = strstr(text,"<level>");
      while (rpos)
        {
        *rpos = '\0';
        if (ia)
          {
          sprintf(text2,"%sLevel: %.2f%s",text,level,rpos+7);
          }
        else
          {
          sprintf(text2,"%s%s",text,rpos+7);
          }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<level>");
        }
      this->TextMapper[i]->SetInput(text);
      delete [] text;
      delete [] text2;
      }
    else
      {
      this->TextMapper[i]->SetInput("");
      }
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
  
  
  // is there an image actor ?
  vtkImageActor *ia = 0;  
  vtkImageMapToWindowLevelColors *wl = this->WindowLevel;
  vtkPropCollection *pc = viewport->GetProps();
  int numProps = pc->GetNumberOfItems();
  for (i = 0; i < numProps; i++)
    {
    ia = vtkImageActor::SafeDownCast(pc->GetItemAsObject(i));
    if (ia)
      {
      if (!wl)
        {
        wl = vtkImageMapToWindowLevelColors::SafeDownCast(
          ia->GetInput()->GetSource());
        }
      break;
      }
    }  

  // Check to see whether we have to rebuild everything
  if ( (this->GetMTime() > this->BuildTime) ||
       (ia && (ia != this->LastImageActor || 
               ia->GetMTime() > this->BuildTime)) ||
       (wl && wl->GetMTime() > this->BuildTime))
    {
    int *vSize = viewport->GetSize();
    int maxX, Y1, Y2;
    vtkDebugMacro(<<"Rebuilding text");
    
    // replace text
    this->ReplaceText(ia,wl);
    
    // get the viewport size in display coordinates
    this->LastSize[0] = vSize[0];
    this->LastSize[1] = vSize[1];

    // only adjust size then the text changes due to non w/l slice reasons
    if (this->GetMTime() > this->BuildTime)
      {
      // Update all the composing objects tofind the best size for the font
      // use the last size as a first guess
      int tempi[8];
      fontSize = this->TextMapper[0]->GetFontSize();
      for (i = 0; i < 4; i++)
        {
        this->TextMapper[i]->GetSize(viewport,tempi+i*2);
        }
      Y1 = tempi[1] + tempi[5];
      Y2 = tempi[3] + tempi[7];
      maxX = (tempi[0] + tempi[2]) > (tempi[4] + tempi[6]) ?
        tempi[0] + tempi[2] : tempi[4] + tempi[6];
      
      int numLines1 = this->TextMapper[0]->GetNumberOfLines() + 
        this->TextMapper[2]->GetNumberOfLines();
      int numLines2 = this->TextMapper[1]->GetNumberOfLines() + 
        this->TextMapper[3]->GetNumberOfLines();
      
      int lineMax1 = (int)(vSize[1]*this->MaximumLineHeight) * 
        (numLines1 ? numLines1 : 1);
      int lineMax2 = (int)(vSize[1]*this->MaximumLineHeight) * 
        (numLines2 ? numLines2 : 1);
      
      // target size is to use 90% of x and y
      int tSize[2];
      tSize[0] = (int)(0.9*vSize[0]);
      tSize[1] = (int)(0.9*vSize[1]);    
      
      // while the size is too small increase it
      while (Y1 < tSize[1] && 
             Y2 < tSize[1] &&
             maxX < tSize[0] &&
             Y1 < lineMax1 &&
             Y2 < lineMax2 &&
             fontSize < 100)
        {
        fontSize++;
        for (i = 0; i < 4; i++)
          {
          this->TextMapper[i]->SetFontSize(fontSize);
          this->TextMapper[i]->GetSize(viewport,tempi+i*2);
          }
        Y1 = tempi[1] + tempi[5];
        Y2 = tempi[3] + tempi[7];
        maxX = (tempi[0] + tempi[2]) > (tempi[4] + tempi[6]) ?
          tempi[0] + tempi[2] : tempi[4] + tempi[6];
        }
      // while the size is too large decrease it
      while ((Y1 > tSize[1] || Y2 > tSize[1] || maxX > tSize[0] ||
              Y1 > lineMax1 || Y2 > lineMax2) && fontSize > 0)
        {
        fontSize--;
        for (i = 0; i < 4; i++)
          {
          this->TextMapper[i]->SetFontSize(fontSize);
          this->TextMapper[i]->GetSize(viewport,tempi+i*2);
          }
        Y1 = tempi[1] + tempi[5];
        Y2 = tempi[3] + tempi[7];
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
      }
    this->BuildTime.Modified();
    this->LastImageActor = ia;
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
  if ( this->CornerText[i] && text && (!strcmp(this->CornerText[i],text))) 
    { 
    return;
    } 
  delete [] this->CornerText[i];
  this->CornerText[i] = new char [strlen(text)+1];
  strcpy(this->CornerText[i],text);
  this->Modified();
}
