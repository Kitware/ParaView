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
#include "vtkPVSource.h"

class vtkPVRenderView;
class vtkKWNotebook;
class vtkKWToolbar;
class vtkKWScale;
class vtkKWPushButton;
class vtkKWRadioButton;
class vtkKWLabel;
class vtkKWCheckButton;
class vtkPVAnimationInterface;
class vtkKWRotateCameraInteractor;
class vtkKWCompositeCollection;
class vtkKWTclInteractor;
class vtkPVTimerLogDisplay;
class vtkPVGenericRenderWindowInteractor;
class vtkGenericRenderWindowInteractor;
class vtkPVInteractorStyleTranslateCamera;
class vtkPVInteractorStyleRotateCamera;
class vtkInteractorStyleTrackballCamera;
class vtkPVInteractorStyleCenterOfRotation;
class vtkPVInteractorStyleFly;
class vtkAxes;
class vtkActor;
class vtkPolyDataMapper;

class VTK_EXPORT vtkPVWindow : public vtkKWWindow
{
public:
  static vtkPVWindow* New();
  vtkTypeMacro(vtkPVWindow,vtkKWWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  void SetCurrentPVSourceCallback(vtkPVSource *comp);
  vtkPVSource *GetCurrentPVSource() {return this->CurrentPVSource;}
  vtkPVSource *GetPreviousPVSource(int idx = 1);

  // Description:
  // This adds a PVSource to the Source collection, and makes it current.
  // No trace entry is added during this call.
  void AddPVSource(vtkPVSource *pvs);
  void RemovePVSource(vtkPVSource *pvs);
  
  // Description:
  // The current data is the data object that will be used as input to the next filter.
  // It is usually the last output of the current source.  If the current source
  // has more than one output, they can be selected through the UI.  The current data
  // determines which filters are displayed in the filter menu.
  void SetCurrentPVData(vtkPVData *data);
  vtkGetObjectMacro(CurrentPVData, vtkPVData);
  
  // Description:
  // This is a special list of precreated sources that can be used to glyph.
  vtkCollection *GetGlyphSources();

  // Description:
  // This access method is needed for scripts that modify the glyph source.
  // It indexes the sources by name.
  vtkPVSource *GetGlyphSource(char* name);
    
  vtkGetObjectMacro(SelectMenu, vtkKWMenu);
  vtkGetObjectMacro(GlyphMenu, vtkKWMenu);
  vtkGetObjectMacro(SourceMenu, vtkKWMenu);
  vtkGetObjectMacro(FilterMenu, vtkKWMenu);
  
  // Description:
  // Callback from the reset camera button.
  void ResetCameraCallback();
  
  // Description:
  // Callback to display window proerties
  void ShowWindowProperties();
  
  // Description:
  // Callback to show the page for the current source.
  // The callback adds to the trace file, the other does not.
  void ShowCurrentSourceProperties();
  void ShowCurrentSourcePropertiesCallback();

  // Description:
  // Callback to show the animation page.
  void ShowAnimationProperties();
  
  // Description:
  // Need to be able to get the toolbar so I can add buttons from outside
  // this class.
  vtkGetObjectMacro(Toolbar, vtkKWToolbar);
  
  // Description:
  // Access from script for regression test.
  void SaveInTclScript(const char *filename, int vtkFlag);

  // Description:
  // Save the pipeline as a tcl script.
  void ExportVTKScript();

  // Description:
  // Copy the trace file to a file with the specified file name.
  void SaveTrace();
  
  // Description:
  // Save the pipeline ParaView Tcl script
  void SaveWorkspace();
  
  // Description:
  // Open a data file.
  void OpenCallback();
  vtkPVSource *Open(char *fileName);
  int OpenRecentFile(char *fileName);
  
  // Description:
  // Play the demo
  void PlayDemo();

  // Description:
  // Callback from the calculator button.
  vtkPVSource *CalculatorCallback();
  
  // Description:
  // Callback from the Cut button.
  vtkPVSource *CutCallback();

  // Description:
  // Callback from the Clip button.
  vtkPVSource *ClipCallback();

  // Description:
  // Callback from the ExtractGrid button.
  vtkPVSource *ExtractGridCallback();

  // Description:
  // Callback from the threshold button.
  vtkPVSource *ThresholdCallback();

  // Description:
  // Callback from the contour button.
  vtkPVSource *ContourCallback();

  // Description:
  // Callback from the glyph button.
  vtkPVSource *GlyphCallback();
  
  // Description:
  // Callback from the probe button.
  vtkPVSource *ProbeCallback();
  
  // Description:
  // Callback from the extract voids button.
  vtkPVSource *ExtractVoidsCallback();
  
  virtual void Close();

  // Stuff for creating a log file for times.
  void ShowLog();
  
  // Description:
  // Callback fronm the file menus "SaveData" entry.
  // It pops up a dialog to get the filename.
  void WriteData();

  // Description:
  // Methods that can be used from scripts to save data to files.
  void WriteVTKFile(char *filename);
  void WritePVTKFile(char *filename, int ghostLevel);


  vtkGetObjectMacro(CalculatorButton, vtkKWPushButton);
  vtkGetObjectMacro(CutButton, vtkKWPushButton);
  vtkGetObjectMacro(ClipButton, vtkKWPushButton);
  vtkGetObjectMacro(ExtractGridButton, vtkKWPushButton);
  vtkGetObjectMacro(ThresholdButton, vtkKWPushButton);
  vtkGetObjectMacro(ContourButton, vtkKWPushButton);
  vtkGetObjectMacro(GlyphButton, vtkKWPushButton);
  vtkGetObjectMacro(ProbeButton, vtkKWPushButton);
  vtkGetObjectMacro(ExtractVoidsButton, vtkKWPushButton);
  vtkGetObjectMacro(InteractorStyleToolbar, vtkKWToolbar);

  // Description:
  // Get a source interface from the class name.
  // Useful for writing scripts that create sources.
  vtkPVSourceInterface *GetSourceInterface(const char *className);
  vtkPVSource *CreatePVSource(const char *className);
  
  // Description:
  // Access to the interactor styles from tcl.
  vtkGetObjectMacro(RotateCameraStyle, vtkPVInteractorStyleRotateCamera);
  vtkGetObjectMacro(TranslateCameraStyle, vtkPVInteractorStyleTranslateCamera);
  vtkGetObjectMacro(FlyStyle, vtkPVInteractorStyleFly);
  
  // Description:
  // This list contains all the sources created by the user.
  // It is used to create input menus for filters.
  vtkCollection *GetSources();

  // Description:
  // When you add a source to the source list, you sould update the select menu.
  // This object should probably have AddPVSource RemovePVSource methods ...
  void UpdateSelectMenu();

  // Description:
  // Ability to disable and enable the menus on the menu bar.
  // Most of the manipulation is internal to window.
  void DisableMenus();
  void EnableMenus();

  // Description:
  // Ability to disable and enable the filter buttons on the toolbar.
  // Most of the manipulation is internal to window.
  void DisableFilterButtons();
  void EnableFilterButtons();

  // Description:
  // Display the tcl interactor
  void DisplayCommandPrompt();
  
  // Description:
  // Experimenting with wizards.
  void WizardCallback();

  // Description:
  // Access to the animation interface for scripting.
  vtkPVAnimationInterface* GetAnimationInterface() {return this->AnimationInterface;}

  // Description:
  // Experimenting with modules.
  int LoadModule(const char *name);
  int GetModuleLoaded(const char *name);

  // Description:
  // Callbacks for generic render window interactor
  void AButtonPress(int button, int x, int y);
  void AShiftButtonPress(int button, int x, int y);
  void AButtonRelease(int button, int x, int y);
  void AShiftButtonRelease(int button, int x, int y);
  void MouseMotion(int x, int y);
  void Configure(int width, int height);
  
  // Description:
  // Change the current interactor style
  void ChangeInteractorStyle(int index);

  // Description:
  // Callbacks for center of rotation widgets
  void CenterEntryOpenCallback();
  void CenterEntryCloseCallback();
  void CenterEntryCallback();
  void ResetCenterCallback();

  // Description
  // Access to these widgets from outside vtkPVWindow
  // (in vtkPVInteractorStyleCenterOfRotation)
  vtkGetObjectMacro(CenterXEntry, vtkKWEntry);
  vtkGetObjectMacro(CenterYEntry, vtkKWEntry);
  vtkGetObjectMacro(CenterZEntry, vtkKWEntry);

  // Description:
  // Callback for fly speed widget
  void FlySpeedScaleCallback();
  
protected:
  vtkPVWindow();
  ~vtkPVWindow();

  vtkPVRenderView *MainView;
  vtkKWMenu *AdvancedMenu;
  vtkKWMenu *SourceMenu;
  vtkKWMenu *FilterMenu;
  vtkKWMenu *SelectMenu;
  vtkKWMenu *GlyphMenu;
  
  vtkInteractorStyleTrackballCamera *TrackballCameraStyle;
  vtkPVInteractorStyleTranslateCamera *TranslateCameraStyle;
  vtkPVInteractorStyleRotateCamera *RotateCameraStyle;
  vtkPVInteractorStyleCenterOfRotation *CenterOfRotationStyle;
  vtkPVInteractorStyleFly *FlyStyle;
  
  vtkKWToolbar *InteractorStyleToolbar;
  vtkKWRadioButton *FlyButton;
  vtkKWRadioButton *RotateCameraButton;
  vtkKWRadioButton *TranslateCameraButton;
  vtkKWRadioButton *TrackballCameraButton;
  
  vtkPVGenericRenderWindowInteractor *GenericInteractor;
  
  vtkKWToolbar *Toolbar;
  vtkKWPushButton *CalculatorButton;
  vtkKWPushButton *CutButton;
  vtkKWPushButton *ClipButton;
  vtkKWPushButton *ExtractGridButton;
  vtkKWPushButton *ThresholdButton;
  vtkKWPushButton *ContourButton;
  vtkKWPushButton *GlyphButton;
  vtkKWPushButton *ProbeButton;
  vtkKWPushButton *ExtractVoidsButton;
  
  // widgets for setting center of rotation for rotate camera interactor style
  vtkKWToolbar *PickCenterToolbar;
  vtkKWPushButton *PickCenterButton;
  vtkKWPushButton *ResetCenterButton;
  vtkKWPushButton *CenterEntryOpenButton;
  vtkKWPushButton *CenterEntryCloseButton;
  vtkKWWidget *CenterEntryFrame;
  vtkKWLabel *CenterXLabel;
  vtkKWEntry *CenterXEntry;
  vtkKWLabel *CenterYLabel;
  vtkKWEntry *CenterYEntry;
  vtkKWLabel *CenterZLabel;
  vtkKWEntry *CenterZEntry;
  
  // widgets for setting fly speed for fly interactor style
  vtkKWToolbar *FlySpeedToolbar;
  vtkKWLabel *FlySpeedLabel;
  vtkKWScale *FlySpeedScale;
  
  vtkAxes *CenterSource;
  vtkPolyDataMapper *CenterMapper;
  vtkActor *CenterActor;
  void ResizeCenterActor();
  
  vtkCollection *Sources;
  // Special list of static sources that can be used for glyphing.
  vtkCollection *GlyphSources;
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
  
  vtkPVSource *CurrentPVSource;
  vtkPVData *CurrentPVData;

  // The animation interface. I put it in window because
  // if we ever get more that one renderer, the animation
  // will save out the window with all renderers.
  vtkPVAnimationInterface *AnimationInterface;
  
  vtkKWTclInteractor *TclInteractor;
  vtkPVTimerLogDisplay *TimerLogDisplay;

  // Description:
  // This method gives the window an opportunity to get rid
  // of circular references before closing.
  virtual void CloseNoPrompt();

  // Extensions of files that loaded readers recognize.
  char *FileExtensions;
  char *FileDescriptions;
  void AddFileType(const char* description, const char* ext);
  vtkStringList *ReaderInterfaces;

private:
  static const char* StandardSourceInterfaces;
  static const char* StandardFilterInterfaces;

  // Modules.
  vtkStringList *Modules;
  vtkPVSource *OpenWithReaderInterface(const char *openFileName, 
				       const char *rootName,
                                       const char *rIntName);

  vtkPVWindow(const vtkPVWindow&); // Not implemented
  void operator=(const vtkPVWindow&); // Not implemented
};


#endif
