/*=========================================================================

  Module:    vtkKWRenderWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWRenderWidget - a render widget
// .SECTION Description
// This class encapsulates a render window, a renderer and several other
// objects inside a single widget. Actors and props can be added,
// annotations can be set.
// .WARNING
// This widget set the camera to be in parallel projection mode.
// You can change this default (after Create()) by calling:
// renderwidget->GetRenderer()->GetActiveCamera()->ParallelProjectionOff();

#ifndef __vtkKWRenderWidget_h
#define __vtkKWRenderWidget_h

#include "vtkKWCompositeWidget.h"
#include "vtkWindows.h" // needed for RECT HDC

class vtkCamera;
class vtkCornerAnnotation;
class vtkKWGenericRenderWindowInteractor;
class vtkProp;
class vtkRenderWindow;
class vtkRenderer;
class vtkTextActor;
class vtkKWMenu;
class vtkKWRenderWidgetInternals;

class KWWidgets_EXPORT vtkKWRenderWidget : public vtkKWCompositeWidget
{
public:
  static vtkKWRenderWidget* New();
  vtkTypeRevisionMacro(vtkKWRenderWidget, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Close the widget. 
  // This method brings the widget back to an empty/clean state. 
  // It removes all the actors/props, removes the bindings, resets the
  // annotations, etc.
  virtual void Close();
  
  // Description:
  // Render the scene.
  virtual void Render();

  // Description:
  // Enable/disable rendering.
  vtkGetMacro(RenderState, int);
  vtkSetClampMacro(RenderState, int, 0, 1);
  vtkBooleanMacro(RenderState, int);
  
  // Description:
  // Set/get the rendering mode.
  //BTX
  enum
  {
    InteractiveRender = 0,
    StillRender       = 1,
    DisabledRender    = 2,
    SingleRender      = 3
  };
  //ETX
  vtkSetClampMacro(RenderMode, int, 
                   vtkKWRenderWidget::InteractiveRender, 
                   vtkKWRenderWidget::SingleRender);
  vtkGetMacro(RenderMode, int);
  virtual void SetRenderModeToInteractive() 
    { this->SetRenderMode(vtkKWRenderWidget::InteractiveRender); };
  virtual void SetRenderModeToStill() 
    { this->SetRenderMode(vtkKWRenderWidget::StillRender); };
  virtual void SetRenderModeToSingle() 
    { this->SetRenderMode(vtkKWRenderWidget::SingleRender); };
  virtual void SetRenderModeToDisabled() 
    { this->SetRenderMode(vtkKWRenderWidget::DisabledRender); };

  // Description:
  // Set/Get the collapsing of renders. If this is set to true, then
  // all call to Render() will be collapsed. Once this is set to false, if
  // there are any pending render requests, the widget will render.
  virtual void SetCollapsingRenders(int);
  vtkBooleanMacro(CollapsingRenders, int);
  vtkGetMacro(CollapsingRenders, int);

  // Description:
  // Reset the widget. 
  // This implementation calls ResetCamera() and Render().
  virtual void Reset();

  // Description:
  // Reset the camera to display all the actors in the scene, or just
  // the camera clipping range.
  // This is done for each renderer (if multiple renderers are supported).
  // Note that no default renderers exist before Create() is called.
  virtual void ResetCamera();
  virtual void ResetCameraClippingRange();
  
  // Description:
  // Add/remove the widget bindings.
  // The AddBindings() method sets up general bindings like the Expose or
  // Configure events so that the scene is rendered properly when the widget
  // is mapped to the screen. It also calls the AddInteractionBindings() 
  // which sets up interaction bindings like mouse events, keyboard events, 
  // etc. The AddBindings() method is called automatically when the widget
  // is created by the Create() method. Yet, the methods are public so
  // that one can temporarily enable or disable the bindings to limit
  // the interaction with this widget.
  virtual void AddBindings();
  virtual void RemoveBindings();
  virtual void AddInteractionBindings();
  virtual void RemoveInteractionBindings();
  
  // Description:
  // Convenience method to set the visibility of all annotations.
  // Subclasses should override this method to propagate this visibility
  // flag to their own annotations.
  virtual void SetAnnotationsVisibility(int v);
  vtkBooleanMacro(AnnotationsVisibility, int);

  // Description:
  // Get and control the corner annotation.
  virtual void SetCornerAnnotationVisibility(int v);
  virtual int  GetCornerAnnotationVisibility();
  virtual void ToggleCornerAnnotationVisibility();
  vtkBooleanMacro(CornerAnnotationVisibility, int);
  virtual void SetCornerAnnotationColor(double r, double g, double b);
  virtual void SetCornerAnnotationColor(double *rgb)
    { this->SetCornerAnnotationColor(rgb[0], rgb[1], rgb[2]); };
  virtual double* GetCornerAnnotationColor();
  vtkGetObjectMacro(CornerAnnotation, vtkCornerAnnotation);

  // Description:
  // Get and control the header annotation.
  virtual void SetHeaderAnnotationVisibility(int v);
  virtual int  GetHeaderAnnotationVisibility();
  virtual void ToggleHeaderAnnotationVisibility();
  vtkBooleanMacro(HeaderAnnotationVisibility, int);
  virtual void SetHeaderAnnotationColor(double r, double g, double b);
  virtual void SetHeaderAnnotationColor(double *rgb)
    { this->SetHeaderAnnotationColor(rgb[0], rgb[1], rgb[2]); };
  virtual double* GetHeaderAnnotationColor();
  virtual void SetHeaderAnnotationText(const char*);
  virtual char* GetHeaderAnnotationText();
  vtkGetObjectMacro(HeaderAnnotation, vtkTextActor);
  
  // Description:
  // Set/Get the distance units that pixel sizes are measured in
  virtual void SetDistanceUnits(const char*);
  vtkGetStringMacro(DistanceUnits);
  
  // Description:
  // Get the render window
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  // Get the VTK widget
  vtkGetObjectMacro(VTKWidget, vtkKWCoreWidget);
  
  // Description:
  // If the widget supports multiple renderers:
  // GetNthRenderer() gets the Nth renderer (or NULL if it does not exist),
  // GetRendererIndex() gets the id of a given renderer (or -1 if this renderer
  // does not belong to this widget), i.e. its index/position in the list
  // of renderers. AddRenderer() will add a renderer, RemoveAllRenderers will
  // remove all renderers.
  // Note that no default renderers exist before Create() is called.
  virtual vtkRenderer* GetRenderer() { return this->GetNthRenderer(0); }
  virtual vtkRenderer* GetNthRenderer(int index);
  virtual int GetNumberOfRenderers();
  virtual int GetRendererIndex(vtkRenderer*);
  virtual void AddRenderer(vtkRenderer*);
  virtual void RemoveRenderer(vtkRenderer*);
  virtual void RemoveNthRenderer(int index);
  virtual void RemoveAllRenderers();

  // Description:
  // Get the overlay renderer.
  // Note that no default overlay renderer exist before Create() is called.
  virtual vtkRenderer* GetOverlayRenderer() 
    { return this->GetNthOverlayRenderer(0); }
  virtual vtkRenderer* GetNthOverlayRenderer(int index);
  virtual int GetNumberOfOverlayRenderers();
  virtual int GetOverlayRendererIndex(vtkRenderer*);
  virtual void AddOverlayRenderer(vtkRenderer*);
  virtual void RemoveOverlayRenderer(vtkRenderer*);
  virtual void RemoveNthOverlayRenderer(int index);
  virtual void RemoveAllOverlayRenderers();

  // Description:
  // Set the background color of the renderer(s) (not the overlay ones).
  // Note that no default renderers exist before Create() is called.
  virtual void GetRendererBackgroundColor(double *r, double *g, double *b);
  virtual void SetRendererBackgroundColor(double r, double g, double b);
  virtual void SetRendererBackgroundColor(double rgb[3])
    { this->SetRendererBackgroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Convenience method to add props (actors) to *all* widget renderer(s)
  // or *all*  overlay renderer(s), or to specific ones.
  // Note that no default renderers exist before Create() is called.
  virtual void AddViewProp(vtkProp *prop);
  virtual void AddViewPropToNthRenderer(vtkProp *p, int index);
  virtual void AddOverlayViewProp(vtkProp *prop);
  virtual void AddViewPropToNthOverlayRenderer(vtkProp *p, int index);

  // Description:
  // Query/remove props (actors) from both renderer(s) and overlay renderer(s).
  virtual int  HasViewProp(vtkProp *prop);
  virtual void RemoveViewProp(vtkProp *prop);
  virtual void RemoveAllViewProps();

  // Description:
  // The ComputeVisiblePropBounds() method returns the bounds of the 
  // visible props for a renderer (given its index). By default, it is just
  // a call to vtkRenderer::ComputeVisiblePropBounds().
  virtual void ComputeVisiblePropBounds(int index, double bounds[6]);

  // Description:
  // Set/Get the printing flag (i.e., are we printing?)
  virtual void SetPrinting(int arg);
  vtkBooleanMacro(Printing, int);
  vtkGetMacro(Printing, int);
  
  // Description:
  // Set/Get offscreen rendering flag (e.g., for screenshots)
  vtkBooleanMacro(OffScreenRendering, int);
  virtual void SetOffScreenRendering(int);
  virtual int GetOffScreenRendering();
  
  // Description:
  // Use a context menu. It is posted by a right click, and allows
  // properties and mode to be controlled.
  vtkSetMacro(UseContextMenu, int);
  vtkGetMacro(UseContextMenu, int);
  vtkBooleanMacro(UseContextMenu, int);
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Overridden for debugging purposes. This class is usually the center of the
  // whole "a vtkTkRenderWidget is being destroyed before its render window"
  // problem.
  virtual void Register(vtkObjectBase* o);
  virtual void UnRegister(vtkObjectBase* o);
  
  // Description:
  // Setup print parameters
#if defined(_WIN32) && !defined(__CYGWIN__)
  virtual void SetupPrint(RECT &rcDest, HDC ghdc,
                          int printerPageSizeX, int printerPageSizeY,
                          int printerDPIX, int printerDPIY,
                          float scaleX, float scaleY,
                          int screenSizeX, int screenSizeY);
#endif

  // Description:
  // Get memory device context (when rendering to memory)
  virtual void* GetMemoryDC();

  // Description:
  // Add all the default observers needed by that object, or remove
  // all the observers that were added through AddCallbackCommandObserver.
  // Subclasses can override these methods to add/remove their own default
  // observers, but should call the superclass too.
  virtual void AddCallbackCommandObservers();
  virtual void RemoveCallbackCommandObservers();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void MouseMoveCallback(
    int num, int x, int y, int ctrl, int shift);
  virtual void MouseWheelCallback(
    int delta, int ctrl, int shift);
  virtual void MouseButtonPressCallback(
    int num, int x, int y, int ctrl, int shift, int repeat);
  virtual void MouseButtonReleaseCallback(
    int num, int x, int y, int ctrl, int shift);
  virtual void KeyPressCallback(
    char key, int x, int y, int ctrl, int shift, char *keysym);
  virtual void ConfigureCallback(int width, int height);
  virtual void ExposeCallback();
  virtual void EnterCallback(int /*x*/, int /*y*/) {};
  virtual void FocusInCallback();
  virtual void FocusOutCallback();

