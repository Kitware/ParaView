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

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCornerAnnotation.h"
#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkKWSerializer.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

vtkCxxRevisionMacro(vtkKWRenderWidget, "1.35");

//----------------------------------------------------------------------------
class vtkKWRenderWidgetObserver : public vtkCommand
{
public:
  static vtkKWRenderWidgetObserver *New() 
    {return new vtkKWRenderWidgetObserver;};

  vtkKWRenderWidgetObserver()
    {
      this->KWRenderWidget = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
                       void* calldata)
    {
      if ( this->KWRenderWidget )
        {
        this->KWRenderWidget->ExecuteEvent(wdg, event, calldata);
        this->AbortFlagOn();
        }
    }

  vtkKWRenderWidget* KWRenderWidget;
};

//----------------------------------------------------------------------------
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
  
  this->EventIdentifier = -1;
  
  this->InExpose = 0;

  this->CornerAnnotation = vtkCornerAnnotation::New();
  this->CornerAnnotation->SetMaximumLineHeight(0.07);
  this->CornerAnnotation->VisibilityOn();

  this->HeaderProp = vtkTextActor::New();
  this->HeaderProp->GetTextProperty()->SetJustificationToCentered();
  this->HeaderProp->GetTextProperty()->SetVerticalJustificationToTop();
  this->HeaderProp->GetTextProperty()->ShadowOff();
  this->HeaderProp->ScaledTextOn();
  this->HeaderProp->GetPositionCoordinate()
    ->SetCoordinateSystemToNormalizedViewport();
  this->HeaderProp->GetPositionCoordinate()->SetValue(0.2, 0.88);
  this->HeaderProp->GetPosition2Coordinate()
    ->SetCoordinateSystemToNormalizedViewport();
  this->HeaderProp->GetPosition2Coordinate()->SetValue(0.6, 0.1);
  
  this->Units = NULL;

  vtkCamera *cam = this->GetCurrentCamera();
  if (cam)
    {
    cam->ParallelProjectionOn();
    }

  this->ScalarShift = 0;
  this->ScalarScale = 1;  

  this->CollapsingRenders = 0;
  
  this->Observer = vtkKWRenderWidgetObserver::New();
  this->Observer->KWRenderWidget = this;
  
  this->RenderWindow->AddObserver(vtkCommand::CursorChangedEvent,
                                  this->Observer);

}

//----------------------------------------------------------------------------
vtkKWRenderWidget::~vtkKWRenderWidget()
{
  this->Renderer->Delete();
  this->RenderWindow->Delete();
  this->SetParentWindow(NULL);
  this->VTKWidget->Delete();
  
  if (this->CornerAnnotation)
    {
    this->CornerAnnotation->Delete();
    this->CornerAnnotation = NULL;
    }

  this->HeaderProp->Delete();
  
  this->SetUnits(NULL);
  
  this->Observer->Delete();
  this->Observer = NULL;
}

