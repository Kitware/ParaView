/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRenderWidget.cxx
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
#include "vtkKWRenderWidget.h"

#include "vtkCamera.h"
#include "vtkCornerAnnotation.h"
#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkKWEventMap.h"
#include "vtkKWGenericRenderWindowInteractor.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

vtkCxxRevisionMacro(vtkKWRenderWidget, "1.15");

vtkKWRenderWidget::vtkKWRenderWidget()
{
  this->VTKWidget = vtkKWWidget::New();
  this->VTKWidget->SetParent(this);
  
  this->Renderer = vtkRenderer::New();
  this->RenderWindow = vtkRenderWindow::New();
  this->RenderWindow->AddRenderer(this->Renderer);
  
  this->Printing = 0;
  this->RenderMode = VTK_KW_STILL_RENDER;
  this->RenderState = 1;
  
  this->ParentWindow = NULL;
  
  this->Interactor = vtkKWGenericRenderWindowInteractor::New();
  this->Interactor->SetRenderWidget(this);
  
  this->EventMap = vtkKWEventMap::New();
  
  this->InExpose = 0;

  this->CornerAnnotation = vtkCornerAnnotation::New();
  this->CornerAnnotation->SetMaximumLineHeight(0.07);
  this->CornerAnnotation->VisibilityOff();

  this->Units = NULL;

  this->CurrentCamera = this->Renderer->GetActiveCamera();
  this->CurrentCamera->ParallelProjectionOn();  

  this->ScalarShift = 0;
  this->ScalarScale = 1;  

  this->CollapsingRenders = 0;
}

vtkKWRenderWidget::~vtkKWRenderWidget()
{
  this->Renderer->Delete();
  this->RenderWindow->Delete();
  this->SetParentWindow(NULL);
  this->VTKWidget->Delete();
  
  this->Interactor->SetRenderWidget(NULL);
  this->Interactor->SetInteractorStyle(NULL);
  this->Interactor->Delete();
  this->EventMap->Delete();

  if (this->CornerAnnotation)
    {
    this->CornerAnnotation->Delete();
    this->CornerAnnotation = NULL;
    }
  
  this->SetUnits(NULL);
}

void vtkKWRenderWidget::Create(vtkKWApplication *app, const char *args)
{
  char *local;
  const char *wname;
  
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Render widget already created");
    return;
    }
  
  local = new char[strlen(args)+100];
  
  this->SetApplication(app);
  
  wname = this->GetWidgetName();
  this->Script("frame %s %s", wname, args);
  
  sprintf(local, "%s -rw Addr=%p", args, this->RenderWindow);
  this->Script("vtkTkRenderWidget %s %s",
               this->VTKWidget->GetWidgetName(), local);
  this->Script("pack %s -fill both -expand yes",
               this->VTKWidget->GetWidgetName());
  
  this->RenderWindow->Render();
  delete [] local;

  // Update enable state

  this->UpdateEnableState();
}

