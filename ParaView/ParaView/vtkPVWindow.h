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
// .NAME vtkPVWindow - Main ParaView user interface.
// .SECTION Description
// This class represents a top level window with menu bar and status
// line. It is designed to hold one or more vtkPVViews in it.
// The menu bar contains the following menus:
// @verbatim
// File Menu    -> File manipulations
// View Menu    -> Changes what is displayed on the properties
//                 window (on the left)
// SelectMenu   -> used to select existing data objects
// GlyphMenu    -> used to select existing glyph objects (cascaded from
//                 SelectMenu)
// AdvancedMenu -> for advanced users, contains SourceMenu and FilterMenu,
//                 buttons for command prompt, exporting VTK scripts...
// Help         -> Brings up on-line help
// 
// @endverbatim
// Below the menus, there is a frame separated into two. On the left,
// the "properties" frame is placed and on the right, the render window is
// placed. The "properties" frame is multi-functional. It can display
// general settings, the properties of the current data object/source etc.
// Finally, the status and progress bar (current unused) are packed at
// the bottom.
//
// The PVWindow holds all the existing PVSources (prototypes and instances) 
// and provides access to them. The prototypes are instantiated by
// parsing an XML configuration file. It also has methods to create new data
// objects/sources by cloning prototypes. These data objects can be
// selected either from the SelectMenu or using the navigation window.
// All the reader module prototypes are also stored in the window. The window
// consults each of these when opening a file. The first one which claims
// it can open the current file is passed the filename and is asked to read it.
//
// .SECTION See Also
// vtkPVSource vtkPVData vtkPVRenderView vtkPVNavigationWindow

#ifndef __vtkPVWindow_h
#define __vtkPVWindow_h

#include "vtkKWWindow.h"

class vtkActor;
class vtkAxes;
class vtkGenericRenderWindowInteractor;
class vtkKWCheckButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWNotebook;
class vtkKWPushButton;
class vtkKWRadioButton;
class vtkKWRotateCameraInteractor;
class vtkKWScale;
class vtkKWTclInteractor;
class vtkKWToolbar;
class vtkPVAnimationInterface;
class vtkPVApplication;
class vtkPVData;
class vtkPVErrorLogDisplay;
class vtkPVGenericRenderWindowInteractor;
class vtkPVInteractorStyleCenterOfRotation;
class vtkPVInteractorStyleFly;
class vtkPVInteractorStyle;
class vtkPVReaderModule;
class vtkPVRenderView;
class vtkPVSource;
class vtkPVSourceCollection;
class vtkPVTimerLogDisplay;
class vtkPVXMLPackageParser;
class vtkPolyDataMapper;
class vtkCollection;
class vtkPVColorMap;
class vtkPVTrackballRoll;

//BTX
template <class key, class data> 
class vtkArrayMap;
template <class value>
class vtkLinkedList;
//ETX

