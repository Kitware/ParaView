/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkPVWindow
// .SECTION Description
// This class represents a top level window with menu bar and status
// line. It is designed to hold one or more vtkPVViews in it.

#ifndef __vtkPVWindow_h
#define __vtkPVWindow_h

#include "vtkKWWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkInteractorStyleTrackballCamera.h"

class vtkKWNotebook;
class vtkKWToolbar;
class vtkKWScale;
class vtkKWPushButton;
class vtkKWInteractor;


class VTK_EXPORT vtkPVWindow : public vtkKWWindow
{
public:
  static vtkPVWindow* New();
  vtkTypeMacro(vtkPVWindow,vtkKWWindow);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // I assume this creates a new applciation window.
  void NewWindow();
  
  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is,const char token[1024]);
    
  // Description:
  // Access to the RenderView.
  vtkGetObjectMacro(MainView, vtkPVRenderView);

  // Description:
  // The current source ...  Setting the current source also sets the current PVData.
  // It also sets the selected composite to the source.
  void SetCurrentPVSource(vtkPVSource *comp);
  vtkPVSource *GetCurrentPVSource();
  vtkPVSource *GetPreviousPVSource();
  
  // Description:
  // The current data is the data object that will be used as input to the next filter.
  // It is usually the last output of the current source.  If the current source
  // has more than one output, they can be selected through the UI.  The current data
  // determines which filters are displayed in the filter menu.
  void SetCurrentPVData(vtkPVData *data);
  vtkGetObjectMacro(CurrentPVData, vtkPVData);
  
  vtkKWCompositeCollection *GetSources();
    
  vtkGetObjectMacro(SelectMenu, vtkKWMenu);
  
  // Description:
  // Callback from the reset camera button.
  void ResetCameraCallback();
  
  // Description:
  // Callback to display window proerties
  void ShowWindowProperties();
  
  // Description:
  // Callback to show the page for the current source
  void ShowCurrentSourceProperties();
  
  // Description:
  // Need to be able to get the toolbar so I can add buttons from outside
  // this class.
  vtkGetObjectMacro(Toolbar, vtkKWToolbar);
  
  // Description:
  // Save the pipeline as a tcl script.
  void Save();

  // Description:
  // Open a data file.
  void Open();
  
  // Description:
  // Callback from the calculator button.
  void CalculatorCallback();
  
  // Description:
  // Callback from the threshold button.
  void ThresholdCallback();

  // Description:
  // Callback from the contour button.
  void ContourCallback();

  // Description:
  // Callback from the contour button.
  void GlyphCallback();

protected:
  vtkPVWindow();
  ~vtkPVWindow();
  vtkPVWindow(const vtkPVWindow&) {};
  void operator=(const vtkPVWindow&) {};

  vtkPVRenderView *MainView;
  vtkKWMenu *SourceMenu;
  vtkKWMenu *FilterMenu;
  vtkKWMenu *SelectMenu;
  vtkKWMenu *VTKMenu;
  
  vtkInteractorStyleTrackballCamera *CameraStyle;
  
  vtkKWToolbar *InteractorToolbar;
  vtkKWPushButton *CameraStyleButton;
  vtkKWInteractor *FlyInteractor;
  vtkKWInteractor *RotateCameraInteractor;
  vtkKWInteractor *TranslateCameraInteractor;


  vtkKWToolbar *Toolbar;
  vtkKWPushButton *CalculatorButton;
  vtkKWPushButton *ThresholdButton;
  vtkKWPushButton *ContourButton;
  vtkKWPushButton *GlyphButton;

  vtkKWCompositeCollection *Sources;
  vtkKWLabeledFrame *ApplicationAreaFrame;

  // Used internally.  Down casts vtkKWApplication to vtkPVApplication
  vtkPVApplication *GetPVApplication();

  // Separating out creation of the main view.
  void CreateMainView(vtkPVApplication *pvApp);
  
  void ReadSourceInterfaces();
  vtkCollection *SourceInterfaces;
  
  vtkPVData *CurrentPVData;
};


#endif