void vtkKWRenderWidget::SetupBindings()
{
  const char *wname = this->VTKWidget->GetWidgetName();
  const char *tname = this->GetTclName();

  // setup some default bindings
  this->Script("bind %s <Expose> {%s Exposed}",wname,tname);
  
  this->Script("bind %s <Any-ButtonPress> {%s AButtonPress %%b %%x %%y 0 0}",
               wname, tname);

  this->Script("bind %s <Any-ButtonRelease> {%s AButtonRelease %%b %%x %%y}",
               wname, tname);

  this->Script(
    "bind %s <Shift-Any-ButtonPress> {%s AButtonPress %%b %%x %%y 0 1}",
    wname, tname);

  this->Script(
    "bind %s <Shift-Any-ButtonRelease> {%s AButtonRelease %%b %%x %%y}",
    wname, tname);

  this->Script(
    "bind %s <Control-Any-ButtonPress> {%s AButtonPress %%b %%x %%y 1 0}",
    wname, tname);

  this->Script(
    "bind %s <Control-Any-ButtonRelease> {%s AButtonRelease %%b %%x %%y}",
    wname, tname);

  this->Script("bind %s <B1-Motion> {%s MouseMove 1 %%x %%y}",
               wname, tname);

  this->Script("bind %s <B2-Motion> {%s MouseMove 2 %%x %%y}", 
               wname, tname);
  
  this->Script("bind %s <B3-Motion> {%s MouseMove 3 %%x %%y}", 
               wname, tname);

  this->Script("bind %s <Shift-B1-Motion> {%s MouseMove 1 %%x %%y}", 
               wname, tname);

  this->Script("bind %s <Shift-B2-Motion> {%s MouseMove 2 %%x %%y}", 
               wname, tname);
  
  this->Script("bind %s <Shift-B3-Motion> {%s MouseMove 3 %%x %%y}", 
               wname, tname);

  this->Script("bind %s <Control-B1-Motion> {%s MouseMove 1 %%x %%y}",
               wname, tname);

  this->Script("bind %s <Control-B2-Motion> {%s MouseMove 2 %%x %%y}",
               wname, tname);
  
  this->Script("bind %s <Control-B3-Motion> {%s MouseMove 3 %%x %%y}",
               wname, tname);

  this->Script("bind %s <KeyPress> {%s AKeyPress %%A %%x %%y 0 0}",
               wname, tname);
  
  this->Script("bind %s <Shift-KeyPress> {%s AKeyPress %%A %%x %%y 0 1}",
               wname, tname);
  
  this->Script("bind %s <Control-KeyPress> {%s AKeyPress %%A %%x %%y 1 0}", 
               wname, tname);
  
  this->Script("bind %s <Configure> {%s Configure %%w %%h}",
               wname, tname);
  
  this->Script("bind %s <Enter> {%s Enter %%x %%y}",
               wname, tname);
}

void vtkKWRenderWidget::RemoveBindings()
{
  const char *wname = this->VTKWidget->GetWidgetName();
  
  this->Script("bind %s <Expose> {}", wname);
  this->Script("bind %s <Any-ButtonPress> {}", wname);
  this->Script("bind %s <Any-ButtonRelease> {}", wname);
  this->Script("bind %s <Shift-Any-ButtonPress> {}", wname);
  this->Script("bind %s <Shift-Any-ButtonRelease> {}", wname);
  this->Script("bind %s <Control-Any-ButtonPress> {}", wname);
  this->Script("bind %s <Control-Any-ButtonRelease> {}", wname);
  this->Script("bind %s <B1-Motion> {}", wname);
  this->Script("bind %s <B2-Motion> {}", wname);
  this->Script("bind %s <B3-Motion> {}", wname);
  this->Script("bind %s <Shift-B1-Motion> {}", wname);
  this->Script("bind %s <Shift-B2-Motion> {}", wname);
  this->Script("bind %s <Shift-B3-Motion> {}", wname);
  this->Script("bind %s <Control-B1-Motion> {}", wname);
  this->Script("bind %s <Control-B2-Motion> {}", wname);
  this->Script("bind %s <Control-B3-Motion> {}", wname);
  this->Script("bind %s <KeyPress> {}", wname);
  this->Script("bind %s <Shift-KeyPress> {}", wname);
  this->Script("bind %s <Control-KeyPress> {}", wname);
  this->Script("bind %s <Configure> {}", wname);
}

void vtkKWRenderWidget::MouseMove(int vtkNotUsed(num), int x, int y)
{
  this->Interactor->SetMoveEventInformationFlipY(x, y);
  this->Interactor->MouseMoveEvent();
}

void vtkKWRenderWidget::AButtonPress(int num, int x, int y,
                                     int ctrl, int shift)
{
  this->Script("focus %s", this->VTKWidget->GetWidgetName());
  
  this->Interactor->SetEventInformationFlipY(x, y, ctrl, shift);
  
  switch (num)
    {
    case 1:
      this->Interactor->LeftButtonPressEvent();
      break;
    case 2:
      this->Interactor->MiddleButtonPressEvent();
      break;
    case 3:
      this->Interactor->RightButtonPressEvent();
      break;
    }
}