class VTK_EXPORT vtkPVWindow : public vtkKWWindow
{
public:
  static vtkPVWindow* New();
  vtkTypeMacro(vtkPVWindow,vtkKWWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the window and all of the associated widgets. This
  // essentially creates the whole user interface. ParaView supports
  // only one window.
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // Access to the RenderView.
  vtkGetObjectMacro(MainView, vtkPVRenderView);

  // Description:
  // The current source ...  Setting the current source also sets the
  // current PVData.  It also sets the selected composite to the source.
  void SetCurrentPVSource(vtkPVSource *comp);
  void SetCurrentPVSourceCallback(vtkPVSource *comp);
  vtkPVSource *GetCurrentPVSource() {return this->CurrentPVSource;}
  vtkPVSource *GetPreviousPVSource(int idx = 1);

  // Description:
  // This adds a PVSource to the collection called "listname", and makes 
  // it current. No trace entry is added during this call.
  void AddPVSource(const char* listname, vtkPVSource *pvs);

  // Description:
  // This removes a PVSource from the collection called "listname".
  // No trace entry is added during this call.
  void RemovePVSource(const char* listname, vtkPVSource *pvs);
  
  // Description: 
  // The current data is the data object that will be used as
  // input to the next filter.  It is usually the last output of the
  // current source. The current data determines which filters are 
  // displayed in the filter menu.
  void SetCurrentPVData(vtkPVData *data);
  vtkGetObjectMacro(CurrentPVData, vtkPVData);
  
  // Description:
  // Find a data source with name "sourcename" in the source list called
  // "listname"
  vtkPVSource *GetPVSource(const char* listname, char* sourcename);

  // Description:
  // This method is called when error icon is clicked.
  void ProcessErrorClick();
    
  // Description:
  // Access methods to ParaView specific menus.
  vtkGetObjectMacro(SelectMenu, vtkKWMenu);
  vtkGetObjectMacro(GlyphMenu, vtkKWMenu);
  vtkGetObjectMacro(SourceMenu, vtkKWMenu);
  vtkGetObjectMacro(FilterMenu, vtkKWMenu);
  
  // Description:
  // Callback for the reset camera button.
  void ResetCameraCallback();
  
  // Description:
  // Callback to show the page for the current source.
  // The callback adds to the trace file, the other does not.
  void ShowCurrentSourceProperties();
  void ShowCurrentSourcePropertiesCallback();

  // Description:
  // Callback to show the animation page.
  void ShowAnimationProperties();
  
  // Description:
  // Access to the toolbars.
  vtkGetObjectMacro(Toolbar, vtkKWToolbar);
  vtkGetObjectMacro(InteractorToolbar, vtkKWToolbar);
  
  // Description:
  // Access from script for regression test.
  void SaveInTclScript(const char *filename, int vtkFlag, int askFlag);

  // Description:
  // Save the pipeline as a tcl script.
  void ExportVTKScript();

  // Description:
  // Copy the trace file to a file with the specified file name.
  void SaveTrace();
  
  // Description:
  // Save the state of ParaView as a Tcl script. Not implemented
  // yet.
  void SaveWorkspace();
  
  // Description:
  // Open a data file. Prompt the user for the filename first.
  void OpenCallback();

  // Description:
  // Open a data file. Does not prompt the user.
  // Returns VTK_OK on success and VTK_ERROR on failure.
  int Open(char *fileName);

  // Description:
  // Play the demo.
  void PlayDemo();

  // Description:
  // Callback from the ExtractGrid button.
  vtkPVSource *ExtractGridCallback();

  // Description:

  // Stuff for creating a log file for times.
  void ShowTimerLog();
  void ShowErrorLog();
  
  // Description:
  // Callback fronm the file menus "SaveData" entry.
  // It pops up a dialog to get the filename.
  void WriteData();

  // Description:
  // Methods that can be used from scripts to save data to files.
  void WriteVTKFile(char *filename);
  void WritePVTKFile(char *filename, int ghostLevel);

  // Description:
  // These methods create a new data source/object given a name and a 
  // source list.
  vtkPVSource *CreatePVSource(const char *className)
    { return this->CreatePVSource(className, 0, 1); }
  vtkPVSource *CreatePVSource(const char *className, const char* sourceList)
    { return this->CreatePVSource(className, sourceList, 1); }
  vtkPVSource *CreatePVSource(const char *className, const char* sourceList,
                              int addTraceEntry);
  
  // Description:
  // Access to the interactor styles from tcl.
  vtkGetObjectMacro(RotateCameraStyle, vtkPVInteractorStyle);
  vtkGetObjectMacro(TranslateCameraStyle, vtkPVInteractorStyle);
  vtkGetObjectMacro(FlyStyle, vtkPVInteractorStyleFly);
  
  // Description:
  // Get the source list called "listname". The default source
  // lists are "Sources" and "GlyphSources".
  vtkPVSourceCollection *GetSourceList(const char* listname);

  // Description: 
  // Re-populate the select menu from the list of existing data
  // objects.
  void UpdateSelectMenu();

  // Description:
  // Ability to disable and enable the menus on the menu bar.
  // Most of the manipulation is internal to window.
  void DisableMenus();
  void EnableMenus();

  // Description:
  // Ability to disable and enable the filter buttons on the toolbar.
  // Most of the manipulation is internal to window.
  void DisableToolbarButtons();
  void EnableToolbarButtons();
  void EnableToolbarButton(const char* buttonName);
  void DisableToolbarButton(const char* buttonName);

  // Description:
  // Re-populate the source menu (under Advanced).
  void UpdateSourceMenu();

  // Description:
  // Re-populate the filter menu (under Advanced).
  void UpdateFilterMenu();

  // Description:
  // Display the tcl interactor.
  void DisplayCommandPrompt();
  
  // Description:
  // Experimenting with wizards. Has to cleaned up - Berk
  void WizardCallback();

  // Description:
  // Access to the animation interface for scripting.
  vtkPVAnimationInterface* GetAnimationInterface() 
    {return this->AnimationInterface;}

  // Description:
  // Add a prototype from which a module can be created.
  void AddPrototype(const char* name, vtkPVSource* prototype);

  // Description:
  // Add a push button to the main toolbar.
  void AddToolbarButton(const char* buttonName, const char* imageName,
                        const char* fileName, 
                        const char* command, const char* balloonHelp);

  // Description:
  // Determines whether message dialogs or error macros as used
  // for displaying certain warnings/errors.
  vtkSetMacro(UseMessageDialog, int);
  vtkGetMacro(UseMessageDialog, int);
  vtkBooleanMacro(UseMessageDialog, int);

  // Description:
  // Adds package name. These names are later used when writing
  // VTK scripts.
  void AddPackageName(const char* name);

  // Description:
  // Whether to parse and create the default interfaces at startup
  vtkSetMacro(InitializeDefaultInterfaces, int);
  vtkGetMacro(InitializeDefaultInterfaces, int);
  vtkBooleanMacro(InitializeDefaultInterfaces, int);

  // Description:
  // Open a ParaView package (prompt the user for the filename) 
  // and load the contents. Returns VTK_OK on success.
  int OpenPackage();
  int OpenPackage(const char* fileName);

  // Description:
  // Load in a Tcl based script to drive the application.
  virtual void LoadScript() { this->Superclass::LoadScript(); }
  virtual void LoadScript(const char *name);

  // Description:
  // Callbacks for generic render window interactor
  void MouseAction(int action,int button, int x,int y, int shift,int control);
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

  // Description:
  // Return the main render window interactor.
  vtkGetObjectMacro(GenericInteractor, vtkPVGenericRenderWindowInteractor);

  // Description:
  // Popup the vtk warning message
  virtual void WarningMessage(const char* message);
  
  // Description:
  // Popup the vtk error message
  virtual void ErrorMessage(const char* message);

  // Description:
  // This method returns a color map for a specific global parameter
  // (e.g. Temperature).
  vtkPVColorMap* GetPVColorMap(const char* parameterName);

  // Description:
  // Propagates the center to the manipulators.
  void SetCenterOfRotation(float x, float y, float z);

protected:
  vtkPVWindow();
  ~vtkPVWindow();

  // Main render window
  vtkPVRenderView *MainView;

  // ParaView specific menus
  vtkKWMenu *AdvancedMenu;
  vtkKWMenu *SourceMenu;
  vtkKWMenu *FilterMenu;
  vtkKWMenu *SelectMenu;
  vtkKWMenu *GlyphMenu;
  
  vtkPVInteractorStyle *TranslateCameraStyle;
  vtkPVInteractorStyle *RotateCameraStyle;

  // This should be made into a 3D Widget.
  vtkPVInteractorStyleCenterOfRotation *CenterOfRotationStyle;

  // Fly should also be made into a manipulator.
  vtkPVInteractorStyleFly *FlyStyle;

    
  // Interactor stuff
  vtkKWToolbar *InteractorToolbar;
  vtkKWRadioButton *FlyButton;
  vtkKWRadioButton *RotateCameraButton;
  vtkKWRadioButton *TranslateCameraButton;
  vtkKWRadioButton *TrackballCameraButton;
  
  vtkPVGenericRenderWindowInteractor *GenericInteractor;
  
  // Main toolbar
  vtkKWToolbar *Toolbar;
  
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
  
  // Used internally.  Down casts vtkKWApplication to vtkPVApplication
  vtkPVApplication *GetPVApplication();

  // Separating out creation of the main view.
  void CreateMainView(vtkPVApplication *pvApp);
  
  // Get rid of all references we own.
  void PrepareForDelete();

  // Disable or enable the select menu.
  void EnableSelectMenu();

  // Returns VTK_OK is file exists and is readable.
  int CheckIfFileIsReadable(const char* fname);

  vtkPVSource *CurrentPVSource;
  vtkPVData *CurrentPVData;

  // The animation interface. I put it in window because
  // if we ever get more that one renderer, the animation
  // will save out the window with all renderers.
  vtkPVAnimationInterface *AnimationInterface;

  // Initialization methods called from create.
  void InitializeMenus(vtkKWApplication* app);
  void InitializeToolbars(vtkKWApplication* app);
  void InitializeInteractorInterfaces(vtkKWApplication* app);

  vtkKWTclInteractor *TclInteractor;
  vtkPVTimerLogDisplay *TimerLogDisplay;
  vtkPVErrorLogDisplay *ErrorLogDisplay;

  // Description:
  // This method gives the window an opportunity to get rid
  // of circular references before closing.
  virtual void CloseNoPrompt();

  // Extensions of files that loaded readers recognize.
  char *FileExtensions;
  char *FileDescriptions;
  // Add a file type and the corresponding prototype
  void AddFileType(const char* description, const char* ext, 
                   vtkPVReaderModule* prototype);

  // Read interface description from XML.
  void ReadSourceInterfaces();
  void ReadSourceInterfacesFromString(const char*);
  void ReadSourceInterfacesFromFile(const char*);
  int ReadSourceInterfacesFromDirectory(const char*);

//BTX
  vtkArrayMap<const char*, vtkPVSource*>* Prototypes;
  vtkArrayMap<const char*, vtkPVSourceCollection*>* SourceLists;
  vtkArrayMap<const char*, vtkKWPushButton*>* ToolbarButtons;
  vtkArrayMap<const char*, const char*>* Writers;
  vtkLinkedList<vtkPVReaderModule*>* ReaderList;
  vtkLinkedList<const char*>* PackageNames;

  friend class vtkPVXMLPackageParser;
//ETX

  vtkCollection *PVColorMaps;

  // This can be used to disable the pop-up dialogs if necessary
  // (usually used from inside regression scripts)
  int UseMessageDialog;

  // Whether or not to read the default interfaces.
  int InitializeDefaultInterfaces;

  // Description:
  // Utility function which return the position of the first '.'
  // from the right. Note that this returns a pointer offset
  // from the original pointer. DO NOT DELETE THE ORIGINAL POINTER
  // while using the extension.
  const char* ExtractFileExtension(const char* fname);

  // Description:
  // Create error log display.
  void CreateErrorLogDisplay();

private:
  static const char* StandardReaderInterfaces;
  static const char* StandardSourceInterfaces;
  static const char* StandardFilterInterfaces;


  vtkPVWindow(const vtkPVWindow&); // Not implemented
  void operator=(const vtkPVWindow&); // Not implemented
};


#endif
