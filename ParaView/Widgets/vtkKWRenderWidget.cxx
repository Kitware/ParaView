/*=========================================================================

  Module:    vtkKWRenderWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWRenderWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCornerAnnotation.h"
#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkKWRenderWidgetCallbackCommand.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

vtkCxxRevisionMacro(vtkKWRenderWidget, "1.69");

//----------------------------------------------------------------------------
vtkKWRenderWidget::vtkKWRenderWidget()
{
  this->VTKWidget = vtkKWWidget::New();
  this->VTKWidget->SetParent(this);
  
  this->Renderer = vtkRenderer::New();
  this->Renderer->SetLayer(1);
  this->OverlayRenderer = vtkRenderer::New();
  this->RenderWindow = vtkRenderWindow::New();
  this->RenderWindow->SetNumberOfLayers(2);
  this->RenderWindow->AddRenderer(this->OverlayRenderer);
  this->RenderWindow->AddRenderer(this->Renderer);
  
  this->Printing = 0;
  this->RenderMode = vtkKWRenderWidget::STILL_RENDER;
  this->RenderState = 1;
  
  this->ParentWindow = NULL;
  
  this->EventIdentifier = -1;
  
  this->InExpose = 0;

  this->CornerAnnotation = vtkCornerAnnotation::New();
  this->CornerAnnotation->SetMaximumLineHeight(0.07);
  this->CornerAnnotation->VisibilityOff();

  this->HeaderAnnotation = vtkTextActor::New();
  this->HeaderAnnotation->SetNonLinearFontScale(0.7,10);
  this->HeaderAnnotation->GetTextProperty()->SetJustificationToCentered();
  this->HeaderAnnotation->GetTextProperty()->SetVerticalJustificationToTop();
  this->HeaderAnnotation->GetTextProperty()->ShadowOff();
  this->HeaderAnnotation->ScaledTextOn();
  this->HeaderAnnotation->GetPositionCoordinate()
    ->SetCoordinateSystemToNormalizedViewport();
  this->HeaderAnnotation->GetPositionCoordinate()->SetValue(0.2, 0.84);
  this->HeaderAnnotation->GetPosition2Coordinate()
    ->SetCoordinateSystemToNormalizedViewport();
  this->HeaderAnnotation->GetPosition2Coordinate()->SetValue(0.6, 0.1);
  this->HeaderAnnotation->VisibilityOff();
  
  this->DistanceUnits = NULL;

  vtkCamera *cam = this->GetCurrentCamera();
  if (cam)
    {
    cam->ParallelProjectionOn();
    }
  this->OverlayRenderer->SetActiveCamera(cam);
  this->CollapsingRenders = 0;
  
  this->Observer = vtkKWRenderWidgetCallbackCommand::New();
  
  this->PreviousRenderMode = vtkKWRenderWidget::STILL_RENDER;
}

//----------------------------------------------------------------------------
vtkKWRenderWidget::~vtkKWRenderWidget()
{
  if (this->Observer)
    {
    this->Observer->Delete();
    this->Observer = NULL;
    }

  if (this->Renderer)
    {
    this->Renderer->Delete();
    this->Renderer = NULL;
    }

  if (this->OverlayRenderer)
    {
    this->OverlayRenderer->Delete();
    this->OverlayRenderer = NULL;
    }

  if (this->RenderWindow)
    {
    this->RenderWindow->Delete();
    this->RenderWindow = NULL;
    }

  this->SetParentWindow(NULL);

  if (this->VTKWidget)
    {
    this->VTKWidget->Delete();
    this->VTKWidget = NULL;
    }
  
  if (this->CornerAnnotation)
    {
    this->CornerAnnotation->Delete();
    this->CornerAnnotation = NULL;
    }

  if (this->HeaderAnnotation)
    {
    this->HeaderAnnotation->Delete();
    this->HeaderAnnotation = NULL;
    }
  
  this->SetDistanceUnits(NULL);
}

//----------------------------------------------------------------------------
vtkRenderer* vtkKWRenderWidget::GetNthRenderer(int id)
{
  if (id != 0)
    {
    return NULL;
    }

  return this->Renderer;
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::GetRendererId(vtkRenderer *ren)
{
  if (!ren || ren != this->Renderer)
    {
    return -1;
    }

  return 0;
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
void vtkKWRenderWidget::SetDistanceUnits(const char* _arg)
{
  if (this->DistanceUnits == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->DistanceUnits && _arg && (!strcmp(this->DistanceUnits, _arg))) 
    {
    return;
    }

  if (this->DistanceUnits) 
    { 
    delete [] this->DistanceUnits; 
    }

  if (_arg)
    {
    this->DistanceUnits = new char[strlen(_arg)+1];
    strcpy(this->DistanceUnits, _arg);
    }
  else
    {
    this->DistanceUnits = NULL;
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

  // Create the VTK Tk render widget in VTKWidget

  this->Script("vtkTkRenderWidget %s %s",
               this->VTKWidget->GetWidgetName(), local);
  this->VTKWidget->SetApplication(app);

  this->Script("grid rowconfigure %s 0 -weight 1",
               this->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 1",
               this->GetWidgetName());

  this->Script("grid %s -sticky nsew",
               this->VTKWidget->GetWidgetName());
  
  this->RenderWindow->Render();
  delete [] local;

  // Make the corner annotation visibile

  this->SetCornerAnnotationVisibility(1);

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddBindings()
{
  // First remove the old one so that bindings don't get duplicated

  this->RemoveBindings();

  const char *tname = this->GetTclName();

  if (this->VTKWidget->IsCreated())
    {
    const char *wname = this->VTKWidget->GetWidgetName();

    // Setup some default bindings
    
    this->Script("bind %s <Expose> {%s Exposed}",
                 wname, tname);
  
    this->Script("bind %s <Enter> {%s Enter %%x %%y}",
                 wname, tname);

    this->Script("bind %s <FocusIn> {%s FocusInCallback}", 
                 wname, tname);

    this->Script("bind %s <FocusOut> {%s FocusOutCallback}", 
                 wname, tname);
    }

  if (this->IsCreated())
    {
    this->Script("bind %s <Configure> {%s Configure %%w %%h}",
                 this->GetWidgetName(), tname);
    }
  
  this->AddInteractionBindings();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveBindings()
{
  if (this->VTKWidget->IsCreated())
    {
    const char *wname = this->VTKWidget->GetWidgetName();
  
    this->Script("bind %s <Expose> {}", wname);
    this->Script("bind %s <Enter> {}", wname);

    this->Script("bind %s <FocusIn> {}", wname);
    this->Script("bind %s <FocusOut> {}", wname);
    }

  if (this->IsCreated())
    {
    this->Script("bind %s <Configure> {}", this->GetWidgetName());
    }

  this->RemoveInteractionBindings();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddInteractionBindings()
{
  // First remove the old one so that bindings don't get duplicated

  this->RemoveInteractionBindings();

  // If we are disabled, don't do anything

  if (!this->Enabled)
    {
    return;
    }

  if (this->VTKWidget->IsCreated())
    {
    const char *wname = this->VTKWidget->GetWidgetName();
    const char *tname = this->GetTclName();

    this->Script(
      "bind %s <Any-ButtonPress> {%s AButtonPress %%b %%x %%y 0 0}",
      wname, tname);

    this->Script(
      "bind %s <Any-ButtonRelease> {%s AButtonRelease %%b %%x %%y}",
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

    this->Script(
      "bind %s <B1-Motion> {%s MouseMove 1 %%x %%y}",
      wname, tname);

    this->Script(
      "bind %s <B2-Motion> {%s MouseMove 2 %%x %%y}", 
      wname, tname);
  
    this->Script(
      "bind %s <B3-Motion> {%s MouseMove 3 %%x %%y}", 
      wname, tname);

    this->Script(
      "bind %s <Shift-B1-Motion> {%s MouseMove 1 %%x %%y}", 
      wname, tname);

    this->Script(
      "bind %s <Shift-B2-Motion> {%s MouseMove 2 %%x %%y}", 
      wname, tname);
  
    this->Script(
      "bind %s <Shift-B3-Motion> {%s MouseMove 3 %%x %%y}", 
      wname, tname);

    this->Script(
      "bind %s <Control-B1-Motion> {%s MouseMove 1 %%x %%y}",
      wname, tname);

    this->Script(
      "bind %s <Control-B2-Motion> {%s MouseMove 2 %%x %%y}",
      wname, tname);
  
    this->Script(
      "bind %s <Control-B3-Motion> {%s MouseMove 3 %%x %%y}",
      wname, tname);

    this->Script(
      "bind %s <KeyPress> {%s AKeyPress %%A %%x %%y 0 0 %%K}",
      wname, tname);
  
    this->Script(
      "bind %s <Shift-KeyPress> {%s AKeyPress %%A %%x %%y 0 1 %%K}",
      wname, tname);
  
    this->Script(
      "bind %s <Control-KeyPress> {%s AKeyPress %%A %%x %%y 1 0 %%K}",
      wname, tname);
  
    this->Script(
      "bind %s <Motion> {%s MouseMove 0 %%x %%y}", 
      wname, tname);
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveInteractionBindings()
{
  if (this->VTKWidget->IsCreated())
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
void vtkKWRenderWidget::FocusInCallback()
{
  this->InvokeEvent(vtkKWEvent::FocusInEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::FocusOutCallback()
{
  this->InvokeEvent(vtkKWEvent::FocusOutEvent, NULL);
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

  if (this->RenderMode != vtkKWRenderWidget::DISABLED_RENDER)
    {
    this->Renderer->ResetCameraClippingRange();
    this->RenderWindow->Render();
    }
  
  static_in_render = 0;
}

//----------------------------------------------------------------------------
const char* vtkKWRenderWidget::GetRenderModeAsString()
{
  switch (this->RenderMode)
    {
    case vtkKWRenderWidget::INTERACTIVE_RENDER:
      return "Interactive";
    case vtkKWRenderWidget::STILL_RENDER:
      return "Still";
    case vtkKWRenderWidget::SINGLE_RENDER:
      return "Single";
    case vtkKWRenderWidget::DISABLED_RENDER:
      return "Disabled";
    default:
      return "Unknown (error)";
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetParentWindow(vtkKWWindow *window)
{
  if (this->ParentWindow == window)
    {
    return;
    }

  // Remove old observers 
  // (some of them may use the parent window, to display progress for example)

  if (this->ParentWindow)
    {
    this->RemoveObservers();
    }

  this->ParentWindow = window;

  // Reinstall observers

  if (this->ParentWindow)
    {
    this->AddObservers();
    }
  
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetOffScreenRendering(int val)
{
  if (this->GetRenderWindow())
    {
    this->GetRenderWindow()->SetOffScreenRendering(val);
    }
  this->SetPrinting(val);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetPrinting(int arg)
{
  if (arg == this->Printing)
    {
    return;
    }

  this->Printing = arg;
  this->Modified();

  if (this->Printing)
    {
    this->PreviousRenderMode = this->GetRenderMode();
    this->SetRenderModeToSingle();
    }
  else
    {
    this->SetRenderMode(this->PreviousRenderMode);

    // SetupPrint will call SetupMemoryRendering().
    // As convenience, let's call ResumeScreenRendering()
    this->ResumeScreenRendering();
    }
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
void vtkKWRenderWidget::AddOverlayProp(vtkProp *prop)
{
  this->OverlayRenderer->AddProp(prop);
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::HasProp(vtkProp *prop)
{
  if (this->Renderer->GetProps()->IsItemPresent(prop) ||
      this->OverlayRenderer->GetProps()->IsItemPresent(prop))
    {
    return 1;
    }
  
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveProp(vtkProp *prop)
{
  // safe to call both, vtkViewport does a check first
  this->Renderer->RemoveProp(prop);
  this->OverlayRenderer->RemoveProp(prop);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveAllProps()
{
  this->Renderer->RemoveAllProps();
  this->OverlayRenderer->RemoveAllProps();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetBackgroundColor(float r, float g, float b)
{
  float *color = this->GetBackgroundColor();
  if (!color || (color[0] == r && color[1] == g && color[2] == b))
    {
    return;
    }

  if (r < 0 || g < 0 || b < 0)
    {
    return;
    }
  
  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  if (renderers)
    {
    renderers->InitTraversal();
    vtkRenderer *renderer = renderers->GetNextItem();
    while (renderer)
      {
      renderer->SetBackground(r, g, b);
      renderer = renderers->GetNextItem();
      }
    }

  this->Render();
}

//----------------------------------------------------------------------------
float* vtkKWRenderWidget::GetBackgroundColor()
{
  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  if (renderers)
    {
    renderers->InitTraversal();
    vtkRenderer *renderer = renderers->GetNextItem();
    if (renderer)
      {
      return renderer->GetBackground();
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::Close()
{
  this->RemoveBindings();

  // Clear all corner annotation texts

  if (this->GetCornerAnnotation())
    {
    this->GetCornerAnnotation()->ClearAllTexts();
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetAnnotationsVisibility(int v)
{
  this->SetCornerAnnotationVisibility(v);
  this->SetHeaderAnnotationVisibility(v);
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
  if (this->GetCornerAnnotationVisibility() == v)
    {
    return;
    }

  if (v)
    {
    this->CornerAnnotation->VisibilityOn();
    if (!this->HasProp(this->CornerAnnotation))
      {
      this->AddOverlayProp(this->CornerAnnotation);
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
  float *color = this->GetCornerAnnotationColor();
  if (!color || (color[0] == r && color[1] == g && color[2] == b))
    {
    return;
    }

  if (this->CornerAnnotation && this->CornerAnnotation->GetTextProperty())
    {
    this->CornerAnnotation->GetTextProperty()->SetColor(r, g, b);
    if (this->GetCornerAnnotationVisibility())
      {
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
  return (this->HeaderAnnotation && 
          this->HasProp(this->HeaderAnnotation) && 
          this->HeaderAnnotation->GetVisibility());
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetHeaderAnnotationVisibility(int v)
{
  if (this->GetHeaderAnnotationVisibility() == v)
    {
    return;
    }

  if (v)
    {
    this->HeaderAnnotation->VisibilityOn();
    if (!this->HasProp(this->HeaderAnnotation))
      {
      this->AddOverlayProp(this->HeaderAnnotation);
      }
    }
  else
    {
    this->HeaderAnnotation->VisibilityOff();
    if (this->HasProp(this->HeaderAnnotation))
      {
      this->RemoveProp(this->HeaderAnnotation);
      }
    }

  this->Render();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetHeaderAnnotationColor(float r, float g, float b)
{
  float *color = this->GetHeaderAnnotationColor();
  if (!color || (color[0] == r && color[1] == g && color[2] == b))
    {
    return;
    }

  if (this->HeaderAnnotation && this->HeaderAnnotation->GetTextProperty())
    {
    this->HeaderAnnotation->GetTextProperty()->SetColor(r, g, b);
    if (this->GetHeaderAnnotationVisibility())
      {
      this->Render();
      }
    }
}

//----------------------------------------------------------------------------
float* vtkKWRenderWidget::GetHeaderAnnotationColor()
{
  if (!this->HeaderAnnotation ||
      !this->HeaderAnnotation->GetTextProperty())
    {
    return 0;
    }
  float *color = this->HeaderAnnotation->GetTextProperty()->GetColor();
  if (color[0] < 0 || color[1] < 0 || color[2] < 0)
    {
    color = this->HeaderAnnotation->GetProperty()->GetColor();
    }
  return color;
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetHeaderAnnotationText(const char *text)
{
  if (this->HeaderAnnotation)
    {
    this->HeaderAnnotation->SetInput(text);
    if (this->GetHeaderAnnotationVisibility())
      {
      this->Render();
      }
    }
}

//----------------------------------------------------------------------------
char* vtkKWRenderWidget::GetHeaderAnnotationText()
{
  if (this->HeaderAnnotation)
    {
    return this->HeaderAnnotation->GetInput();
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
void vtkKWRenderWidget::AddObservers()
{
  this->Observer->SetRenderWidget(this);

  if (this->RenderWindow)
    {
    this->RenderWindow->AddObserver(
      vtkCommand::CursorChangedEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveObservers()
{
  this->Observer->SetRenderWidget(NULL);

  if (this->RenderWindow)
    {
    this->RenderWindow->RemoveObservers(
      vtkCommand::CursorChangedEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ProcessEvent(vtkObject *vtkNotUsed(caller),
                                     unsigned long event,
                                     void *calldata)
{
  const char *cptr = 0;
  
  switch (event)
    {
    case vtkCommand::CursorChangedEvent:
      cptr = "left_ptr";
      switch (*(static_cast<int*>(calldata))) 
        {
        case VTK_CURSOR_ARROW:
          cptr = "arrow";
          break;
        case VTK_CURSOR_SIZENE:
#ifdef _WIN32
          cptr = "size_ne_sw";
#else
          cptr = "top_right_corner";
#endif
          break;
        case VTK_CURSOR_SIZENW:
#ifdef _WIN32
          cptr = "size_nw_se";
#else
          cptr = "top_left_corner";
#endif
          break;
        case VTK_CURSOR_SIZESW:
#ifdef _WIN32
          cptr = "size_ne_sw";
#else
          cptr = "bottom_left_corner";
#endif
          break;
        case VTK_CURSOR_SIZESE:
#ifdef _WIN32
          cptr = "size_nw_se";
#else
          cptr = "bottom_right_corner";
#endif
          break;
        case VTK_CURSOR_SIZENS:
          cptr = "sb_v_double_arrow";
          break;
        case VTK_CURSOR_SIZEWE:
          cptr = "sb_h_double_arrow";
          break;
        case VTK_CURSOR_SIZEALL:
          cptr = "fleur";
          break;
        case VTK_CURSOR_HAND:
          cptr = "hand2";
          break;
        }
      if (this->GetParentWindow())
        {
        this->Script("%s config -cursor %s", 
                     this->GetParentWindow()->GetWidgetName(), cptr);
        }
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // If enabled back, set up the bindings, otherwise remove

  if (this->Enabled)
    {
    this->AddInteractionBindings();
    }
  else
    {
    this->RemoveInteractionBindings();
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CornerAnnotation: " << this->CornerAnnotation << endl;
  os << indent << "HeaderAnnotation: " << this->HeaderAnnotation << endl;
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
  os << indent << "RenderMode: " << this->GetRenderModeAsString() << endl;
  os << indent << "RenderState: " << this->RenderState << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "CollapsingRenders: " << this->CollapsingRenders << endl;
  os << indent << "DistanceUnits: " 
     << (this->DistanceUnits ? this->DistanceUnits : "(none)") << endl;
  os << indent << "EventIdentifier: " << this->EventIdentifier << endl;
}
