/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWindow.h
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
class vtkKWRotateCameraInteractor;


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
  vtkGetObjectMacro(SourceMenu, vtkKWMenu);
  vtkGetObjectMacro(FilterMenu, vtkKWMenu);
  
  vtkGetObjectMacro(SelectPointInteractor, vtkKWInteractor);
  
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
  void SaveInTclScript();

  // Description:
  // Save the pipeline ParaView Tcl script
  void SaveWorkspace();

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
  // Callback from the glyph button.
  void GlyphCallback();
  
  // Description:
  // Callback from the probe button.
  void ProbeCallback();
  
  // Description:
  // Callback from the frame rate scale.
  void FrameRateScaleCallback();

  // Description:
  // Callback from the reduction check.
  void ReductionCheckCallback();
  
  virtual void Close();

  // Stuff for creating a log file for times.
  void StartLog();
  void StopLog();

  vtkGetObjectMacro(CalculatorButton, vtkKWPushButton);
  vtkGetObjectMacro(ThresholdButton, vtkKWPushButton);
  vtkGetObjectMacro(ContourButton, vtkKWPushButton);
  vtkGetObjectMacro(GlyphButton, vtkKWPushButton);
  vtkGetObjectMacro(ProbeButton, vtkKWPushButton);
  vtkGetObjectMacro(InteractorToolbar, vtkKWToolbar);

  // Description:
  // Get a source interface from the class name.
  // Useful for writing scripts that create sources.
  vtkPVSourceInterface *GetSourceInterface(const char *className);
  
  // Description:
  // Access to the RotateCamera interactor for thinks like setting its center of roation.
  vtkKWRotateCameraInteractor *GetRotateCameraInteractor()
    {return this->RotateCameraInteractor;}

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
  vtkKWRotateCameraInteractor *RotateCameraInteractor;
  vtkKWInteractor *TranslateCameraInteractor;
  vtkKWInteractor *SelectPointInteractor;
  
  vtkKWToolbar *Toolbar;
  vtkKWPushButton *CalculatorButton;
  vtkKWPushButton *ThresholdButton;
  vtkKWPushButton *ContourButton;
  vtkKWPushButton *GlyphButton;
  vtkKWPushButton *ProbeButton;
  
  vtkKWLabel *FrameRateLabel;
  vtkKWScale *FrameRateScale;

  vtkKWCheckButton *ReductionCheck;
  
  vtkKWCompositeCollection *Sources;
  vtkKWLabeledFrame *ApplicationAreaFrame;

  // Used internally.  Down casts vtkKWApplication to vtkPVApplication
  vtkPVApplication *GetPVApplication();

  // Separating out creation of the main view.
  void CreateMainView(vtkPVApplication *pvApp);
  
  // Get rid of all references we own.
  void PrepareForDelete();

  void ReadSourceInterfaces();
  void ReadSourceInterfacesFromString(const char*);
  void ReadSourceInterfacesFromFile(const char*);
  int ReadSourceInterfacesFromDirectory(const char*);
  
  vtkCollection *SourceInterfaces;
  
  vtkPVData *CurrentPVData;
  
private:
  static const char* StandardSourceInterfaces;
  static const char* StandardFilterInterfaces;
};


#endif
