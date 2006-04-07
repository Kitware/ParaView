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

#include "vtkKWApplication.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCornerAnnotation.h"
#include "vtkKWEvent.h"
#include "vtkKWGenericRenderWindowInteractor.h"
#include "vtkKWIcon.h"
#include "vtkKWInternationalization.h"
#include "vtkKWMenu.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWindow.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkInteractorStyleSwitch.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

#include <vtksys/stl/string>
#include <vtksys/stl/vector>

vtkStandardNewMacro(vtkKWRenderWidget);
vtkCxxRevisionMacro(vtkKWRenderWidget, "1.132");

//----------------------------------------------------------------------------
class vtkKWRenderWidgetInternals
{
public:

  typedef vtksys_stl::vector<vtkRenderer*> RendererPoolType;
  typedef vtksys_stl::vector<vtkRenderer*>::iterator RendererPoolIterator;

  RendererPoolType RendererPool;
  RendererPoolType OverlayRendererPool;
};

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
  this->Internals = new vtkKWRenderWidgetInternals;

  // The vtkTkRenderWidget

  this->VTKWidget = vtkKWCoreWidget::New();

  // Create a render window

  this->RenderWindow = vtkRenderWindow::New();
  this->RenderWindow->SetNumberOfLayers(2);

  // Create a default (generic) interactor, which is pretty much
  // the only way to interact with a VTK Tk render widget

  this->Interactor = vtkKWGenericRenderWindowInteractor::New();
  this->Interactor->SetRenderWidget(this);  
  this->Interactor->SetRenderWindow(this->RenderWindow);
  this->InteractorTimerToken = NULL;

  // Switch to trackball style, it's nicer

  vtkInteractorStyleSwitch *istyle = vtkInteractorStyleSwitch::SafeDownCast(
    this->Interactor->GetInteractorStyle());
  if (istyle)
    {
    istyle->SetCurrentStyleToTrackballCamera();
    }

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

  // Remove all renderers

  this->RemoveAllRenderers();
  this->RemoveAllOverlayRenderers();

  // Delete our pool

  delete this->Internals;

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
void vtkKWRenderWidget::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();
  
  // Create the default renderers

  this->CreateDefaultRenderers();

  // Get the first renderer camera, set it to parallel
  // then make sure all renderers use the same

  vtkRenderer *ren = this->GetNthRenderer(0);
  if (ren)
    {
    vtkCamera *cam = ren->GetActiveCamera();
    if (cam)
      {
      cam->ParallelProjectionOn();
      int i, nb_renderers = this->GetNumberOfRenderers();
      for (i = 1; i < nb_renderers; i++)
        {
        ren = this->GetNthRenderer(i);
        if (ren)
          {
          ren->SetActiveCamera(cam);
          }
        }
      nb_renderers = this->GetNumberOfOverlayRenderers();
      for (i = 0; i < nb_renderers; i++)
        {
        ren = this->GetNthOverlayRenderer(i);
        if (ren)
          {
          ren->SetActiveCamera(cam);
          }
        }
      }
    }

  // Install the renderers

  this->InstallRenderers();

  // Create the VTK Tk render widget in VTKWidget

  char opts[256];
  sprintf(opts, "-rw Addr=%p", this->RenderWindow);

  this->VTKWidget->SetParent(this);
  this->VTKWidget->CreateSpecificTkWidget("vtkTkRenderWidget", opts);

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
void vtkKWRenderWidget::CreateDefaultRenderers()
{
  // Create two renderers by default (main one and overlay)

  if (this->GetNumberOfRenderers() <= 0)
    {
    vtkRenderer *renderer = vtkRenderer::New();
    this->AddRenderer(renderer);
    renderer->Delete();
    }
 
  if (this->GetNumberOfOverlayRenderers() <= 0)
    {
    vtkRenderer *renderer = vtkRenderer::New();
    this->AddOverlayRenderer(renderer);
    renderer->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::InstallRenderers()
{
  if (!this->RenderWindow)
    {
    return;
    }

  this->RenderWindow->GetRenderers()->RemoveAllItems();

  int i, nb_renderers = this->GetNumberOfOverlayRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthOverlayRenderer(i);
    if (renderer)
      {
      this->RenderWindow->AddRenderer(renderer);
      }
    }

  nb_renderers = this->GetNumberOfRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthRenderer(i);
    if (renderer)
      {
      this->RenderWindow->AddRenderer(renderer);
      }
    }
}

