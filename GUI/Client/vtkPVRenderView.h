/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderView - For using styles
// .SECTION Description
// This is a render view that is a parallel object.  It needs to be cloned
// in all the processes to work correctly.  After cloning, the parallel
// nature of the object is transparent.
// Other features:
// I am going to try to divert the events to a vtkInteractorStyle object.
// I also have put compositing into this object.  I had to create a separate
// renderwindow and renderer for the off screen compositing (Hacks).
// Eventually I need to merge these back into the main renderer and renderer
// window.


#ifndef __vtkPVRenderView_h
#define __vtkPVRenderView_h

#include "vtkKWView.h"

class vtkInteractorObserver;
class vtkKWChangeColorButton;
class vtkKWLabel;
class vtkKWPushButton;
class vtkKWRadioButton;
class vtkKWScale;
class vtkPVSourceNotebook;
class vtkKWSplitFrame;
class vtkLabeledFrame;
class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVAxesWidget;
class vtkPVCameraIcon;
class vtkPVCameraControl;
class vtkPVCornerAnnotation;
class vtkPVData;
class vtkPVInteractorStyleControl;
class vtkPVSource;
class vtkPVSourceList;
class vtkPVSourcesNavigationWindow;
class vtkPVTreeComposite;
class vtkPVWindow;
class vtkPVRenderModuleUI;
class vtkPVRenderViewObserver;
class vtkPVRenderModule;
class vtkTimerLog;

#define VTK_PV_VIEW_MENU_LABEL       " 3D View Properties"

