/*=========================================================================

  Module:    vtkKWRenderWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWRenderWidget
// .SECTION Description

#ifndef __vtkKWRenderWidget_h
#define __vtkKWRenderWidget_h

#include "vtkKWCompositeWidget.h"
#include "vtkWindows.h" // needed for RECT HDC

class vtkCamera;
class vtkCornerAnnotation;
class vtkKWGenericRenderWindowInteractor;
class vtkCallbackCommand;
class vtkKWWindow;
class vtkProp;
class vtkRenderWindow;
class vtkRenderer;
class vtkTextActor;
class vtkTextMapper;

class KWWIDGETS_EXPORT vtkKWRenderWidget : public vtkKWCompositeWidget
{
public:
  static vtkKWRenderWidget* New();
  vtkTypeRevisionMacro(vtkKWRenderWidget, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Render the scene.
  virtual void Render();

  // Description:
  // Enable/disable rendering.
  vtkGetMacro(RenderState, int);
  vtkSetClampMacro(RenderState, int, 0, 1);
  vtkBooleanMacro(RenderState, int);
  
  // Description:
  // Set/get the rendering mode
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
  virtual const char *GetRenderModeAsString();

  // Description:
  // Reset the view/widget (will usually reset the camera too).
  virtual void Reset();
  virtual void ResetCamera();
  
  // Description:
  // Close the widget. 
  // Usually called when new data is about to be loaded.
  virtual void Close();
  
  // Description:
  // Add/remove the widget bindings.
  // AddBindings(), which sets up general bindings like Expose or Configure
  // events, will ultimately call AddInteractionBindings() which sets up
  // interaction bindings (mouse events, keyboard events, etc.).
  virtual void AddBindings();
  virtual void RemoveBindings();
  virtual void AddInteractionBindings();
  virtual void RemoveInteractionBindings();
  
  // Description:
  // Manage props inside this widget renderer(s). Add, remove, query.
  virtual void AddProp(vtkProp *prop);
  virtual void AddOverlayProp(vtkProp *prop);
  virtual int  HasProp(vtkProp *prop);
  virtual void RemoveAllProps();

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
  // Avoid windows name mangling.
# define RemovePropA RemoveProp
# define RemovePropW RemoveProp
#endif

  // Description:
  // Manage props inside this widget renderer(s). Add, remove, query.
  void RemoveProp(vtkProp *prop);

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef RemovePropA
# undef RemovePropW
  //BTX
  // Define possible mangled names.
  void RemovePropA(vtkProp*);
  void RemovePropW(vtkProp*);
  //ETX
#endif

  // Description:
  // Set the widget background color
  virtual void GetRendererBackgroundColor(double *r, double *g, double *b);
  virtual void SetRendererBackgroundColor(double r, double g, double b);
  virtual void SetRendererBackgroundColor(double rgb[3])
    { this->SetRendererBackgroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Event handlers and useful interactions
  virtual void MouseMove(int num, int x, int y);
  virtual void AButtonPress(int num, int x, int y, int ctrl, int shift);
  virtual void AButtonRelease(int num, int x, int y);
  virtual void AKeyPress(char key, int x, int y, int ctrl, int shift, char *keysym);
  virtual void Configure(int width, int height);
  virtual void Exposed();
  virtual void Enter(int /*x*/, int /*y*/) {};
  virtual void FocusInCallback();
  virtual void FocusOutCallback();

  // Description:
  // Convenience method to set the visibility of all annotations.
  virtual void SetAnnotationsVisibility(int v);
  vtkBooleanMacro(AnnotationsVisibility, int);

  // Description:
  // Get and control the corner annotation.
  virtual void SetCornerAnnotationVisibility(int v);
  virtual int  GetCornerAnnotationVisibility();
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
  // Get the current camera
  vtkCamera *GetCurrentCamera();

  // Description:
  // Set / Get the collapsing of renders. If this is set to true, then
  // all renders will be collapsed. Once this is set to false, if
  // there are any pending render requests. The widget will render.
  virtual void SetCollapsingRenders(int);
  vtkBooleanMacro(CollapsingRenders, int);
  vtkGetMacro(CollapsingRenders, int);

  // Description:
  // This id is a hint that should be appended to the call data and
  // should help identify who sent the event. Of course, the client data
  // is a pointer to who sent the event, but in some situation it is either
  // not available nor very usable. Default to -1.
  // Can be safely ignored in most cases.
  vtkSetMacro(EventIdentifier, int);
  vtkGetMacro(EventIdentifier, int);

  // Description:
  // Get the render window
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  // If the widget supports multiple renderers (excluding overlay renderers):
  // GetNthRenderer() gets the Nth renderer (or NULL if it does not exist),
  // GetRendererId() gets the id of a given renderer (or -1 if this renderer
  // does not belong to this widget)
  virtual vtkRenderer* GetRenderer() { return this->GetNthRenderer(0); }
  virtual vtkRenderer* GetNthRenderer(int id);
  virtual int GetRendererId(vtkRenderer*);
  virtual vtkRenderer* GetOverlayRenderer();

  // Description:
  // Get the VTK widget
  vtkGetObjectMacro(VTKWidget, vtkKWCoreWidget);
  
  // Description:
  // Are we printing ?
  virtual void SetPrinting(int arg);
  vtkBooleanMacro(Printing, int);
  vtkGetMacro(Printing, int);
  
