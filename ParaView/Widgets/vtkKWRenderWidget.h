/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRenderWidget.h
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
// .NAME vtkKWRenderWidget
// .SECTION Description

#ifndef __vtkKWRenderWidget_h
#define __vtkKWRenderWidget_h

#define VTK_KW_INTERACTIVE_RENDER  0
#define VTK_KW_STILL_RENDER        1
#define VTK_KW_DISABLED_RENDER     2
#define VTK_KW_SINGLE_RENDER       3

#include "vtkKWWidget.h"

class vtkKWEventMap;
class vtkKWGenericRenderWindowInteractor;
class vtkKWWindow;
class vtkCornerAnnotation;
class vtkProp;
class vtkRenderer;
class vtkRenderWindow;

class VTK_EXPORT vtkKWRenderWidget : public vtkKWWidget
{
public:
  vtkTypeRevisionMacro(vtkKWRenderWidget, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  virtual void Create(vtkKWApplication *app, const char *args);

  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  
  virtual void SetupBindings();
  virtual void RemoveBindings();
  
  virtual void MouseMove(int num, int x, int y);
  virtual void AButtonPress(int num, int x, int y, int ctrl, int shift);
  virtual void AButtonRelease(int num, int x, int y);
  virtual void AKeyPress(char key, int x, int y, int ctrl, int shift);
  virtual void Exposed();
  virtual void Configure(int width, int height);
  
  virtual void Render();
  virtual void Reset() = 0;
  
  vtkGetMacro(RenderState, int);
  vtkSetClampMacro(RenderState, int, 0, 1);
  vtkBooleanMacro(RenderState, int);
  
  vtkSetClampMacro( RenderMode, int, 
                    VTK_KW_INTERACTIVE_RENDER,
                    VTK_KW_SINGLE_RENDER );
  vtkGetMacro( RenderMode, int );
  void SetRenderModeToInteractive() 
    { this->RenderMode = VTK_KW_INTERACTIVE_RENDER; };
  void SetRenderModeToStill() 
    { this->RenderMode = VTK_KW_STILL_RENDER; };
  void SetRenderModeToSingle() 
    { this->RenderMode = VTK_KW_SINGLE_RENDER; };
  void SetRenderModeToDisabled() 
    { this->RenderMode = VTK_KW_DISABLED_RENDER; };

#ifdef _WIN32
  void SetupPrint(RECT &rcDest, HDC ghdc,
                  int printerPageSizeX, int printerPageSizeY,
                  int printerDPIX, int printerDPIY,
                  float scaleX, float scaleY,
                  int screenSizeX, int screenSizeY);
#endif
  vtkSetMacro(Printing, int);
  vtkGetMacro(Printing, int);
  
  void SetParentWindow(vtkKWWindow *window);
  vtkGetObjectMacro(ParentWindow, vtkKWWindow);

  virtual void SetupMemoryRendering(int width, int height, void *cd);
  virtual void ResumeScreenRendering();
  virtual void* GetMemoryDC();
  
  vtkGetObjectMacro(VTKWidget, vtkKWWidget);

  virtual void AddProp(vtkProp *prop);
  virtual int  HasProp(vtkProp *prop);
  virtual void RemoveProp(vtkProp *prop);
  virtual void RemoveAllProps();
  
  virtual void SetBackgroundColor(float r, float g, float b);
  virtual void SetBackgroundColor(float *rgb)
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); }
  virtual void GetBackgroundColor(float *r, float *g, float *b);
  virtual float* GetBackgroundColor();

  virtual void Close();
  
  // Description:
  // Get the underlying vtkCornerAnnotation.
  vtkGetObjectMacro(CornerAnnotation, vtkCornerAnnotation);

  void SetCornerTextColor(float r, float g, float b);
  void SetCornerTextColor(float *rgb)
    { this->SetCornerTextColor(rgb[0], rgb[1], rgb[2]); }
  float* GetCornerTextColor();
      
protected:
  vtkKWRenderWidget();
  ~vtkKWRenderWidget();
  
  vtkKWWidget *VTKWidget;

  vtkCornerAnnotation *CornerAnnotation;

  vtkRenderer     *Renderer;
  vtkRenderWindow *RenderWindow;
  vtkKWWindow     *ParentWindow;
  
  vtkKWGenericRenderWindowInteractor *Interactor;
  vtkKWEventMap *EventMap;
  
  int InExpose;

  int RenderState;
  int RenderMode;
  int Printing;
  
private:
  vtkKWRenderWidget(const vtkKWRenderWidget&);  // Not implemented
  void operator=(const vtkKWRenderWidget&);  // Not implemented
};

#endif
