/*=========================================================================

  Module:    vtkKWView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWView - a view superclass
// .SECTION Description
// The view class normally sits within a vtkKWWindow and holds a
// vtkWindow. Normally you will not create this class but instead use
// the concrete subclasses such as vtkKWImageView and vtkKWRenderView.


#ifndef __vtkKWView_h
#define __vtkKWView_h

#define VTK_KW_INTERACTIVE_RENDER  0
#define VTK_KW_STILL_RENDER        1
#define VTK_KW_DISABLED_RENDER     2
#define VTK_KW_SINGLE_RENDER       3

#define VTK_KW_VIEW_MENU_INDEX     10

#include "vtkKWWidget.h"
#include "vtkWindows.h" // needed for RECT HDC

class vtkKWApplication;
class vtkKWChangeColorButton;
class vtkKWCheckButton;
class vtkPVCornerAnnotation;
class vtkKWEntry;
class vtkKWFrame;
class vtkKWLabeledFrame;
class vtkKWLabel;
class vtkKWMenu;
class vtkKWMenuButton;
class vtkKWNotebook;
class vtkKWSegmentedProgressGauge;
class vtkKWText;
class vtkKWWindow;
class vtkKWWindow;
class vtkRenderWindow;
class vtkRenderer;
class vtkTextActor;
class vtkTextMapper;
class vtkViewport;
class vtkViewport;
class vtkWindow;

class VTK_EXPORT vtkKWView : public vtkKWWidget
{
  public:
  vtkTypeRevisionMacro(vtkKWView,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a View
  virtual void Create(vtkKWApplication* vtkNotUsed(app), 
                      const char* vtkNotUsed(args)) {}

  // Description:
  // Sets up some default event handlers to allow
  // users to manipulate the view etc.
  virtual void SetupBindings();

  // Description:
  // Used to queue up expose event prior to processing.
  vtkSetMacro(InExpose,int);
  vtkGetMacro(InExpose,int);

  // Description:
  // These are the event handlers that UIs can use or override.
  virtual void AButtonPress(int vtkNotUsed(num), int vtkNotUsed(x), 
                            int vtkNotUsed(y)) {}
  virtual void AButtonRelease(int vtkNotUsed(num), int vtkNotUsed(x), 
                              int vtkNotUsed(y)) {}
  virtual void AShiftButtonPress(int vtkNotUsed(num), int vtkNotUsed(x),
                                 int vtkNotUsed(y)) {}
  virtual void AShiftButtonRelease(int vtkNotUsed(num), int vtkNotUsed(x), 
                                   int vtkNotUsed(y)) {}
  virtual void AControlButtonPress(int vtkNotUsed(num), int vtkNotUsed(x),
                                   int vtkNotUsed(y)) {}
  virtual void AControlButtonRelease(int vtkNotUsed(num), int vtkNotUsed(x), 
                                     int vtkNotUsed(y)) {}
  virtual void AKeyPress(char vtkNotUsed(key), int vtkNotUsed(x), 
                         int vtkNotUsed(y)) {}
  virtual void Button1Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {}
  virtual void Button2Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {}
  virtual void Button3Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {}
  virtual void ShiftButton1Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {}
  virtual void ShiftButton2Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {}
  virtual void ShiftButton3Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {}
  virtual void ControlButton1Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {}
  virtual void ControlButton2Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {}
  virtual void ControlButton3Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {}
  virtual void Exposed() {}
  virtual void Enter(int x, int y);

  // Description:
  // Handle the edit copy menu option.
  virtual void EditCopy();

  // Description:
  // Handle the file save as image menu option.
  virtual void SaveAsImage();
  virtual void SaveAsImage(const char* filename);

  // Description
  // Printthe image. This may pop up a dialog box etc.
  virtual void PrintView();
#ifdef _WIN32
  void SetupPrint(RECT &rcDest, HDC ghdc,
                  int printerPageSizeX, int printerPageSizeY,
                  int printerDPIX, int printerDPIY,
                  float scaleX, float scaleY,
                  int screenSizeX, int screenSizeY);
#endif
  int GetPrinting() {return this->Printing;};
  vtkSetMacro(Printing,int);
  
  // Description;
  // Set the parent window if used so that additional
  // features may be enabled. The parent window is the vtkKWWindow
  // that contains the view.
  vtkGetObjectMacro(ParentWindow,vtkKWWindow);
  void SetParentWindow(vtkKWWindow *);

  //BTX
  // Description:
  // Return the RenderWindow or ImageWindow as appropriate.
  virtual vtkWindow *GetVTKWindow();

  // Description:
  // Return the Renderer or Imager as appropriate.
  virtual vtkViewport *GetViewport();
  //ETX

  // Description:
  // Methods to support off screen rendering.
  virtual void SetupMemoryRendering(int width,int height, void *cd);
  virtual void ResumeScreenRendering();
  virtual unsigned char *GetMemoryData();
  virtual void *GetMemoryDC();
  
  // Description:
  // Get the attachment point for the Composits properties.
  // This attachment point may be obtained from the parent window
  // if it has been set.
  virtual vtkKWWidget *GetPropertiesParent();
  void SetPropertiesParent(vtkKWWidget*);

  // Description:
  // Make the properties show up in the view instead of the window
  // or as a top level dialog.
  void CreateDefaultPropertiesParent();

  // Description:
  // Make sure that the Views property parent is currently packed
  virtual void PackProperties();
  
  // Description:
  // Render the image.
  virtual void Render();
  virtual void Reset() {};
        
  // Description:
  // Return the tk widget used for the vtkWindow. This widget
  // is a child of the widget representing the view.
  vtkKWWidget *GetVTKWidget() {return this->VTKWidget;};
  
  // Description:
  // Methods to indicate when this view is the selected view.
  virtual void Select(vtkKWWindow *);
  virtual void Deselect(vtkKWWindow *);
  virtual void MakeSelected();
  
  // Description:
  // Displays and/or updates the property ui display
  virtual void ShowViewProperties();

  // Description::
  // Override Unregister since widgets have loops.
  virtual void UnRegister(vtkObjectBase *o);

  // Description::
  // Set the title of this view.
  virtual void SetTitle(const char *);

  // Description:
  // Allow access to the notebook object.
  vtkGetObjectMacro(Notebook,vtkKWNotebook);

  // Description:
  // Close the view - called from the vtkkwwindow. This default method
  // will simply call Close() for all the composites. Can be overridden.
  virtual void Close();

  // Description:
  // Create the properties sheet, called by ShowViewProperties.
  virtual void CreateViewProperties();

  // Description:
  // Allow access to the UI components of interest
  vtkGetObjectMacro(CornerAnnotation,vtkPVCornerAnnotation);

  vtkSetMacro( InteractiveUpdateRate, float );
  vtkGetMacro( InteractiveUpdateRate, float );

  void SetStillUpdateRates( int count, float *rates );
  vtkGetMacro( NumberOfStillUpdates, int );
  float *GetStillUpdateRates() { return this->StillUpdateRates; };
  float GetStillUpdateRate(int i) { return this->StillUpdateRates[i]; };

  vtkSetClampMacro( RenderMode, int, 
                    VTK_KW_INTERACTIVE_RENDER,
                    VTK_KW_DISABLED_RENDER );
  vtkGetMacro( RenderMode, int );
  void SetRenderModeToInteractive() 
    { this->RenderMode = VTK_KW_INTERACTIVE_RENDER; };
  void SetRenderModeToStill() 
    { this->RenderMode = VTK_KW_STILL_RENDER; };
  void SetRenderModeToSingle() 
    { this->RenderMode = VTK_KW_SINGLE_RENDER; };
  void SetRenderModeToDisabled() 
    { this->RenderMode = VTK_KW_DISABLED_RENDER; };

  // In addition to the render mode, we have the render state - which
  // can be on or off. This allows a window to disable all its views 
  // while updating the GUI
  vtkGetMacro( RenderState, int );
  vtkSetClampMacro( RenderState, int, 0, 1 );
  vtkBooleanMacro( RenderState, int );
  
  // Description:
  // Change the color of the corner annotation text
  virtual void SetCornerTextColor( double rgb[3] );
  virtual double *GetCornerTextColor();

  // Description:
  // Turn interactivity on / off - used for UI components that want 
  // interactive rendering while values are being modified.
  void InteractOn();
  void InteractOff();

  // Description:
  // Set the background color
  virtual void SetRendererBackgroundColor(double r, double g, double b);
  virtual void GetRendererBackgroundColor(double *r, double *g, double *b);
  
  // Description:
  // Set the name to be used in the menu for the view properties
  // sheet entry
  vtkSetStringMacro(MenuEntryName);
  vtkGetStringMacro(MenuEntryName);
  vtkSetStringMacro(MenuEntryHelp);
  vtkGetStringMacro(MenuEntryHelp);
  vtkSetMacro(MenuEntryUnderline, int);
  vtkGetMacro(MenuEntryUnderline, int);

  // Description:
  // Options to enable / disable UI elements. Should be set before the
  // UI is created.
  vtkSetMacro( SupportSaveAsImage, int );
  vtkGetMacro( SupportSaveAsImage, int );
  vtkBooleanMacro( SupportSaveAsImage, int );
  
  vtkSetMacro( SupportPrint, int );
  vtkGetMacro( SupportPrint, int );
  vtkBooleanMacro( SupportPrint, int );

  vtkSetMacro( SupportCopy, int );
  vtkGetMacro( SupportCopy, int );
  vtkBooleanMacro( SupportCopy, int );

  vtkSetMacro( SupportControlFrame, int );
  vtkGetMacro( SupportControlFrame, int );
  vtkBooleanMacro( SupportControlFrame, int );

  // Description:
  // Get the control frame to put custom controls within
  vtkGetObjectMacro( ControlFrame, vtkKWWidget );
  
  //BTX
  // Description:
  // This class now longer owns these objects.
  // I am providing these methods as temporary access to these objects.
  // I plan to make these private.
  virtual vtkRenderWindow* GetRenderWindow() { return NULL; }
  virtual vtkRenderer* GetRenderer() { return NULL; }
  virtual vtkRenderer* GetRenderer2D() { return NULL; }
  //ETX
    
  // Description:
  // The guts of the abort check method. Made public so that it can
  // be accessed by the render timer callback.
  int ShouldIAbort();

  // Description:
  // Should I display the progress gauge in the title bar?  By default,
  // it is off.
  vtkSetMacro(UseProgressGauge, int);
  vtkGetMacro(UseProgressGauge, int);
  vtkBooleanMacro(UseProgressGauge, int);

  vtkGetObjectMacro(ProgressGauge, vtkKWSegmentedProgressGauge);

  // Description:
  // Check if the application needs to abort.
  virtual int CheckForOtherAbort() { return 0; }

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkKWView();
  ~vtkKWView();

  vtkPVCornerAnnotation *CornerAnnotation;
  
  vtkKWNotebook *Notebook;
  int InExpose;
  int SharedPropertiesParent;
  float Center[2];
  vtkKWWindow *ParentWindow;
  vtkKWWidget *PropertiesParent;
  vtkKWWidget *VTKWidget;
  vtkKWLabel *Label;
  int UseProgressGauge;
  vtkKWSegmentedProgressGauge *ProgressGauge;
  vtkKWWidget *Frame;
  vtkKWWidget *Frame2;
  vtkKWWidget *ControlFrame;

  vtkKWFrame             *AnnotationProperties;

  vtkKWFrame             *GeneralProperties;
  vtkKWLabeledFrame      *ColorsFrame;
  vtkKWChangeColorButton *RendererBackgroundColor;

  //vtkRenderer            *Renderer;
  //vtkRenderWindow        *RenderWindow;
  
  int              PropertiesCreated;

  float            InteractiveUpdateRate;
  float            *StillUpdateRates;
  int              NumberOfStillUpdates;
  int              RenderMode;
  int              RenderState;
  
  char             *MenuEntryName;
  char             *MenuEntryHelp;
  int              MenuEntryUnderline;
  
  int              Printing;
  
  int              SupportSaveAsImage;
  int              SupportPrint;
  int              SupportCopy;
  int              SupportControlFrame;

#ifdef _WIN32
  // internal methods used in printing
  void Print(HDC ghdc, HDC adc);
  void Print(HDC ghdc, HDC adc, int printerPageSizeX, int printerPageSizeY,
             int printerDPIX, int printerDPIY,
             float minX, float minY, float scaleX, float scaleY);
#endif
  
private:
  vtkKWView(const vtkKWView&); // Not implemented
  void operator=(const vtkKWView&); // Not implemented
};

#endif