//----------------------------------------------------------------------------
vtkCamera* vtkKWRenderWidget::GetCurrentCamera()
{
  vtkRenderer *ren = this->GetRenderer();
  if (ren)
    {
    return ren->GetActiveCamera();
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetUnits(const char* _arg)
{
  if (this->Units == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->Units && _arg && (!strcmp(this->Units, _arg))) 
    {
    return;
    }

  if (this->Units) 
    { 
    delete [] this->Units; 
    }

  if (_arg)
    {
    this->Units = new char[strlen(_arg)+1];
    strcpy(this->Units, _arg);
    }
  else
    {
    this->Units = NULL;
    }

  this->Modified();
  
  this->UpdateAccordingToUnits();
} 

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetupBindings()
{
  // First remove the old one so that bindings don't get duplicated

  this->RemoveBindings();

  const char *wname = this->VTKWidget->GetWidgetName();
  const char *tname = this->GetTclName();

  // Setup some default bindings

  this->Script("bind %s <Expose> {%s Exposed}",
               wname, tname);
  
  this->Script("bind %s <Enter> {%s Enter %%x %%y}",
               wname, tname);

  this->Script("bind %s <Configure> {%s Configure %%w %%h}",
               this->GetWidgetName(), tname);
  
  this->SetupInteractionBindings();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveBindings()
{
  const char *wname = this->VTKWidget->GetWidgetName();
  
  this->Script("bind %s <Expose> {}", wname);
  this->Script("bind %s <Enter> {}", wname);
  this->Script("bind %s <Configure> {}", this->GetWidgetName());

  this->RemoveInteractionBindings();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetupInteractionBindings()
{
  // First remove the old one so that bindings don't get duplicated

  this->RemoveInteractionBindings();

  // If we are disabled, don't do anything

  if (!this->Enabled)
    {
    return;
    }

  const char *wname = this->VTKWidget->GetWidgetName();
  const char *tname = this->GetTclName();

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
  
  this->Script("bind %s <Motion> {%s MouseMove %%b %%x %%y}", wname, tname);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveInteractionBindings()
{
  const char *wname = this->VTKWidget->GetWidgetName();
  
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

  this->Script("bind %s <Motion> {}", wname);
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkKWRenderWidget::Render()
{
  if (this->CollapsingRenders)
    {
    this->CollapsingRendersCount++;
    return;
    }

  if (!this->RenderState)
    {
    return;
    }

  static int static_in_render = 0;
  if (static_in_render)
    {
    return;
    }
  static_in_render = 1;

  if (this->RenderMode != VTK_KW_DISABLED_RENDER)
    {
    this->Renderer->ResetCameraClippingRange();
    this->RenderWindow->Render();
    }
  
  static_in_render = 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetParentWindow(vtkKWWindow *window)
{
  if (this->ParentWindow == window)
    {
    return;
    }
  this->ParentWindow = window;
  
  this->Modified();
}

//----------------------------------------------------------------------------
void* vtkKWRenderWidget::GetMemoryDC()
{
#ifdef _WIN32
  return (void *)vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->GetMemoryDC();
#else
  return NULL;
#endif
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ResumeScreenRendering() 
{
#ifdef _WIN32
  vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->ResumeScreenRendering();
#endif
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddProp(vtkProp *prop)
{
  this->Renderer->AddProp(prop);
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::HasProp(vtkProp *prop)
{
  return this->Renderer->GetProps()->IsItemPresent(prop);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveProp(vtkProp *prop)
{
  this->Renderer->RemoveProp(prop);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveAllProps()
{
  this->Renderer->RemoveAllProps();
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
float* vtkKWRenderWidget::GetBackgroundColor()
{
  return this->Renderer->GetBackground();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::Close()
{
  this->RemoveBindings();

  if (this->GetCornerAnnotation())
    {
    this->GetCornerAnnotation()->ClearAllTexts();
    }

  this->SetCornerAnnotationVisibility(0);
  this->SetHeaderAnnotationVisibility(0);
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::GetCornerAnnotationVisibility()
{
  return (this->CornerAnnotation &&
          this->HasProp(this->CornerAnnotation) && 
          this->CornerAnnotation->GetVisibility());
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetCornerAnnotationVisibility(int v)
{
  if (!this->CornerAnnotation ||
      this->GetCornerAnnotationVisibility() == v)
    {
    return;
    }

  if (v)
    {
    this->CornerAnnotation->VisibilityOn();
    if (!this->HasProp(this->CornerAnnotation))
      {
      this->AddProp(this->CornerAnnotation);
      }
    }
  else
    {
    this->CornerAnnotation->VisibilityOff();
    if (this->HasProp(this->CornerAnnotation))
      {
      this->RemoveProp(this->CornerAnnotation);
      }
    }

  this->Render();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetCornerAnnotationColor(float r, float g, float b)
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

//----------------------------------------------------------------------------
float* vtkKWRenderWidget::GetCornerAnnotationColor()
{
  if (!this->CornerAnnotation ||
      !this->CornerAnnotation->GetTextProperty())
    {
    return 0;
    }
  float *color = this->CornerAnnotation->GetTextProperty()->GetColor();
  if (color[0] < 0 || color[1] < 0 || color[2] < 0)
    {
    color = this->CornerAnnotation->GetProperty()->GetColor();
    }
  return color;
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::GetHeaderAnnotationVisibility()
{
  return (this->HeaderProp && 
          this->HasProp(this->HeaderProp) && 
          this->HeaderProp->GetVisibility());
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetHeaderAnnotationVisibility(int v)
{
  if (!this->HeaderProp || 
      this->GetHeaderAnnotationVisibility() == v)
    {
    return;
    }

  if (v)
    {
    this->HeaderProp->VisibilityOn();
    if (!this->HasProp(this->HeaderProp))
      {
      this->AddProp(this->HeaderProp);
      }
    }
  else
    {
    this->HeaderProp->VisibilityOff();
    if (this->HasProp(this->HeaderProp))
      {
      this->RemoveProp(this->HeaderProp);
      }
    }

  this->Render();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetHeaderAnnotationColor(float r, float g, float b)
{
  if (this->HeaderProp && this->HeaderProp->GetTextProperty())
    {
    float *rgb = this->HeaderProp->GetTextProperty()->GetColor();
    if (rgb[0] != r || rgb[1] != g || rgb[2] != b)
      {
      this->HeaderProp->GetTextProperty()->SetColor(r, g, b);
      this->Render();
      }
    }
}

//----------------------------------------------------------------------------
float* vtkKWRenderWidget::GetHeaderAnnotationColor()
{
  if (!this->HeaderProp ||
      !this->HeaderProp->GetTextProperty())
    {
    return 0;
    }
  float *color = this->HeaderProp->GetTextProperty()->GetColor();
  if (color[0] < 0 || color[1] < 0 || color[2] < 0)
    {
    color = this->HeaderProp->GetProperty()->GetColor();
    }
  return color;
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetHeaderAnnotationText(const char *text)
{
  if (this->HeaderProp)
    {
    this->HeaderProp->SetInput(text);
    if (this->GetHeaderAnnotationVisibility())
      {
      this->Render();
      }
    }
}

//----------------------------------------------------------------------------
char* vtkKWRenderWidget::GetHeaderAnnotationText()
{
  if (this->HeaderProp)
    {
    return this->HeaderProp->GetInput();
    }
  return 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ExecuteEvent(vtkObject*, unsigned long event,
                                       void *par)
{
  if ( event == vtkCommand::CursorChangedEvent )
    {
    int val = *(static_cast<int*>(par));
    const char* image = "left_ptr";
    switch ( val ) 
      {
      case VTK_CURSOR_ARROW:
        image = "arrow";
        break;
      case VTK_CURSOR_SIZENE:
#ifdef _WIN32
        image = "size_ne_sw";
#else
        image = "top_right_corner";
#endif
        break;
      case VTK_CURSOR_SIZENW:
#ifdef _WIN32
        image = "size_nw_se";
#else
        image = "top_left_corner";
#endif
        break;
      case VTK_CURSOR_SIZESW:
#ifdef _WIN32
        image = "size_ne_sw";
#else
        image = "bottom_left_corner";
#endif
        break;
      case VTK_CURSOR_SIZESE:
#ifdef _WIN32
        image = "size_nw_se";
#else
        image = "bottom_right_corner";
#endif
        break;
      case VTK_CURSOR_SIZENS:
        image = "sb_v_double_arrow";
        break;
      case VTK_CURSOR_SIZEWE:
        image = "sb_h_double_arrow";
        break;
      case VTK_CURSOR_SIZEALL:
        image = "fleur";
        break;
      case VTK_CURSOR_HAND:
        image = "hand2";
        break;
      }
    this->Script("%s config -cursor %s", 
                 this->GetParentWindow()->GetWidgetName(), image);
    }
}

//------------------------------------------------------------------------------
void vtkKWRenderWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // If enabled back, set up the bindings, otherwise remove

  if (this->Enabled)
    {
    this->SetupInteractionBindings();
    }
  else
    {
    this->RemoveInteractionBindings();
    }
}

//------------------------------------------------------------------------------
void vtkKWRenderWidget::SerializeSelf(ostream& os, vtkIndent indent)
{
  float *fptr;
  double *dptr;

  // Invoke superclass

  this->Superclass::SerializeSelf(os, indent);

  // Background color

  fptr = this->GetBackgroundColor();
  if (fptr)
    {
    os << indent << "BackgroundColor " 
       << fptr[0] << " " << fptr[1] << " " << fptr[2] << endl;
    }

  // Header Annotation

  os << indent << "HeaderAnnotationVisibility " 
     << this->GetHeaderAnnotationVisibility() << endl;

  fptr = this->GetHeaderAnnotationColor();
  if (fptr)
    {
    os << indent << "HeaderAnnotationColor " 
       << fptr[0] << " " << fptr[1] << " " << fptr[2] << endl;
    }

  if (this->GetHeaderAnnotationText())
    {
    os << indent << "HeaderAnnotationText " 
       << this->GetHeaderAnnotationText() << endl;
    }

  // Units

  if (this->GetUnits())
    {
    os << indent << "Units " << this->GetUnits() << endl;
    }

  // Camera

  vtkCamera *cam = this->GetCurrentCamera();
  if (cam)
    {
    dptr = cam->GetPosition();
    os << indent << "CameraPosition " 
       << dptr[0] << " " << dptr[1] << " " << dptr[2] << endl;

    dptr = cam->GetFocalPoint();
    os << indent << "CameraFocalPoint " 
       << dptr[0] << " " << dptr[1] << " " << dptr[2] << endl;

    dptr = cam->GetViewUp();
    os << indent << "CameraViewUp " 
       << dptr[0] << " " << dptr[1] << " " << dptr[2] << endl;

    dptr = cam->GetClippingRange();
    os << indent << "CameraClippingRange " 
       << dptr[0] << " " << dptr[1] << endl;

    os << indent << "CameraViewAngle " << cam->GetViewAngle() << endl;

    os << indent << "CameraParallelScale " << cam->GetParallelScale() << endl;

    os << indent << "CameraParallelProjection " 
       << cam->GetParallelProjection() << endl;
    }

  // ScalarShift and ScalarScale

  os << indent << "ScalarShift " << this->GetScalarShift() << endl;

  os << indent << "ScalarScale " << this->GetScalarScale() << endl;
}

//------------------------------------------------------------------------------
void vtkKWRenderWidget::SerializeToken(istream& is, const char token[1024])
{
  float fval, fbuffer3[3];
  double dval, dbuffer3[3];
  int i;
  char buffer[1024];

  // Background color

  if (!strcmp(token, "BackgroundColor"))
    {
    fbuffer3[0] = fbuffer3[1] = fbuffer3[2] = 0.0;
    is >> fbuffer3[0] >> fbuffer3[1] >> fbuffer3[2];
    this->SetBackgroundColor(fbuffer3);
    return;
    }

  // Header Annotation

  if (!strcmp(token, "HeaderAnnotationVisibility"))
    {
    is >> i;
    this->SetHeaderAnnotationVisibility(i);
    return;
    }

  if (!strcmp(token, "HeaderAnnotationColor"))
    {
    fbuffer3[0] = fbuffer3[1] = fbuffer3[2] = 1.0;
    is >> fbuffer3[0] >> fbuffer3[1] >> fbuffer3[2];
    this->SetHeaderAnnotationColor(fbuffer3);
    return;
    }

  if (!strcmp(token, "HeaderAnnotationText"))
    {
    buffer[0] = '\0';
    vtkKWSerializer::EatWhiteSpace(&is);
    is.getline(buffer, 1024);
    this->SetHeaderAnnotationText(buffer);
    return;
    }

  // Units

  if (!strcmp(token, "Units"))
    {
    buffer[0] = '\0';
    vtkKWSerializer::EatWhiteSpace(&is);
    is.getline(buffer, 1024);
    this->SetUnits(buffer);
    return;
    }

  // Camera

  vtkCamera *cam = this->GetCurrentCamera();
  if (cam)
    {
    if (!strcmp(token, "CameraPosition"))
      {
      is >> dbuffer3[0] >> dbuffer3[1] >> dbuffer3[2];
      cam->SetPosition(dbuffer3);
      cam->ComputeViewPlaneNormal();
      return;
      }

    if (!strcmp(token, "CameraFocalPoint"))
      {
      is >> dbuffer3[0] >> dbuffer3[1] >> dbuffer3[2];
      cam->SetFocalPoint(dbuffer3);
      cam->ComputeViewPlaneNormal();
      return;
      }

    if (!strcmp(token, "CameraViewUp"))
      {
      is >> dbuffer3[0] >> dbuffer3[1] >> dbuffer3[2];
      cam->SetViewUp(dbuffer3);
      return;
      }

    if (!strcmp(token, "CameraClippingRange"))
      {
      is >> dbuffer3[0] >> dbuffer3[1];
      cam->SetClippingRange(dbuffer3[0], dbuffer3[1]);
      return;
      }

    if (!strcmp(token, "CameraViewAngle"))
      {
      is >> dval;
      cam->SetViewAngle(dval);
      return;
      }

    if (!strcmp(token, "CameraParallelScale"))
      {
      is >> dval;
      cam->SetParallelScale(dval);
      return;
      }

    if (!strcmp(token, "CameraParallelProjection"))
      {
      is >> i;
      cam->SetParallelProjection(i);
      return;
      }
    }

  // ScalarShift and ScalarScale

  if (!strcmp(token, "ScalarShift"))
    {
    is >> fval;
    this->SetScalarShift(fval);
    return;
    }

  if (!strcmp(token, "ScalarScale"))
    {
    is >> fval;
    this->SetScalarScale(fval);
    return;
    }

  // Invoke superclass

  this->Superclass::SerializeToken(is, token);
}

//------------------------------------------------------------------------------
void vtkKWRenderWidget::SerializeRevision(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeRevision(os, indent);
  os << indent << "vtkKWRenderWidget ";
  this->ExtractRevision(os, "$Revision: 1.35 $");
}

//----------------------------------------------------------------------------
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
  os << indent << "CollapsingRenders: " << this->CollapsingRenders << endl;
  os << indent << "ScalarShift: " << this->ScalarShift << endl;
  os << indent << "ScalarScale: " << this->ScalarScale << endl;
  os << indent << "Units: " << (this->Units ? this->Units : "(none)") << endl;
  os << indent << "EventIdentifier: " << this->EventIdentifier << endl;
}