#ifdef _WIN32
  void SetupPrint(RECT &rcDest, HDC ghdc,
                  int printerPageSizeX, int printerPageSizeY,
                  int printerDPIX, int printerDPIY,
                  float scaleX, float scaleY,
                  int screenSizeX, int screenSizeY);
#endif

  // Description:
  // Get memory device context (when rendering to memory)
  virtual void* GetMemoryDC();

  // Description:
  // Setup offscreen rendering (e.g., for screenshots)
  vtkBooleanMacro(OffScreenRendering, int);
  virtual void SetOffScreenRendering(int);
  virtual int GetOffScreenRendering();
  
  // Description:
  // Method that processes the events
  virtual void ProcessEvent(
    vtkObject *caller, unsigned long event, void *calldata);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // For debugging purposes. This class is usually the center of the
  // whole "a vtkTkRenderWidget is being destroyed before its render window"
  // problem.
  virtual void Register(vtkObjectBase* o);
  virtual void UnRegister(vtkObjectBase* o);
  
protected:
  vtkKWRenderWidget();
  ~vtkKWRenderWidget();
  
  vtkKWCoreWidget *VTKWidget;

  vtkRenderWindow *RenderWindow;
  vtkKWGenericRenderWindowInteractor *Interactor;

  vtkCornerAnnotation *CornerAnnotation;
  vtkTextActor        *HeaderAnnotation;
  
  int RenderMode;
  int PreviousRenderMode;
  int InExpose;
  int RenderState;
  int Printing;
  Tcl_TimerToken InteractorTimerToken;
  
  char *DistanceUnits;

  int CollapsingRenders;
  int CollapsingRendersCount;

  virtual void UpdateAccordingToUnits() {};

  // Description:
  // Setup memory rendering
  virtual void SetupMemoryRendering(int width, int height, void *cd);
  virtual void ResumeScreenRendering();
  
  // Description:
  // Add/remove the observers.
  virtual void AddObservers();
  virtual void RemoveObservers();
  vtkCallbackCommand *Observer;
  int EventIdentifier;
  
  virtual void RemovePropInternal(vtkProp* prop);

  // Description:
  // Callback commmand. Pass the parameters to the virtual ProcessEvent method
  static void ProcessEventFunction(
    vtkObject *object, unsigned long event, void *clientdata, void *calldata);

private:
  vtkKWRenderWidget(const vtkKWRenderWidget&);  // Not implemented
  void operator=(const vtkKWRenderWidget&);  // Not implemented

  // Put those two in the private section to force subclasses to use
  // accessors. This will solve case when user-defined renderer 
  // are setup to replace the default one.

  vtkRenderer     *Renderer;
  vtkRenderer     *OverlayRenderer;
};

#endif

