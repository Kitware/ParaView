/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWView.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

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

class VTK_EXPORT vtkKWView : public vtkKWWidget
{
public:
  vtkTypeMacro(vtkKWView,vtkKWWidget);

  // Description:
  // Create a View
  virtual void Create(vtkKWApplication *app, char *args) = 0;

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
  virtual void AKeyPress(char key, int x, int y) {};
  virtual void Button1Motion(int x, int y) {};
  virtual void Button2Motion(int x, int y) {};
  virtual void Button3Motion(int x, int y) {};
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
  virtual vtkWindow *GetVTKWindow() = 0;

  // Description:
  // Return the Renderer or Imager as appropriate.
  virtual vtkViewport *GetViewport() = 0;

  // Description:
  // Methods to support off screen rendering.
  virtual void SetupMemoryRendering(int width,int height, void *cd) = 0;
  virtual void ResumeScreenRendering() = 0;
  virtual unsigned char *GetMemoryData() = 0;
  virtual void *GetMemoryDC() = 0;
  
  // Description:
  // Add/Get/Remove the composites.
  void AddComposite(vtkKWComposite *c);
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
  virtual void SetTitle(char *);

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

  virtual void CornerChanged();
  virtual void OnDisplayCorner();
  virtual void CornerSelected(int c);
  
  // Description:
  // Allow access to the UI components of interest
  vtkGetObjectMacro(HeaderButton,vtkKWCheckButton);
  vtkGetObjectMacro(HeaderEntry,vtkKWEntry);
  vtkGetObjectMacro(CornerButton,vtkKWCheckButton);
  vtkGetObjectMacro(CornerText,vtkKWText);

  vtkSetMacro( InteractiveUpdateRate, float );
  vtkGetMacro( InteractiveUpdateRate, float );

  void SetStillUpdateRates( int count, float *rates );
  vtkGetMacro( NumberOfStillUpdates, int );
  float *GetStillUpdateRates() { return this->StillUpdateRates; };

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

//BTX
  void SetMultiPassStillAbortCheckMethod(int (*f)(void *), void *arg);
//ETX

  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is,const char token[1024]);

  // Description:
  // Change the color of the annotation text
  void SetHeaderTextColor( float r, float g, float b );
  void SetCornerTextColor( float r, float g, float b );

protected:
  vtkKWView();
  ~vtkKWView();
  vtkKWView(const vtkKWView&) {};
  void operator=(const vtkKWView&) {};

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

  vtkKWGenericComposite  *CornerComposite;
  vtkKWLabeledFrame      *CornerFrame;
  vtkKWWidget            *CornerDisplayFrame;
  vtkKWChangeColorButton *CornerColor;
  vtkKWCheckButton       *CornerButton;
  vtkKWWidget            *CornerLabel;
  vtkKWText              *CornerText;
  vtkScaledTextActor     *CornerProp;
  vtkTextMapper          *CornerMapper;
  vtkKWWidget            *CornerOptionsFrame;
  vtkKWWidget            *CornerOptionsLabel;
  vtkKWOptionMenu        *CornerOptions;

  int              PropertiesCreated;

  float            InteractiveUpdateRate;
  float            *StillUpdateRates;
  int              NumberOfStillUpdates;
  int              RenderMode;

//BTX
  int              (*MultiPassStillAbortCheckMethod)(void *);
//ETX

  void             *MultiPassStillAbortCheckMethodArg;
  int Printing;
  float PrintTargetDPI;
};


#endif


