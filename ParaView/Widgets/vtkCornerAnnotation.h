/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCornerAnnotation.h
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
// .NAME vtkCornerAnnotation - text annotation in four corners
// .SECTION Description
// This is an annotation object that manages four text actors / mappers
// to provide annotation in the four corners of a viewport
//
// .SECTION See Also
// vtkActor2D vtkTextMapper

#ifndef __vtkCornerAnnotation_h
#define __vtkCornerAnnotation_h

#include "vtkActor2D.h"

class vtkTextMapper;
class vtkImageMapToWindowLevelColors;
class vtkImageActor;

class VTK_EXPORT vtkCornerAnnotation : public vtkActor2D
{
public:
  vtkTypeMacro(vtkCornerAnnotation,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with a rectangle in normaled view coordinates
  // of (0.2,0.85, 0.8, 0.95).
  static vtkCornerAnnotation *New();
  
  // Description:
  // Draw the scalar bar and annotation text to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  int RenderTranslucentGeometry(vtkViewport* ) {return 0;};
  int RenderOverlay(vtkViewport* viewport);

  // Description:
  // Set/Get the maximum height of a line of text as a 
  // percentage of the vertical area allocated to this
  // scaled text actor. Defaults to 1.0
  vtkSetMacro(MaximumLineHeight,float);
  vtkGetMacro(MaximumLineHeight,float);
  
  // Description:
  // Set/Get the minimum size font that will be shown.
  // If the font drops below this size it will not be rendered.
  vtkSetMacro(MinimumFontSize,int);
  vtkGetMacro(MinimumFontSize,int);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Set the text to be displayed for each corner
  void SetText(int i,const char *text);

  // Description:
  // Set an image actor to look at for slice information
  void SetImageActor(vtkImageActor*);
  vtkGetObjectMacro(ImageActor,vtkImageActor);
  
  // Description:
  // Set an instance of vtkImageMapToWindowLevelColors to use for
  // looking at window level changes
  void SetWindowLevel(vtkImageMapToWindowLevelColors*);
  vtkGetObjectMacro(WindowLevel,vtkImageMapToWindowLevelColors);
  
protected:
  vtkCornerAnnotation();
  ~vtkCornerAnnotation();

  float MaximumLineHeight;

  vtkImageMapToWindowLevelColors *WindowLevel;
  vtkImageActor *ImageActor;
  vtkImageActor *LastImageActor;

  char *CornerText[4];
  
  int FontSize;
  vtkActor2D    *TextActor[4];
  vtkTimeStamp   BuildTime;
  int            LastSize[2];
  vtkTextMapper *TextMapper[4];
  int MinimumFontSize;
  
  // search for replacable tokens and replace
  void ReplaceText(vtkImageActor *ia,  vtkImageMapToWindowLevelColors *wl);
private:
  vtkCornerAnnotation(const vtkCornerAnnotation&);  // Not implemented.
  void operator=(const vtkCornerAnnotation&);  // Not implemented.
};


#endif