class VTK_EXPORT vtkPVRenderView : public vtkKWView
{
public:
  static vtkPVRenderView* New();
  vtkTypeRevisionMacro(vtkPVRenderView,vtkKWView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // I wanted to get rid of these methods, but they are used
  // for corner annotation to work.
  void AddAnnotationProp(vtkPVCornerAnnotation *c);
  void RemoveAnnotationProp(vtkPVCornerAnnotation *c);

  // Description:
  // Set the application right after construction.
  void CreateRenderObjects(vtkPVApplication *pvApp);
  
  // Description:
  // Add widgets to vtkKWView's notebook
  virtual void CreateViewProperties();
  
  // Description:
  // Create the TK widgets associated with the view.
  virtual void Create(vtkKWApplication *app, const char *args);
  
  // Description:
  // Method called by the toolbar reset camera button.
  void ResetCamera();
  
  // Description:
  // Specify the position, focal point, and view up of the camera.
  void SetCameraState(float p0, float p1, float p2,
                      float fp0, float fp1, float fp2,
                      float up0, float up1, float up2);
  
  // Description:
  // Set the parallel scale of the camera.
  void SetCameraParallelScale(float scale);

  // Description:
  // Callback to set the view up or down one of the three axes.
  void StandardViewCallback(float x, float y, float z);

  // Description:
  // Make snapshot of the render window.
  virtual void SaveAsImage() { this->Superclass::SaveAsImage(); }
  virtual void SaveAsImage(const char* filename) ;
  
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  // Description:
  // Composites
  void Render();
  void ForceRender();
  void EventuallyRender();
  void EventuallyRenderCallBack();

  // Description:
  // Tcl "update" has to be called for various reasons (packing).
  // This calls update without triggering EventuallyRenderCallback.
  // I was having problems with multiple renders.
  void UpdateTclButAvoidRendering();
  
  // Description:
  // Callback method bound to expose or configure events.
  void Exposed();
  void Configured();
  
  // Description:
  // Update the navigation window for a particular source
  void UpdateNavigationWindow(vtkPVSource *currentSource, int nobind);

  // Description:
  // Get the frame for the navigation window
  vtkGetObjectMacro(NavigationFrame, vtkKWLabeledFrame);

  // Description:
  // Show either navigation window with a fragment of pipeline or a
  // source window with a list of sources. If the argument registery
  // is 1, then the value will be stored in the registery.
  void ShowNavigationWindowCallback(int registery);
  void ShowSelectionWindowCallback(int registery);
  
  // Description:
  // Export the renderer and render window to a file.
  void SaveInBatchScript(ofstream *file);
  void SaveState(ofstream *file);

  // Description:
  // Change the background color.
  void SetRendererBackgroundColor(double r, double g, double b);

  // Description:
  // Close the view - called from the vtkkwwindow. This default method
  // will simply call Close() for all the composites. Can be overridden.
  virtual void Close();

  // Description:
  // This method Sets all IVars to NULL and unregisters
  // vtk objects.  This should eliminate circular references.
  void PrepareForDelete();
  
  // Description:
  // Get the parallel projection check button
  vtkGetObjectMacro(ParallelProjectionCheck, vtkKWCheckButton);

  // Description:
  // Callback for toggling between parallel and perspective.
  void ParallelProjectionCallback();
  void ParallelProjectionOn();
  void ParallelProjectionOff();
  
  // Description:
  // Callback for the triangle strips check button
  void TriangleStripsCallback();
  void SetUseTriangleStrips(int state);
  
  // Description:
  // Callback for the immediate mode rendering check button
  void ImmediateModeCallback();
  void SetUseImmediateMode(int state);
  
  // Description:
  // Get the triangle strips check button.
  vtkGetObjectMacro(TriangleStripsCheck, vtkKWCheckButton);
  
  // Description:
  // Get the immediate mode rendering check button.
  vtkGetObjectMacro(ImmediateModeCheck, vtkKWCheckButton);

  // Description:
  // A convience method to get the PVWindow.
  vtkPVWindow *GetPVWindow();

  // Description:
  // Get the widget that controls the interactor styles.
  vtkGetObjectMacro(ManipulatorControl3D, vtkPVInteractorStyleControl);
  vtkGetObjectMacro(ManipulatorControl2D, vtkPVInteractorStyleControl);

  // Description:
  // Get the widget that controls the camera.
  vtkGetObjectMacro(CameraControl, vtkPVCameraControl);
  
  // Description:
  // Get the size of the render window.
  int* GetRenderWindowSize();

  // Description:
  // Setup camera manipulators by populating the control and setting
  // initial values.
  void SetupCameraManipulators();

  // Description:
  // Update manipulators after they were added to control.
  void UpdateCameraManipulators();

  // Description:
  // Here so that sources get packed in the second frame.
  virtual vtkKWWidget *GetSourceParent();
  vtkKWSplitFrame *GetSplitFrame() {return this->SplitFrame;}

  // Description:
  // Sources now share a single notebook and information GUI.
  // This object does not seen like the best place to create
  // this notebook and GUI. But it is here now for legacy reasons.
  // Get SourceParent may not be needed anymore.
  vtkGetObjectMacro(SourceNotebook,vtkPVSourceNotebook);

  // Description:
  // This method is called when an event is called that PVRenderView
  // is interested in.
  void ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata);

  // Description:
  // Store current camera at a specified position. This stores all the
  // camera parameters and generates a small icon.
  void StoreCurrentCamera(int position);
  
  // Description:
  // Restore current camera from a specified position.
  void RestoreCurrentCamera(int position);  

  // Description:
  // Set the global variable for always displaying 3D widgets.
  void Display3DWidgetsCallback();
  void SetDisplay3DWidgets(int s);

  // Description:
  // Show the names in sources browser.
  void SetSourcesBrowserAlwaysShowName(int s);

  // Description:
  // Switch to the View Properties menu back and forth
  void SwitchBackAndForthToViewProperties();

  //BTX
  // Description:
  // Access to VTK renderer and render window.
  vtkRenderWindow *GetRenderWindow();
  vtkRenderer *GetRenderer();
  //ETX
  
  // Description:
  // This method resizes the render window size but the actuall window stays
  // the same
  void SetRenderWindowSize(int x, int y);

  // Description:
  // Enable the input 3D widget
  void Enable3DWidget(vtkInteractorObserver *o);

  // Description:
  // Callbacks for the orientation axes.
  void SetOrientationAxesVisibility(int val);
  void OrientationAxesCheckCallback();
  void SetOrientationAxesInteractivity(int val);
  void OrientationAxesInteractiveCallback();
  void SetOrientationAxesOutlineColor(double r, double g, double b);
  void SetOrientationAxesTextColor(double r, double g, double b);

