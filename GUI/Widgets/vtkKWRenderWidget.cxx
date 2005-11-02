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
#include "vtkKWIcon.h"
#include "vtkKWGenericRenderWindowInteractor.h"
#include "vtkKWWindow.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkKWMenu.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

#include <vtksys/stl/string>

vtkStandardNewMacro(vtkKWRenderWidget);
vtkCxxRevisionMacro(vtkKWRenderWidget, "1.109");

//----------------------------------------------------------------------------
void vtkKWRenderWidget::Register(vtkObjectBase* o)
{
  this->Superclass::Register(o);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::UnRegister(vtkObjectBase* o)
{
  this->Superclass::UnRegister(o);
}

//----------------------------------------------------------------------------
vtkKWRenderWidget::vtkKWRenderWidget()
{
  // The vtkTkRenderWidget

  this->VTKWidget = vtkKWCoreWidget::New();

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

  vtkCamera *cam = this->GetRenderer()->GetActiveCamera();
  if (cam)
    {
    cam->ParallelProjectionOn();
    }
  vtkRenderer *overlay_ren = this->GetOverlayRenderer();
  if (overlay_ren)
    {
    overlay_ren->SetActiveCamera(cam);
    }

  // Current state (render mode, in expose, printing, etc)

  this->RenderMode         = vtkKWRenderWidget::StillRender;
  this->PreviousRenderMode = this->RenderMode;
  this->RenderState        = 1;
  this->CollapsingRenders  = 0;
  this->InExpose           = 0;
  this->Printing           = 0;

  // Context menu

  this->UseContextMenu = 0;
  this->ContextMenu = NULL;
}

//----------------------------------------------------------------------------
vtkKWRenderWidget::~vtkKWRenderWidget()
{
  this->Close();

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

  if (this->ContextMenu)
    {
    this->ContextMenu->Delete();
    this->ContextMenu = NULL;
    }
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
int vtkKWRenderWidget::GetNumberOfRenderers()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::GetRendererIndex(vtkRenderer *ren)
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
void vtkKWRenderWidget::ComputeVisiblePropBounds(int index, double bounds[6])
{
  vtkRenderer *renderer = this->GetNthRenderer(index);
  if (renderer)
    {
    renderer->ComputeVisiblePropBounds(bounds);
    }
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
void vtkKWRenderWidget::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);
  
  // Create the VTK Tk render widget in VTKWidget

  char opts[256];
  sprintf(opts, "-rw Addr=%p", this->RenderWindow);

  this->VTKWidget->SetParent(this);
  this->VTKWidget->CreateSpecificTkWidget(app, "vtkTkRenderWidget", opts);

  this->Script("grid rowconfigure %s 0 -weight 1", this->GetWidgetName());
  this->Script("grid columnconfigure %s 0 -weight 1", this->GetWidgetName());
  this->Script("grid %s -row 0 -column 0 -sticky nsew", 
               this->VTKWidget->GetWidgetName());
  
  // When the render window is created by the Tk render widget, it
  // is Render()'ed, which calls Initialize() on the interactor, which
  // always reset its Enable state.

  // Make the corner annotation visibile

  this->SetCornerAnnotationVisibility(1);

  // Add the bindings

  this->AddBindings();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddBindings()
{
  if (!this->IsAlive())
    {
    return;
    }

  // First remove the old one so that bindings don't get duplicated

  this->RemoveBindings();

  if (this->VTKWidget->IsAlive())
    {
    // Setup some default bindings
    
    this->VTKWidget->SetBinding("<Expose>", this, "ExposeCallback");
    this->VTKWidget->SetBinding("<Enter>", this, "EnterCallback %x %y");
    this->VTKWidget->SetBinding("<FocusIn>", this, "FocusInCallback");
    this->VTKWidget->SetBinding("<FocusOut>", this, "FocusOutCallback");
    }

  // Many attemps have been made to attach <Configure> to the VTKWidget
  // instead, this sounds more logical since the side effect of the callback
  // is to resize the window, but it seems impossible to do so effetively,
  // the <Configure> event for the VTKWidget is probably called to early
  // in respect to when we can resize the renderwindow
  // Both the vtkRenderWindow and vtkTkRenderWidget have callbacks that
  // react to the window manager's configure event, and as such they
  // resize the render window properly, but this binding is actually only
  // a helper in case the whole widget is resized but we do not want
  // to explicitly 'update' or Render.

  this->SetBinding("<Configure>", this, "ConfigureCallback %w %h");
  
  this->AddInteractionBindings();

  this->AddCallbackCommandObservers();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveBindings()
{
  if (!this->IsAlive())
    {
    return;
    }

  if (this->VTKWidget->IsAlive())
    {
    this->VTKWidget->RemoveBinding("<Expose>");
    this->VTKWidget->RemoveBinding("<Enter>");
    this->VTKWidget->RemoveBinding("<FocusIn>");
    this->VTKWidget->RemoveBinding("<FocusOut>");
    }

  this->RemoveBinding("<Configure>");

  this->RemoveInteractionBindings();

  this->RemoveCallbackCommandObservers();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddInteractionBindings()
{
  if (!this->IsCreated())
    {
    return;
    }

  // First remove the old one so that bindings don't get duplicated

  this->RemoveInteractionBindings();

  // If we are disabled, don't do anything

  if (!this->GetEnabled())
    {
    return;
    }

  if (this->VTKWidget->IsAlive())
    {
    this->VTKWidget->SetBinding(
      "<Any-ButtonPress>", this, "MouseButtonPressCallback %b %x %y 0 0");

    this->VTKWidget->SetBinding(
      "<Any-ButtonRelease>", this, "MouseButtonReleaseCallback %b %x %y");

    this->VTKWidget->SetBinding(
      "<Shift-Any-ButtonPress>", this, "MouseButtonPressCallback %b %x %y 0 1");

    this->VTKWidget->SetBinding(
      "<Shift-Any-ButtonRelease>", this, "MouseButtonReleaseCallback %b %x %y");

    this->VTKWidget->SetBinding(
      "<Control-Any-ButtonPress>", this, "MouseButtonPressCallback %b %x %y 1 0");

    this->VTKWidget->SetBinding(
      "<Control-Any-ButtonRelease>", this, "MouseButtonReleaseCallback %b %x %y");

    this->VTKWidget->SetBinding(
      "<B1-Motion>", this, "MouseMoveCallback 1 %x %y");

    this->VTKWidget->SetBinding(
      "<B2-Motion>", this, "MouseMoveCallback 2 %x %y");
  
    this->VTKWidget->SetBinding(
      "<B3-Motion>", this, "MouseMoveCallback 3 %x %y");

    this->VTKWidget->SetBinding(
      "<Shift-B1-Motion>", this, "MouseMoveCallback 1 %x %y");

    this->VTKWidget->SetBinding(
      "<Shift-B2-Motion>", this, "MouseMoveCallback 2 %x %y");
  
    this->VTKWidget->SetBinding(
      "<Shift-B3-Motion>", this, "MouseMoveCallback 3 %x %y");

    this->VTKWidget->SetBinding(
      "<Control-B1-Motion>", this, "MouseMoveCallback 1 %x %y");

    this->VTKWidget->SetBinding(
      "<Control-B2-Motion>", this, "MouseMoveCallback 2 %x %y");
  
    this->VTKWidget->SetBinding(
      "<Control-B3-Motion>", this, "MouseMoveCallback 3 %x %y");

    this->VTKWidget->SetBinding(
      "<KeyPress>", this, "KeyPressCallback %A %x %y 0 0 %K");
  
    this->VTKWidget->SetBinding(
      "<Shift-KeyPress>", this, "KeyPressCallback %A %x %y 0 1 %K");
  
    this->VTKWidget->SetBinding(
      "<Control-KeyPress>", this, "KeyPressCallback %A %x %y 1 0 %K");
  
    this->VTKWidget->SetBinding(
      "<Motion>", this, "MouseMoveCallback 0 %x %y");

    this->VTKWidget->SetBinding(
      "<MouseWheel>", this, "MouseWheelCallback %D");
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveInteractionBindings()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->VTKWidget->IsAlive())
    {
    this->VTKWidget->RemoveBinding("<Any-ButtonPress>");
    this->VTKWidget->RemoveBinding("<Any-ButtonRelease>");
    this->VTKWidget->RemoveBinding("<Shift-Any-ButtonPress>");
    this->VTKWidget->RemoveBinding("<Shift-Any-ButtonRelease>");
    this->VTKWidget->RemoveBinding("<Control-Any-ButtonPress>");
    this->VTKWidget->RemoveBinding("<Control-Any-ButtonRelease>");

    this->VTKWidget->RemoveBinding("<B1-Motion>");
    this->VTKWidget->RemoveBinding("<B2-Motion>");
    this->VTKWidget->RemoveBinding("<B3-Motion>");

    this->VTKWidget->RemoveBinding("<Shift-B1-Motion>");
    this->VTKWidget->RemoveBinding("<Shift-B2-Motion>");
    this->VTKWidget->RemoveBinding("<Shift-B3-Motion>");

    this->VTKWidget->RemoveBinding("<Control-B1-Motion>");
    this->VTKWidget->RemoveBinding("<Control-B2-Motion>");
    this->VTKWidget->RemoveBinding("<Control-B3-Motion>");

    this->VTKWidget->RemoveBinding("<KeyPress>");
    this->VTKWidget->RemoveBinding("<Shift-KeyPress>");
    this->VTKWidget->RemoveBinding("<Control-KeyPress>");

    this->VTKWidget->RemoveBinding("<Motion>");
    this->VTKWidget->RemoveBinding("<MouseWheel>");
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::MouseMoveCallback(int vtkNotUsed(num), int x, int y)
{
  this->Interactor->SetEventPositionFlipY(x, y);
  this->Interactor->MouseMoveEvent();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::MouseWheelCallback(int delta)
{
  if (delta < 0)
    {
    this->Interactor->MouseWheelBackwardEvent();
    }
  else
    {
    this->Interactor->MouseWheelForwardEvent();
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::MouseButtonPressCallback(int num, int x, int y,
                                     int ctrl, int shift)
{
  this->VTKWidget->Focus();
  
  if (this->UseContextMenu && num == 3)
    {
    if (!this->ContextMenu)
      {
      this->ContextMenu = vtkKWMenu::New();
      }
    if (!this->ContextMenu->IsCreated())
      {
      this->ContextMenu->Create(this->GetApplication());
      }
    this->ContextMenu->DeleteAllMenuItems();
    this->PopulateContextMenu(this->ContextMenu);
    if (this->ContextMenu->GetNumberOfItems())
      {
      this->Script("tk_popup %s [winfo pointerx %s] [winfo pointery %s]", 
                   this->ContextMenu->GetWidgetName(), 
                   this->VTKWidget->GetWidgetName(), 
                   this->VTKWidget->GetWidgetName());
      }
    }
  else
    {
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
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::MouseButtonReleaseCallback(int num, int x, int y)
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
void vtkKWRenderWidget::KeyPressCallback(char key, 
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
void vtkKWRenderWidget::ConfigureCallback(int width, int height)
{
  this->Interactor->UpdateSize(width, height);
  this->Interactor->ConfigureEvent();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ExposeCallback()
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
void vtkKWRenderWidget::PopulateContextMenu(vtkKWMenu *menu)
{
  this->PopulateAnnotationMenu(menu);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::PopulateAnnotationMenu(vtkKWMenu *menu)
{
  if (!menu)
    {
    return;
    }

  if (menu->GetNumberOfItems())
    {
    menu->AddSeparator();
    }

  vtksys_stl::string label;
  char *buttonvar;
  int show_icons = 0;

  // Corner Annotation

  label = "Corner Annotation";
  buttonvar = menu->CreateCheckButtonVariable(
    this, "CornerAnnotationVisibility");
  menu->AddCheckButton(
    label.c_str(), buttonvar, this, "ToggleCornerAnnotationVisibility");
  menu->CheckCheckButton(
    this, "CornerAnnotationVisibility", this->GetCornerAnnotationVisibility());
  delete [] buttonvar;
  if (show_icons)
    {
    menu->SetItemImageToPredefinedIcon(
      label.c_str(), vtkKWIcon::IconCornerAnnotation);
    menu->SetItemCompoundMode(label.c_str(), 1);
    }

  // Header Annotation

  label = "Header Annotation";
  buttonvar = menu->CreateCheckButtonVariable(
    this, "HeaderAnnotationVisibility");
  menu->AddCheckButton(
    label.c_str(), buttonvar, this, "ToggleHeaderAnnotationVisibility");
  menu->CheckCheckButton(
    this, "HeaderAnnotationVisibility", this->GetHeaderAnnotationVisibility());
  delete [] buttonvar;
  if (show_icons)
    {
    menu->SetItemImageToPredefinedIcon(
      label.c_str(), vtkKWIcon::IconHeaderAnnotation);
    menu->SetItemCompoundMode(label.c_str(), 1);
    }
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
  int i, nb_renderers = this->GetNumberOfRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthRenderer(i);
    if (renderer)
      {
      double bounds[6];
      this->ComputeVisiblePropBounds(i, bounds);
      if (bounds[0] == VTK_LARGE_FLOAT)
        {
        vtkDebugMacro(<< "Cannot reset camera!");
        return;
        }

      double vn[3];
      vtkCamera *cam = renderer->GetActiveCamera();
      if (cam != NULL)
        {
        cam->GetViewPlaneNormal(vn);
        }
      else
        {
        vtkErrorMacro(<< "Trying to reset non-existant camera");
        return;
        }

      double center[3];
      center[0] = ((bounds[0] + bounds[1]) / 2.0);
      center[1] = ((bounds[2] + bounds[3]) / 2.0);
      center[2] = ((bounds[4] + bounds[5]) / 2.0);

      double aspect[2];
      renderer->ComputeAspect();
      renderer->GetAspect(aspect);

      double distance, width, viewAngle, *vup;

      // Check Y and Z for the Y axis on the window

      width = (bounds[3] - bounds[2]) / aspect[1];
      
      if (((bounds[5] - bounds[4]) / aspect[1]) > width) 
        {
        width = (bounds[5] - bounds[4]) / aspect[1];
        }
      
      // Check X and Y for the X axis on the window

      if (((bounds[1] - bounds[0]) / aspect[0]) > width) 
        {
        width = (bounds[1] - bounds[0]) / aspect[0];
        }

      if (((bounds[3] - bounds[2]) / aspect[0]) > width) 
        {
        width = (bounds[3] - bounds[2]) / aspect[0];
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
      if (fabs(vtkMath::Dot(vup, vn)) > 0.999)
        {
        vtkWarningMacro(
          "Resetting view-up since view plane normal is parallel");
        cam->SetViewUp(-vup[2], vup[0], vup[1]);
        }

      // Update the camera
      
      cam->SetFocalPoint(center[0], center[1], center[2]);
      cam->SetPosition((center[0] + distance * vn[0]),
                       (center[1] + distance * vn[1]),
                       (center[2] + distance * vn[2]));

      // Setup default parallel scale
      
      cam->SetParallelScale(0.5 * width);
      }
    }

  this->ResetCameraClippingRange();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ResetCameraClippingRange()
{
  int i, nb_renderers = this->GetNumberOfRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthRenderer(i);
    if (renderer)
      {
      double bounds[6];
      this->ComputeVisiblePropBounds(i, bounds);
      renderer->ResetCameraClippingRange(bounds);
      }
    }
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

  if (this->RenderMode != vtkKWRenderWidget::DisabledRender)
    {
    this->ResetCameraClippingRange();
    this->RenderWindow->Render();
    }
  
  static_in_render = 0;
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::GetOffScreenRendering()
{
  if (this->GetRenderWindow())
    {
    return this->GetRenderWindow()->GetOffScreenRendering();
    }
  return 0;
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
  double scale;
  int cxDIB = screenSizeX;         // Size of DIB - x
  int cyDIB = screenSizeY;         // Size of DIB - y
  
  // target DPI specified here
  if (this->GetApplication())
    {
    scale = printerDPIX/this->GetApplication()->GetPrintTargetDPI();
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
void vtkKWRenderWidget::AddViewProp(vtkProp *prop)
{
  int i, nb_renderers = this->GetNumberOfRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthRenderer(i);
    if (renderer)
      {
      renderer->AddViewProp(prop);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddOverlayViewProp(vtkProp *prop)
{
  vtkRenderer *overlay_ren = this->GetOverlayRenderer();
  if (overlay_ren)
    {
    overlay_ren->AddViewProp(prop);
    }
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::HasViewProp(vtkProp *prop)
{
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderer *overlay_ren = this->GetOverlayRenderer();
  if ((ren && ren->GetViewProps()->IsItemPresent(prop)) ||
      (overlay_ren && overlay_ren->GetViewProps()->IsItemPresent(prop)))
    {
    return 1;
    }
  
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveViewProp(vtkProp* prop)
{
  // safe to call both, vtkViewport does a check first

  int i, nb_renderers = this->GetNumberOfRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthRenderer(i);
    if (renderer)
      {
      renderer->RemoveViewProp(prop);
      }
    }

  vtkRenderer *overlay_ren = this->GetOverlayRenderer();
  if (overlay_ren)
    {
    overlay_ren->RemoveViewProp(prop);
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveAllViewProps()
{
  int i, nb_renderers = this->GetNumberOfRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthRenderer(i);
    if (renderer)
      {
      renderer->RemoveAllViewProps();
      }
    }

  vtkRenderer *overlay_ren = this->GetOverlayRenderer();
  if (overlay_ren)
    {
    overlay_ren->RemoveAllViewProps();
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::SetRendererBackgroundColor(double r, double g, double b)
{
  double color[3];
  this->GetRendererBackgroundColor(color, color + 1, color + 2);
  if (color[0] == r && color[1] == g && color[2] == b)
    {
    return;
    }

  if (r < 0 || g < 0 || b < 0)
    {
    return;
    }
  
  int nb_renderers = this->GetNumberOfRenderers();
  for (int i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthRenderer(i);
    if (renderer)
      {
      renderer->SetBackground(r, g, b);
      }
    }

  this->Render();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::GetRendererBackgroundColor(double *r, double *g, double *b)
{
  int nb_renderers = this->GetNumberOfRenderers();
  for (int i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthRenderer(i);
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
  this->RemoveAllViewProps();

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
          this->HasViewProp(this->CornerAnnotation) && 
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
    if (!this->HasViewProp(this->CornerAnnotation))
      {
      this->AddOverlayViewProp(this->CornerAnnotation);
      }
    }
  else
    {
    this->CornerAnnotation->VisibilityOff();
    if (this->HasViewProp(this->CornerAnnotation))
      {
      this->RemoveViewProp(this->CornerAnnotation);
      }
    }

  this->Render();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ToggleCornerAnnotationVisibility()
{
  this->SetCornerAnnotationVisibility(!this->GetCornerAnnotationVisibility());
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
          this->HasViewProp(this->HeaderAnnotation) && 
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
    if (!this->HasViewProp(this->HeaderAnnotation))
      {
      this->AddOverlayViewProp(this->HeaderAnnotation);
      }
    }
  else
    {
    this->HeaderAnnotation->VisibilityOff();
    if (this->HasViewProp(this->HeaderAnnotation))
      {
      this->RemoveViewProp(this->HeaderAnnotation);
      }
    }

  this->Render();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ToggleHeaderAnnotationVisibility()
{
  this->SetHeaderAnnotationVisibility(!this->GetHeaderAnnotationVisibility());
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
  if (r)
    {
    this->CollapsingRenders = 1;
    this->CollapsingRendersCount = 0;
    }
  else
    {
    this->CollapsingRenders = 0;
    if (this->CollapsingRendersCount)
      {
      this->Render();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget_InteractorTimer(ClientData arg)
{
  vtkRenderWindowInteractor *me = (vtkRenderWindowInteractor*)arg;
  me->InvokeEvent(vtkCommand::TimerEvent);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddCallbackCommandObservers()
{
  this->Superclass::AddCallbackCommandObservers();

  this->AddCallbackCommandObserver(
    this->Interactor, vtkCommand::CreateTimerEvent);
  this->AddCallbackCommandObserver(
    this->Interactor, vtkCommand::DestroyTimerEvent);

  this->AddCallbackCommandObserver(
    this->RenderWindow, vtkCommand::CursorChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveCallbackCommandObservers()
{
  this->Superclass::RemoveCallbackCommandObservers();

  this->RemoveCallbackCommandObserver(
    this->Interactor, vtkCommand::CreateTimerEvent);
  this->RemoveCallbackCommandObserver(
    this->Interactor, vtkCommand::DestroyTimerEvent);

  this->RemoveCallbackCommandObserver(
    this->RenderWindow, vtkCommand::CursorChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ProcessCallbackCommandEvents(vtkObject *caller,
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

  if (caller == this->RenderWindow)
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
          this->GetParentWindow()->SetConfigurationOption("-cursor", cptr);
          }
        break;
      }
    }

  this->Superclass::ProcessCallbackCommandEvents(caller, event, calldata);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // If enabled back, set up the bindings, otherwise remove

  if (this->GetEnabled())
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
  os << indent << "RenderMode: " << this->RenderMode << endl;
  os << indent << "RenderState: " << this->RenderState << endl;
  os << indent << "Renderer: " << this->GetRenderer() << endl;
  os << indent << "CollapsingRenders: " << this->CollapsingRenders << endl;
  os << indent << "DistanceUnits: " 
     << (this->DistanceUnits ? this->DistanceUnits : "(none)") << endl;
}
