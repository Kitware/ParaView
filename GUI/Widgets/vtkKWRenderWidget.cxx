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
#include "vtkKWGenericRenderWindowInteractor.h"
#include "vtkKWRenderWidgetCallbackCommand.h"
#include "vtkKWWindow.h"
#include "vtkMath.h"
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

vtkStandardNewMacro(vtkKWRenderWidget);
vtkCxxRevisionMacro(vtkKWRenderWidget, "1.78");

//----------------------------------------------------------------------------
vtkKWRenderWidget::vtkKWRenderWidget()
{
  // The main callback

  this->Observer = vtkKWRenderWidgetCallbackCommand::New();
  this->EventIdentifier = -1;

  // The vtkTkRenderWidget

  this->ParentWindow = NULL;
  
  this->VTKWidget = vtkKWWidget::New();
  this->VTKWidget->SetParent(this);

  // Create two renderers by default (main one and overlay)

  this->Renderer = vtkRenderer::New();
  this->Renderer->SetLayer(0);

  this->OverlayRenderer = vtkRenderer::New();
  this->OverlayRenderer->SetLayer(1);

  // Create a render window and add two renderers

  this->RenderWindow = vtkRenderWindow::New();
  this->RenderWindow->SetNumberOfLayers(2);
  this->RenderWindow->AddRenderer(this->OverlayRenderer);
  this->RenderWindow->AddRenderer(this->Renderer);

  // Create a default (generic) interactor, which is pretty much
  // the only way to interact with a VTK Tk render widget

  this->Interactor = vtkKWGenericRenderWindowInteractor::New();
  this->Interactor->SetRenderWidget(this);  
  this->Interactor->SetRenderWindow(this->RenderWindow);
  this->Interactor->AddObserver(vtkCommand::CreateTimerEvent, this->Observer);
  this->Interactor->AddObserver(vtkCommand::DestroyTimerEvent, this->Observer);
  this->InteractorTimerToken = NULL;

  // Corner annotation

  this->CornerAnnotation = vtkCornerAnnotation::New();
  this->CornerAnnotation->SetMaximumLineHeight(0.07);
  this->CornerAnnotation->VisibilityOff();

  // Header annotation

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
  
  // Distance units

  this->DistanceUnits = NULL;

  // Get the camera, use it in overlay renderer too

  vtkCamera *cam = this->GetCurrentCamera();
  if (cam)
    {
    cam->ParallelProjectionOn();
    }
  this->GetOverlayRenderer()->SetActiveCamera(cam);

  // Current state (render mode, in expose, printing, etc)

  this->RenderMode         = vtkKWRenderWidget::STILL_RENDER;
  this->PreviousRenderMode = this->RenderMode;
  this->RenderState        = 1;
  this->CollapsingRenders  = 0;
  this->InExpose           = 0;
  this->Printing           = 0;
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

  if (this->Interactor)
    {
    this->Interactor->SetRenderWidget(NULL);
    this->Interactor->SetInteractorStyle(NULL);
    this->Interactor->Delete();
    this->Interactor = NULL;
    }

  if (this->InteractorTimerToken)
    {
    Tcl_DeleteTimerHandler(this->InteractorTimerToken);
    this->InteractorTimerToken = NULL;
    }

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
vtkRenderer* vtkKWRenderWidget::GetOverlayRenderer()
{
  return this->OverlayRenderer;
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
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  
  // Create the VTK Tk render widget in VTKWidget

  char *local = new char[(args ? strlen(args) : 0) + 100];
  sprintf(local, "%s -rw Addr=%p", (args ? args : ""), this->RenderWindow);
  this->VTKWidget->Create(app, "vtkTkRenderWidget", local);
  delete [] local;

  this->Script("grid rowconfigure %s 0 -weight 1", this->GetWidgetName());
  this->Script("grid columnconfigure %s 0 -weight 1", this->GetWidgetName());
  this->Script("grid %s -sticky nsew", this->VTKWidget->GetWidgetName());
  
  this->RenderWindow->Render();

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
void vtkKWRenderWidget::MouseMove(int vtkNotUsed(num), int x, int y)
{
  this->Interactor->SetEventPositionFlipY(x, y);
  this->Interactor->MouseMoveEvent();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AButtonPress(int num, int x, int y,
                                     int ctrl, int shift)
{
  this->VTKWidget->Focus();
  
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AKeyPress(char key, 
                                  int x, int y,
                                  int ctrl, int shift,
                                  char *keysym)
{
  this->Interactor->SetEventPositionFlipY(x, y);
  this->Interactor->SetControlKey(ctrl);
  this->Interactor->SetShiftKey(shift);
  this->Interactor->SetKeyCode(key);
  this->Interactor->SetKeySym(keysym);
  this->Interactor->CharEvent();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::Configure(int width, int height)
{
  this->Interactor->UpdateSize(width, height);
  this->Interactor->ConfigureEvent();
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
void vtkKWRenderWidget::Reset()
{
  this->ResetCamera();
  this->Render();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ResetCamera()
{
  double bounds[6];
  double center[3];
  double distance, width, viewAngle, vn[3], *vup;
  
  this->GetRenderer()->ComputeVisiblePropBounds(bounds);
  if (bounds[0] == VTK_LARGE_FLOAT)
    {
    vtkDebugMacro(<< "Cannot reset camera!");
    return;
    }

  vtkCamera *cam = this->GetCurrentCamera();
  if (cam != NULL)
    {
    cam->GetViewPlaneNormal(vn);
    }
  else
    {
    vtkErrorMacro(<< "Trying to reset non-existant camera");
    return;
    }

  center[0] = (double)(((double)bounds[0] + (double)bounds[1]) / 2.0);
  center[1] = (double)(((double)bounds[2] + (double)bounds[3]) / 2.0);
  center[2] = (double)(((double)bounds[4] + (double)bounds[5]) / 2.0);

  width = (double)bounds[3] - (double)bounds[2];
  if (width < ((double)bounds[1] - (double)bounds[0]))
    {
    width = (double)bounds[1] - (double)bounds[0];
    }
  if (width < ((double)bounds[5] - (double)bounds[4]))
    {
    width = (double)bounds[5] - (double)bounds[4];
    }
  
  if (cam->GetParallelProjection())
    {
    viewAngle = 30;  // the default in vtkCamera
    }
  else
    {
    viewAngle = cam->GetViewAngle();
    }
  
  distance = width / (double)tan(viewAngle * vtkMath::Pi() / 360.0);

  // Check view-up vector against view plane normal

  vup = cam->GetViewUp();
  if (fabs(vtkMath::Dot(vup,vn)) > 0.999)
    {
    vtkWarningMacro(<<"Resetting view-up since view plane normal is parallel");
    cam->SetViewUp(-vup[2], vup[0], vup[1]);
    }

  // Update the camera

  cam->SetFocalPoint(center[0], center[1], center[2]);
  cam->SetPosition((double)((double)center[0] + distance * vn[0]),
                   (double)((double)center[1] + distance * vn[1]),
                   (double)((double)center[2] + distance * vn[2]));

  this->GetRenderer()->ResetCameraClippingRange(bounds);

  // Setup default parallel scale

  cam->SetParallelScale(0.5 * width);
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
    this->GetRenderer()->ResetCameraClippingRange();
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
  this->GetRenderer()->AddProp(prop);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddOverlayProp(vtkProp *prop)
{
  this->GetOverlayRenderer()->AddProp(prop);
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::HasProp(vtkProp *prop)
{
  if (this->GetRenderer()->GetProps()->IsItemPresent(prop) ||
      this->GetOverlayRenderer()->GetProps()->IsItemPresent(prop))
    {
    return 1;
    }
  
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveProp(vtkProp *prop)
{
  // safe to call both, vtkViewport does a check first
  this->GetRenderer()->RemoveProp(prop);
  this->GetOverlayRenderer()->RemoveProp(prop);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveAllProps()
{
  this->GetRenderer()->RemoveAllProps();
  this->GetOverlayRenderer()->RemoveAllProps();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetBackgroundColor(double r, double g, double b)
{
  this->Superclass::SetBackgroundColor(r, g, b);

  double color[3];
  this->GetBackgroundColor(color, color + 1, color + 2);
  if (color[0] == r && color[1] == g && color[2] == b)
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
void vtkKWRenderWidget::GetBackgroundColor(double *r, double *g, double *b)
{
  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  if (renderers)
    {
    renderers->InitTraversal();
    vtkRenderer *renderer = renderers->GetNextItem();
    if (renderer)
      {
      renderer->GetBackground(*r, *g, *b);
      return;
      }
    }
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
void vtkKWRenderWidget::SetCornerAnnotationColor(double r, double g, double b)
{
  double *color = this->GetCornerAnnotationColor();
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
double* vtkKWRenderWidget::GetCornerAnnotationColor()
{
  if (!this->CornerAnnotation ||
      !this->CornerAnnotation->GetTextProperty())
    {
    return 0;
    }
  double *color = this->CornerAnnotation->GetTextProperty()->GetColor();
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
void vtkKWRenderWidget::SetHeaderAnnotationColor(double r, double g, double b)
{
  double *color = this->GetHeaderAnnotationColor();
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
double* vtkKWRenderWidget::GetHeaderAnnotationColor()
{
  if (!this->HeaderAnnotation ||
      !this->HeaderAnnotation->GetTextProperty())
    {
    return 0;
    }
  double *color = this->HeaderAnnotation->GetTextProperty()->GetColor();
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
void vtkKWRenderWidget_InteractorTimer(ClientData arg)
{
  vtkRenderWindowInteractor *me = (vtkRenderWindowInteractor*)arg;
  me->InvokeEvent(vtkCommand::TimerEvent);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ProcessEvent(vtkObject *caller,
                                     unsigned long event,
                                     void *calldata)
{
  // Handle the timer event for the generic interactor

  if (caller == this->Interactor)
    {
    switch (event)
      {
      case vtkCommand::CreateTimerEvent:
      case vtkCommand::DestroyTimerEvent:
        if (this->InteractorTimerToken)
          {
          Tcl_DeleteTimerHandler(this->InteractorTimerToken);
          this->InteractorTimerToken = NULL;
          }
        if (event == vtkCommand::CreateTimerEvent)
          {
          this->InteractorTimerToken = 
            Tcl_CreateTimerHandler(
              10, 
              (Tcl_TimerProc*)vtkKWRenderWidget_InteractorTimer, 
              (ClientData)caller);
          }
        break;
      }
    return;
    }

  // Handle event for this class

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
      break;
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
  os << indent << "Renderer: " << this->GetRenderer() << endl;
  os << indent << "CollapsingRenders: " << this->CollapsingRenders << endl;
  os << indent << "DistanceUnits: " 
     << (this->DistanceUnits ? this->DistanceUnits : "(none)") << endl;
  os << indent << "EventIdentifier: " << this->EventIdentifier << endl;
}