protected:
  vtkKWRenderWidget();
  ~vtkKWRenderWidget();
  
  vtkKWCoreWidget                    *VTKWidget;
  vtkRenderWindow                    *RenderWindow;
  vtkKWGenericRenderWindowInteractor *Interactor;
  vtkCornerAnnotation                *CornerAnnotation;
  vtkTextActor                       *HeaderAnnotation;
  
  int RenderMode;
  int PreviousRenderMode;
  int InExpose;
  int RenderState;
  int Printing;

  Tcl_TimerToken InteractorTimerToken;
  
  char *DistanceUnits;

  int CollapsingRenders;
  int CollapsingRendersCount;

  // Description:
  // Create the default renderers inside the render window.
  // Superclass can override to create different renderers.
  // It is called by Create().
  virtual void CreateDefaultRenderers();

  // Description:
  // Install the renderers inside the render window.
  // Superclass can override to install them in a different layout.
  // It is called by Create().
  virtual void InstallRenderers();

  // Description:
  // Update the widget according to the units.
  // Should be called when any units-related ivar has changed.
  virtual void UpdateAccordingToUnits() {};

  // Description:
  // Setup memory rendering
  virtual void SetupMemoryRendering(int width, int height, void *cd);
  virtual void ResumeScreenRendering();
  
  // Description:
  // Processes the events that are passed through CallbackCommand (or others).
  // Subclasses can oberride this method to process their own events, but
  // should call the superclass too.
  virtual void ProcessCallbackCommandEvents(
    vtkObject *caller, unsigned long event, void *calldata);
  
  // Context menu

  int UseContextMenu;
  vtkKWMenu *ContextMenu;

  // Description:
  // Populate the context menu
  // Superclass should override this method to populate *and* update this
  // menu with the commands they feel confortable exposing to the user.
  // This implementation calls PopulateAnnotationMenu() to add all
  // annotation relevant entries.
  virtual void PopulateContextMenu(vtkKWMenu*);
  virtual void PopulateAnnotationMenu(vtkKWMenu*);

  // PIMPL Encapsulation for STL containers

  vtkKWRenderWidgetInternals *Internals;

private:
  vtkKWRenderWidget(const vtkKWRenderWidget&);  // Not implemented
  void operator=(const vtkKWRenderWidget&);  // Not implemented
};

#endif

