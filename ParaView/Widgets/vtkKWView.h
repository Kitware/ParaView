/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWView.h
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


#include "vtkKWCompositeCollection.h"
class vtkKWApplication;
class vtkKWWindow;
class vtkViewport;
class vtkKWCornerAnnotation;
class vtkKWMenu;
#include "vtkWindow.h"
#include "vtkKWNotebook.h"
#include "vtkKWEntry.h"
#include "vtkKWCheckButton.h"
#include "vtkKWGenericComposite.h"
#include "vtkTextMapper.h"
#include "vtkScaledTextActor.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWText.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWChangeColorButton.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

class VTK_EXPORT vtkKWView : public vtkKWWidget
{
public:
  vtkTypeMacro(vtkKWView,vtkKWWidget);

  // Description:
  // Create a View
  virtual void Create(vtkKWApplication *app, const char *args) {}

  // Description:
  // Sets up some default event handlers to allow
  // users to manipulate the view etc.
  void SetupBindings();

  // Description:
  // Used to queue up expose event prior to processing.
  vtkSetMacro(InExpose,int);
  vtkGetMacro(InExpose,int);

  // Description:
  // These are the event handlers that UIs can use or override.
  virtual void AButtonPress(int num, int x, int y) {};
  virtual void AButtonRelease(int num, int x, int y) {};
  virtual void AShiftButtonPress(int num, int x, int y) {};
  virtual void AShiftButtonRelease(int num, int x, int y) {};
  virtual void AKeyPress(char key, int x, int y) {};
  virtual void Button1Motion(int x, int y) {};
  virtual void Button2Motion(int x, int y) {};
  virtual void Button3Motion(int x, int y) {};
  virtual void ShiftButton1Motion(int x, int y) {};
  virtual void ShiftButton2Motion(int x, int y) {};
  virtual void ShiftButton3Motion(int x, int y) {};
  virtual void Exposed() {};
  virtual void Enter(int x, int y);

  // Description:
  // Handle the edit copy menu option.
  virtual void EditCopy();

  // Description:
  // Handle the file save as image menu option.
  virtual void SaveAsImage();

  // Description
  // Print the image. This may pop up a dialog box etc.
  virtual void Print();
  int GetPrinting() {return this->Printing;};
  vtkSetMacro(Printing,int);
  
  // Description:
  // Set/Get the last position of the mouse.
  vtkSetVector2Macro(LastPosition,int);
  vtkGetVector2Macro(LastPosition,int);

  // Description;
  // Set the parent window if used so that additional
  // features may be enabled. The parent window is the vtkKWWindow
  // that contains the view.
  vtkGetObjectMacro(ParentWindow,vtkKWWindow);
  void SetParentWindow(vtkKWWindow *);

  // Description:
  // Return the RenderWindow or ImageWindow as appropriate.
  virtual vtkWindow *GetVTKWindow() { return this->RenderWindow; }

  // Description:
  // Return the Renderer or Imager as appropriate.
  virtual vtkViewport *GetViewport() { return this->Renderer; }

  // Description:
  // Methods to support off screen rendering.
  virtual void SetupMemoryRendering(int width,int height, void *cd);
  virtual void ResumeScreenRendering();
  virtual unsigned char *GetMemoryData();
  virtual void *GetMemoryDC();
  
  // Description:
  // Add/Get/Remove the composites.
  virtual void AddComposite(vtkKWComposite *c);
  void RemoveComposite(vtkKWComposite *c);
  vtkKWCompositeCollection *GetComposites() {return this->Composites;};
  
  // Description:
  // Set/Get the selected composite
  vtkGetObjectMacro(SelectedComposite,vtkKWComposite);
  void SetSelectedComposite(vtkKWComposite *);
  
  // Description:
  // Get the attachment point for the Composits properties.
  // This attachment point may be obtained from the parent window
  // if it has been set.
  virtual vtkKWWidget *GetPropertiesParent();

  // Description:
  // Make the properties show up in the view instead of the window
  // or as a top level dialog.
  void CreateDefaultPropertiesParent();

  // Description:
  // Make sure that the Views property parent is currently packed
  void PackProperties();
  
  // Description:
  // Render the image.
  virtual void Render() {this->GetVTKWindow()->Render();};
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
  void UnRegister(vtkObject *o);

  // Description::
  // Indicate when printing if a higher resolution output can be used.
  virtual int RequireUnityScale() {return 0;};

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
  // Callbacks for the property widgets.
  virtual void HeaderChanged();
  virtual void OnDisplayHeader();