//----------------------------------------------------------------------------
vtkRenderer* vtkKWRenderWidget::GetNthRenderer(int index)
{
  if (index < 0 || index >= this->GetNumberOfRenderers())
    {
    return NULL;
    }

  return this->Internals->RendererPool[index];
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::GetNumberOfRenderers()
{
  return this->Internals->RendererPool.size();
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::GetRendererIndex(vtkRenderer *ren)
{
  vtkKWRenderWidgetInternals::RendererPoolIterator it = 
    this->Internals->RendererPool.begin();
  vtkKWRenderWidgetInternals::RendererPoolIterator end = 
    this->Internals->RendererPool.end();
  int rank = 0;
  for (; it != end; ++it, ++rank)
    {
    if (*it == ren)
      {
      return rank;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddRenderer(vtkRenderer *ren)
{
  if (this->GetRendererIndex(ren) >= 0)
    {
    return;
    }

  ren->SetLayer(0);
  this->Internals->RendererPool.push_back(ren);
  ren->Register(this);

  this->InstallRenderers();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveRenderer(vtkRenderer *ren)
{
  this->RemoveNthRenderer(this->GetRendererIndex(ren));
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveNthRenderer(int index)
{
  if (index < 0 || index >= this->GetNumberOfRenderers())
    {
    return;
    }

  vtkKWRenderWidgetInternals::RendererPoolIterator it = 
    this->Internals->RendererPool.begin() + index;
  (*it)->RemoveAllViewProps();
  (*it)->Delete();
  this->Internals->RendererPool.erase(it);
  this->InstallRenderers();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveAllRenderers()
{
  vtkKWRenderWidgetInternals::RendererPoolIterator it = 
    this->Internals->RendererPool.begin();
  vtkKWRenderWidgetInternals::RendererPoolIterator end = 
    this->Internals->RendererPool.end();
  for (; it != end; ++it)
    {
    (*it)->RemoveAllViewProps();
    (*it)->Delete();
    }
    
  this->Internals->RendererPool.clear();

  this->InstallRenderers();
}

//----------------------------------------------------------------------------
vtkRenderer* vtkKWRenderWidget::GetNthOverlayRenderer(int index)
{
  if (index < 0 || index >= this->GetNumberOfOverlayRenderers())
    {
    return NULL;
    }

  return this->Internals->OverlayRendererPool[index];
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::GetNumberOfOverlayRenderers()
{
  return this->Internals->OverlayRendererPool.size();
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::GetOverlayRendererIndex(vtkRenderer *ren)
{
  vtkKWRenderWidgetInternals::RendererPoolIterator it = 
    this->Internals->OverlayRendererPool.begin();
  vtkKWRenderWidgetInternals::RendererPoolIterator end = 
    this->Internals->OverlayRendererPool.end();
  int rank = 0;
  for (; it != end; ++it, ++rank)
    {
    if (*it == ren)
      {
      return rank;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddOverlayRenderer(vtkRenderer *ren)
{
  if (this->GetOverlayRendererIndex(ren) >= 0)
    {
    return;
    }

  ren->SetLayer(1);
  this->Internals->OverlayRendererPool.push_back(ren);
  ren->Register(this);

  this->InstallRenderers();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveOverlayRenderer(vtkRenderer *ren)
{
  this->RemoveNthOverlayRenderer(this->GetOverlayRendererIndex(ren));
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveNthOverlayRenderer(int index)
{
  if (index < 0 || index >= this->GetNumberOfOverlayRenderers())
    {
    return;
    }

  vtkKWRenderWidgetInternals::RendererPoolIterator it = 
    this->Internals->OverlayRendererPool.begin() + index;
  (*it)->RemoveAllViewProps();
  (*it)->Delete();
  this->Internals->OverlayRendererPool.erase(it);
  this->InstallRenderers();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::RemoveAllOverlayRenderers()
{
  vtkKWRenderWidgetInternals::RendererPoolIterator it = 
    this->Internals->OverlayRendererPool.begin();
  vtkKWRenderWidgetInternals::RendererPoolIterator end = 
    this->Internals->OverlayRendererPool.end();
  for (; it != end; ++it)
    {
    (*it)->RemoveAllViewProps();
    (*it)->Delete();
    }
    
  this->Internals->OverlayRendererPool.clear();
  this->InstallRenderers();
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
void vtkKWRenderWidget::AddViewPropToNthRenderer(vtkProp *p, int index)
{
  vtkRenderer *ren = this->GetNthRenderer(index);
  if (ren && !ren->GetViewProps()->IsItemPresent(p))
    {
    ren->AddViewProp(p);
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddOverlayViewProp(vtkProp *prop)
{
  int i, nb_renderers = this->GetNumberOfOverlayRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthOverlayRenderer(i);
    if (renderer)
      {
      renderer->AddViewProp(prop);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::AddViewPropToNthOverlayRenderer(vtkProp *p, int index)
{
  vtkRenderer *ren = this->GetNthOverlayRenderer(index);
  if (ren && !ren->GetViewProps()->IsItemPresent(p))
    {
    ren->AddViewProp(p);
    }
}

//----------------------------------------------------------------------------
int vtkKWRenderWidget::HasViewProp(vtkProp *prop)
{
  int i, nb_renderers = this->GetNumberOfRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthRenderer(i);
    if (renderer && renderer->GetViewProps()->IsItemPresent(prop))
      {
      return 1;
      }
    }

  nb_renderers = this->GetNumberOfOverlayRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthOverlayRenderer(i);
    if (renderer && renderer->GetViewProps()->IsItemPresent(prop))
      {
      return 1;
      }
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

  nb_renderers = this->GetNumberOfOverlayRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthOverlayRenderer(i);
    if (renderer)
      {
      renderer->RemoveViewProp(prop);
      }
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

  nb_renderers = this->GetNumberOfOverlayRenderers();
  for (i = 0; i < nb_renderers; i++)
    {
    vtkRenderer *renderer = this->GetNthOverlayRenderer(i);
    if (renderer)
      {
      renderer->RemoveAllViewProps();
      }
    }
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
    typedef struct
    {
      const char *Modifier;
      int Ctrl;
      int Shift;
    } EventTranslator;
    EventTranslator translators[] = {
      { "",               0, 0 },
      { "Shift-",         0, 1 },
      { "Control-",       1, 0 },
      { "Control-Shift-", 1, 1 }
    };
    char event[256];
    char callback[256];
    for (size_t i = 0; i < sizeof(translators) / sizeof(translators[0]); i++)
      {
      sprintf(event, "<%sAny-ButtonPress>", translators[i].Modifier);
      sprintf(callback, "MouseButtonPressCallback %%b %%x %%y %d %d 0", 
              translators[i].Ctrl, translators[i].Shift);
      this->VTKWidget->SetBinding(event, this, callback);

      sprintf(event, "<Double-%sAny-ButtonPress>", translators[i].Modifier);
      sprintf(callback, "MouseButtonPressCallback %%b %%x %%y %d %d 1", 
              translators[i].Ctrl, translators[i].Shift);
      this->VTKWidget->SetBinding(event, this, callback);

      sprintf(event, "<%sAny-ButtonRelease>", translators[i].Modifier);
      sprintf(callback, "MouseButtonReleaseCallback %%b %%x %%y %d %d", 
              translators[i].Ctrl, translators[i].Shift);
      this->VTKWidget->SetBinding(event, this, callback);

      sprintf(event, "<%sMotion>", translators[i].Modifier);
      sprintf(callback, "MouseMoveCallback 0 %%x %%y %d %d", 
              translators[i].Ctrl, translators[i].Shift);
      this->VTKWidget->SetBinding(event, this, callback);
        
      for (int b = 1; b <= 3; b++)
        {
        sprintf(event, "<%sB%d-Motion>", translators[i].Modifier, b);
        sprintf(callback, "MouseMoveCallback %d %%x %%y %d %d", 
                b, translators[i].Ctrl, translators[i].Shift);
        this->VTKWidget->SetBinding(event, this, callback);
        }

      sprintf(event, "<%sMouseWheel>", translators[i].Modifier);
      sprintf(callback, "MouseWheelCallback %%D %d %d", 
              translators[i].Ctrl, translators[i].Shift);
      this->VTKWidget->SetBinding(event, this, callback);
        
      sprintf(event, "<%sKeyPress>", translators[i].Modifier);
      sprintf(callback, "KeyPressCallback %%A %%x %%y %d %d %%K", 
              translators[i].Ctrl, translators[i].Shift);
      this->VTKWidget->SetBinding(event, this, callback);
      }
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
    typedef struct
    {
      const char *Modifier;
      int Ctrl;
      int Shift;
    } EventTranslator;
    EventTranslator translators[] = {
      { "",               0, 0 },
      { "Shift-",         0, 1 },
      { "Control-",       1, 0 },
      { "Control-Shift-", 1, 1 }
    };
    char event[256];
    for (size_t i = 0; i < sizeof(translators) / sizeof(translators[0]); i++)
      {
      sprintf(event, "<%sAny-ButtonPress>", translators[i].Modifier);
      this->VTKWidget->RemoveBinding(event);

      sprintf(event, "<Double-%sAny-ButtonPress>", translators[i].Modifier);
      this->VTKWidget->RemoveBinding(event);

      sprintf(event, "<%sAny-ButtonRelease>", translators[i].Modifier);
      this->VTKWidget->RemoveBinding(event);

      sprintf(event, "<%sMotion>", translators[i].Modifier);
      this->VTKWidget->RemoveBinding(event);
        
      for (int b = 1; b <= 3; b++)
        {
        sprintf(event, "<%sB%d-Motion>", translators[i].Modifier, b);
        this->VTKWidget->RemoveBinding(event);
        }

      sprintf(event, "<%sMouseWheel>", translators[i].Modifier);
      this->VTKWidget->RemoveBinding(event);
        
      sprintf(event, "<%sKeyPress>", translators[i].Modifier);
      this->VTKWidget->RemoveBinding(event);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::MouseMoveCallback(
  int vtkNotUsed(num), int x, int y, int ctrl, int shift)
{
  this->Interactor->SetEventInformationFlipY(x, y, ctrl, shift);
  this->Interactor->MouseMoveEvent();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::MouseWheelCallback(int delta, int ctrl, int shift)
{
  this->Interactor->SetControlKey(ctrl);
  this->Interactor->SetShiftKey(shift);

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
void vtkKWRenderWidget::MouseButtonPressCallback(
  int num, int x, int y, int ctrl, int shift, int repeat)
{
  this->VTKWidget->Focus();
  
  if (this->UseContextMenu && num == 3 && repeat == 0)
    {
    if (!this->ContextMenu)
      {
      this->ContextMenu = vtkKWMenu::New();
      }
    if (!this->ContextMenu->IsCreated())
      {
      this->ContextMenu->SetParent(this);
      this->ContextMenu->Create();
      }
    this->ContextMenu->DeleteAllItems();
    this->PopulateContextMenu(this->ContextMenu);
    if (this->ContextMenu->GetNumberOfItems())
      {
      int px, py;
      vtkKWTkUtilities::GetMousePointerCoordinates(this->VTKWidget, &px, &py);
      this->ContextMenu->PopUp(px, py);
      }
    }
  else
    {
    this->Interactor->SetEventInformationFlipY(x, y, ctrl, shift, 0, repeat);
    
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
void vtkKWRenderWidget::MouseButtonReleaseCallback(
  int num, int x, int y, int ctrl, int shift)
{
  this->Interactor->SetEventInformationFlipY(x, y, ctrl, shift);
  
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
  this->Interactor->SetEventInformationFlipY(
    x, y, ctrl, shift, key, 0, keysym);
  this->Interactor->CharEvent();
}

//----------------------------------------------------------------------------
void vtkKWRenderWidget::ConfigureCallback(int width, int height)
{
  // When calling the superclass's SetWidth or SetHeight, the
  // other field will be set to 1 (i.e. a width/height of 0 for a Tk frame
  // translates to a size 1 in that dimension). Fix that.

  if (width <= 1)
    {
    width = this->Interactor->GetSize()[0];
    }
  if (height <= 1)
    {
    height = this->Interactor->GetSize()[1];
    }
  //Interactor->GetSize() can return 0. in that case set
  //the size to 1.
  if(width==0) width=1;
  if(height==0) height=1;
  // We *need* to propagate the size to the vtkTkRenderWidget
  // if we specified the widget's width/height explicitly

  int frame_width = this->GetWidth();
  if (frame_width)
    {
    width = frame_width;
    }
  int frame_height = this->GetHeight();
  if (frame_height)
    {
    height = frame_height;
    }
  if (frame_width || frame_height)
    {    
    this->VTKWidget->SetConfigurationOptionAsInt("-width", width);
    this->VTKWidget->SetConfigurationOptionAsInt("-height", height);
    }

  // Propagate to the interactor too, for safety

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
  this->GetApplication()->ProcessPendingEvents();
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

  int index, show_icons = 0;

  // Corner Annotation

  index = menu->AddCheckButton(
    ks_("Annotation|Corner Annotation"), 
    this, "ToggleCornerAnnotationVisibility");
  menu->SetItemSelectedState(index, this->GetCornerAnnotationVisibility());
  if (show_icons)
    {
    menu->SetItemImageToPredefinedIcon(
      index, vtkKWIcon::IconCornerAnnotation);
    menu->SetItemCompoundMode(index, 1);
    }

  // Header Annotation

  const char *header = this->GetHeaderAnnotationText();
  if (header && header)
    {
    index = menu->AddCheckButton(
      ks_("Annotation|Header Annotation"), 
      this, "ToggleHeaderAnnotationVisibility");
    menu->SetItemSelectedState(index, this->GetHeaderAnnotationVisibility());
    if (show_icons)
      {
      menu->SetItemImageToPredefinedIcon(
        index, vtkKWIcon::IconHeaderAnnotation);
      menu->SetItemCompoundMode(index, 1);
      }
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
      renderer->ResetCamera();
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

  if (this->RenderMode != vtkKWRenderWidget::DisabledRender &&
      this->VTKWidget->IsCreated())
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
#if defined(_WIN32) && !defined(__CYGWIN__)
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
void vtkKWRenderWidget::SetupMemoryRendering( int x, int y, void *cd)
{
#ifdef _WIN32
  if (!cd)
    {
    cd = this->RenderWindow->GetGenericContext();
    }
  vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->SetupMemoryRendering(x, y, (HDC)cd);
#else
  (void)x; (void)y; (void)cd;
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

#if 0
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
        if (this->GetParentTopLevel())
          {
          this->GetParentTopLevel()->SetConfigurationOption("-cursor", cptr);
          }
        break;
      }
    }
#endif

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