void vtkKWRenderWidget::AButtonRelease(int num, int x, int y)
{
  this->Interactor->SetEventInformationFlipY(x, y, 0, 0);
  
  switch (num)
    {
    case 1:
      this->Interactor->LeftButtonReleaseEvent();
      break;
    case 2:
      this->Interactor->MiddleButtonReleaseEvent();
      break;
    case 3:
      this->Interactor->RightButtonReleaseEvent();
      break;
    }
}

void vtkKWRenderWidget::AKeyPress(char key, int x, int y, int ctrl, int shift)
{
  this->Interactor->SetEventPositionFlipY(x, y);
  this->Interactor->SetControlKey(ctrl);
  this->Interactor->SetShiftKey(shift);
  this->Interactor->SetKeyCode(key);
  this->Interactor->KeyPressEvent();
}

void vtkKWRenderWidget::Exposed()
{
  if (this->InExpose)
    {
    return;
    }
  
  this->InExpose = 1;
  this->Script("update");
  this->Render();
  this->InExpose = 0;
}

void vtkKWRenderWidget::Configure(int width, int height)
{
  this->Script("%s configure -width %d -height %d",
               this->VTKWidget->GetWidgetName(), width, height);
  
  this->Interactor->UpdateSize(width, height);
  this->Interactor->ConfigureEvent();
}

void vtkKWRenderWidget::Render()
{
  if ( this->CollapsingRenders )
    {
    this->CollapsingRendersCount ++;
    return;
    }
  this->RenderWindow->Render();
}

#ifdef _WIN32
void vtkKWRenderWidget::SetupPrint(RECT &rcDest, HDC ghdc,
                                   int printerPageSizeX, int printerPageSizeY,
                                   int printerDPIX, int printerDPIY,
                                   float scaleX, float scaleY,
                                   int screenSizeX, int screenSizeY)
{
  float scale;
  int cxDIB = screenSizeX;         // Size of DIB - x
  int cyDIB = screenSizeY;         // Size of DIB - y
  
  // target DPI specified here
  if (this->GetParentWindow())
    {
    scale = printerDPIX/this->GetParentWindow()->GetPrintTargetDPI();
    }
  else
    {
    scale = printerDPIX/100.0;
    }
  

  // Best Fit case -- create a rectangle which preserves
  // the DIB's aspect ratio, and fills the page horizontally.
  //
  // The formula in the "->bottom" field below calculates the Y
  // position of the printed bitmap, based on the size of the
  // bitmap, the width of the page, and the relative size of
  // a printed pixel (printerDPIY / printerDPIX).
  //
  rcDest.bottom = rcDest.left = 0;
  if (((float)cyDIB*(float)printerPageSizeX/(float)printerDPIX) > 
      ((float)cxDIB*(float)printerPageSizeY/(float)printerDPIY))
    {
    rcDest.top = printerPageSizeY;
    rcDest.right = (static_cast<float>(printerPageSizeY)*printerDPIX*cxDIB) /
      (static_cast<float>(printerDPIY)*cyDIB);
    }
  else
    {
    rcDest.right = printerPageSizeX;
    rcDest.top = (static_cast<float>(printerPageSizeX)*printerDPIY*cyDIB) /
      (static_cast<float>(printerDPIX)*cxDIB);
    } 
  
  this->SetupMemoryRendering(rcDest.right/scale*scaleX,
                             rcDest.top/scale*scaleY, ghdc);
}
#endif

void vtkKWRenderWidget::SetParentWindow(vtkKWWindow *window)
{
  if (this->ParentWindow == window)
    {
    return;
    }
  this->ParentWindow = window;
  this->Modified();
}

void* vtkKWRenderWidget::GetMemoryDC()
{
#ifdef _WIN32
  return (void *)vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->GetMemoryDC();
#else
  return NULL;
#endif
}

void vtkKWRenderWidget::SetupMemoryRendering(
#ifdef _WIN32
  int x, int y, void *cd
#else
  int, int, void*
#endif
  )
{
#ifdef _WIN32
  if (!cd)
    {
    cd = this->RenderWindow->GetGenericContext();
    }
  vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->SetupMemoryRendering(x, y, (HDC)cd);
#endif
}