  // Description:
  // Allow access to the UI components of interest
  vtkGetObjectMacro(HeaderButton,vtkKWCheckButton);
  vtkGetObjectMacro(HeaderEntry,vtkKWEntry);
  vtkGetObjectMacro(CornerAnnotation,vtkKWCornerAnnotation);

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
  
//BTX
  // Description:
  // Keep these methods public for use in non-member idle callback
  // of vtkKWRenderView
  void SetMultiPassStillAbortCheckMethod(int (*f)(void *), void *arg);
  int              (*MultiPassStillAbortCheckMethod)(void *);
  void             *MultiPassStillAbortCheckMethodArg;
//ETX

  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is,const char token[1024]);
  virtual void SerializeRevision(ostream& os, vtkIndent indent);

  // Description:
  // Change the color of the annotation text
  virtual void SetHeaderTextColor( float r, float g, float b );

  // Description:
  // Turn interactivity on / off - used for UI components that want 
  // interactive rendering while values are being modified.
  void InteractOn();
  void InteractOff();

  // Description:
  // Set the background color
  virtual void SetBackgroundColor( float r, float g, float b ) {};

  // Description:
  // Set the name to be used in the Properties menu for the view properties
  // sheet entry
  vtkSetStringMacro( MenuPropertiesName );
  vtkGetStringMacro( MenuPropertiesName );
  vtkSetMacro( MenuPropertiesUnderline, int );
  vtkGetMacro( MenuPropertiesUnderline, int );

  // Description:
  // Options to enable / disable UI elements. Should be set before the
  // UI is created.
  vtkSetMacro( SupportSaveAsImage, int );
  vtkGetMacro( SupportSaveAsImage, int );
  vtkBooleanMacro( SupportSaveAsImage, int );
  
  vtkSetMacro( SupportPrint, int );
  vtkGetMacro( SupportPrint, int );
  vtkBooleanMacro( SupportPrint, int );

  vtkSetMacro( SupportControlFrame, int );
  vtkGetMacro( SupportControlFrame, int );
  vtkBooleanMacro( SupportControlFrame, int );

  // Description::
  // Get the control frame to put custom controls within
  vtkGetObjectMacro( ControlFrame, vtkKWWidget );
  
  vtkGetObjectMacro(Renderer, vtkRenderer);
  
  // Description:
  // Get the render window used by this widget
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);
  
  // Description:
  // The guts of the abort check method. Made public so that it can
  // be accessed by the render timer callback.
  int ShouldIAbort();
  
protected:
  vtkKWView();
  ~vtkKWView();
  vtkKWView(const vtkKWView&) {};
  void operator=(const vtkKWView&) {};

  vtkKWCornerAnnotation *CornerAnnotation;
  
  vtkKWNotebook *Notebook;
  int InExpose;
  int SharedPropertiesParent;
  float Center[2];
  int LastPosition[2];
  vtkKWWindow *ParentWindow;
  vtkKWCompositeCollection *Composites;
  vtkKWWidget *PropertiesParent;
  vtkKWWidget *VTKWidget;
  vtkKWWidget *Label;
  vtkKWWidget *Frame;
  vtkKWWidget *Frame2;
  vtkKWWidget *ControlFrame;
  vtkKWComposite *SelectedComposite;

  vtkKWWidget            *AnnotationProperties;

  vtkKWGenericComposite  *HeaderComposite;
  vtkKWLabeledFrame      *HeaderFrame;
  vtkKWWidget            *HeaderDisplayFrame;
  vtkKWWidget            *HeaderEntryFrame;
  vtkKWChangeColorButton *HeaderColor;
  vtkKWCheckButton       *HeaderButton;
  vtkKWWidget            *HeaderLabel;
  vtkKWEntry             *HeaderEntry;
  vtkScaledTextActor     *HeaderProp;
  vtkTextMapper          *HeaderMapper;

  vtkKWWidget            *GeneralProperties;
  vtkKWLabeledFrame      *BackgroundFrame;
  vtkKWChangeColorButton *BackgroundColor;

  vtkRenderer            *Renderer;
  vtkRenderWindow        *RenderWindow;
  
  int              PropertiesCreated;

  float            InteractiveUpdateRate;
  float            *StillUpdateRates;
  int              NumberOfStillUpdates;
  int              RenderMode;
  int              RenderState;
  
  char             *MenuPropertiesName;
  int              MenuPropertiesUnderline;
  
  int              Printing;
  
  int              SupportSaveAsImage;
  int              SupportPrint;
  int              SupportControlFrame;
  
};


#endif