  // Description:
  // Returns the UI created by the render module
  vtkGetObjectMacro(RenderModuleUI, vtkPVRenderModuleUI);

  // Description:
  // Handle the edit copy menu option.
  virtual void EditCopy();

  // Description:
  // Print the image. This may pop up a dialog box, etc.
  virtual void PrintView();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Block render requests. If the render requests come, they will be blocked
  // and when unblocked render will be called.
  void StartBlockingRender();
  void EndBlockingRender();
 
  // Description:
  // Access to these widgets from a script.
  vtkGetObjectMacro(StandardViewsFrame, vtkKWLabeledFrame);
  vtkGetObjectMacro(CameraIconsFrame, vtkKWLabeledFrame);
  vtkGetObjectMacro(CameraControlFrame, vtkKWLabeledFrame);
  vtkGetObjectMacro(OrientationAxesFrame, vtkKWLabeledFrame);
  //BTX
  vtkGetObjectMacro(OrientationAxes, vtkPVAxesWidget);
  //ETX

protected:
  vtkPVRenderView();
  ~vtkPVRenderView();

  // Access to the overlay renderer.
  vtkRenderer *GetRenderer2D();

  vtkPVSourceNotebook* SourceNotebook;

  void CalculateBBox(char* name, int bbox[4]);
 
  vtkKWLabeledFrame *StandardViewsFrame;
  vtkKWPushButton   *XMaxViewButton; 
  vtkKWPushButton   *XMinViewButton; 
  vtkKWPushButton   *YMaxViewButton; 
  vtkKWPushButton   *YMinViewButton; 
  vtkKWPushButton   *ZMaxViewButton; 
  vtkKWPushButton   *ZMinViewButton; 

  vtkKWLabeledFrame *RenderParametersFrame;
  vtkKWCheckButton *TriangleStripsCheck;
  vtkKWCheckButton *ParallelProjectionCheck;
  vtkKWCheckButton *ImmediateModeCheck;

  vtkPVRenderModuleUI* RenderModuleUI;

  vtkKWLabeledFrame *InterfaceSettingsFrame;
  vtkKWCheckButton *Display3DWidgets;

  vtkKWLabeledFrame *OrientationAxesFrame;
  vtkKWCheckButton *OrientationAxesCheck;
  vtkKWCheckButton *OrientationAxesInteractiveCheck;
  vtkKWChangeColorButton *OrientationAxesOutlineColor;
  vtkKWChangeColorButton *OrientationAxesTextColor;
  vtkPVAxesWidget *OrientationAxes;

  vtkKWSplitFrame *SplitFrame;

  vtkKWLabeledFrame* NavigationFrame;
  vtkPVSourcesNavigationWindow* NavigationWindow;
  vtkPVSourcesNavigationWindow* SelectionWindow;
  vtkKWRadioButton *NavigationWindowButton;
  vtkKWRadioButton *SelectionWindowButton;
  
  int BlockRender;

  int ShowSelectionWindow;
  int ShowNavigationWindow;

  // For the renderer in a separate toplevel window.
  vtkKWWidget *TopLevelRenderWindow;

  vtkPVInteractorStyleControl *ManipulatorControl2D;
  vtkPVInteractorStyleControl *ManipulatorControl3D;

  // Camera icons
  vtkKWLabeledFrame* CameraIconsFrame;
  vtkPVCameraIcon* CameraIcons[6];
  
  // Camera controls (elevation, azimuth, roll)
  vtkKWLabeledFrame *CameraControlFrame;
  vtkPVCameraControl *CameraControl;

  vtkKWPushButton *PropertiesButton;

  char *MenuLabelSwitchBackAndForthToViewProperties;
  vtkSetStringMacro(MenuLabelSwitchBackAndForthToViewProperties);
  
  vtkPVRenderViewObserver* Observer;

  // Need to make sure it destructs right before this view does.
  // It's the whole TKRenderWidget destruction pain.
  vtkPVRenderModule* RenderModule;

  vtkTimerLog *RenderTimer;
  Tcl_TimerToken TimerToken;
  
private:
  vtkPVRenderView(const vtkPVRenderView&); // Not implemented
  void operator=(const vtkPVRenderView&); // Not implemented
};


#endif