void vtkKWRenderWidget::ResumeScreenRendering() 
{
#ifdef _WIN32
  vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->ResumeScreenRendering();
#endif
}

void vtkKWRenderWidget::AddProp(vtkProp *prop)
{
  this->Renderer->AddProp(prop);
}

int vtkKWRenderWidget::HasProp(vtkProp *prop)
{
  return this->Renderer->GetProps()->IsItemPresent(prop);
}

void vtkKWRenderWidget::RemoveProp(vtkProp *prop)
{
  this->Renderer->RemoveProp(prop);
}

void vtkKWRenderWidget::RemoveAllProps()
{
  this->Renderer->RemoveAllProps();
}

void vtkKWRenderWidget::SetBackgroundColor(float r, float g, float b)
{
  if (r < 0 || g < 0 || b < 0)
    {
    return;
    }
  
  float *ff = this->Renderer->GetBackground();
  if (ff[0] == r && ff[1] == g && ff[2] == b)
    {
    return;
    }
  
  this->Renderer->SetBackground(r, g, b);
  this->Render();
}

void vtkKWRenderWidget::GetBackgroundColor(float *r, float *g, float *b)
{
  float *ff = this->Renderer->GetBackground();
  *r = ff[0];
  *g = ff[1];
  *b = ff[2];
}

float* vtkKWRenderWidget::GetBackgroundColor()
{
  return this->Renderer->GetBackground();
}

void vtkKWRenderWidget::Close()
{
  this->RemoveBindings();
}

void vtkKWRenderWidget::CornerAnnotationOn()
{
  if (this->CornerAnnotation->GetVisibility() &&
      this->HasProp(this->CornerAnnotation))
    {
    return;
    }
  this->CornerAnnotation->VisibilityOn();
  if (!this->HasProp(this->CornerAnnotation))
    {
    this->AddProp(this->CornerAnnotation);
    }
  this->Render();
}

void vtkKWRenderWidget::CornerAnnotationOff()
{
  if (!this->CornerAnnotation->GetVisibility() ||
      !this->HasProp(this->CornerAnnotation))
    {
    return;
    }
  this->CornerAnnotation->VisibilityOff();
  if (this->HasProp(this->CornerAnnotation))
    {
    this->RemoveProp(this->CornerAnnotation);
    }
  this->Render();
}

void vtkKWRenderWidget::SetCornerTextColor(float r, float g, float b)
{
  if (this->CornerAnnotation && this->CornerAnnotation->GetTextProperty())
    {
    float *rgb = this->CornerAnnotation->GetTextProperty()->GetColor();
    if (rgb[0] != r || rgb[1] != g || rgb[2] != b)
      {
      this->CornerAnnotation->GetTextProperty()->SetColor(r, g, b);
      this->Render();
      }
    }
}

void vtkKWRenderWidget::SetCollapsingRenders(int r)
{
  if ( r )
    {
    this->CollapsingRenders = 1;
    this->CollapsingRendersCount = 0;
    }
  else
    {
    this->CollapsingRenders = 0;
    if ( this->CollapsingRendersCount )
      {
      this->Render();
      }
    }
}

void vtkKWRenderWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CornerAnnotation: " << this->CornerAnnotation << endl;
  os << indent << "Printing: " << this->Printing << endl;
  os << indent << "VTKWidget: " << this->VTKWidget << endl;
  os << indent << "RenderWindow: " << this->RenderWindow << endl;
  os << indent << "ParentWindow: ";
  if (this->ParentWindow)
    {
    os << this->ParentWindow << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "RenderMode: " << this->RenderMode << endl;
  os << indent << "RenderState: " << this->RenderState << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "EventMap: " << this->EventMap << endl;
  os << indent << "CollapsingRenders: " << this->CollapsingRenders << endl;
  os << indent << "ScalarShift: " << this->ScalarShift << endl;
  os << indent << "ScalarScale: " << this->ScalarScale << endl;
  os << indent << "CurrentCamera: " << this->CurrentCamera << endl;
  os << indent << "Units: " << (this->Units ? this->Units : "(none)") << endl;
}
