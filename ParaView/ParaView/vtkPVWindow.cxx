/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWindow.cxx
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
#include "vtkPVWindow.h"

#include "vtkActor.h"
#include "vtkArrayMap.txx"
#include "vtkAxes.h"
#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkDataSet.h"
#include "vtkDirectory.h"
#include "vtkImageData.h"
#include "vtkInstantiator.h"
#include "vtkKWDirectoryUtilities.h"
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWNotebook.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWPushButton.h"
#include "vtkKWRadioButton.h"
#include "vtkKWScale.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWToolbar.h"
#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterface.h"
#include "vtkPVApplication.h"
#include "vtkPVApplicationSettingsInterface.h"
#include "vtkPVCameraManipulator.h"
#include "vtkPVColorMap.h"
#include "vtkPVConfig.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVErrorLogDisplay.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVGhostLevelDialog.h"
#include "vtkPVInitialize.h"
#include "vtkPVInputProperty.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVInteractorStyleCenterOfRotation.h"
#include "vtkPVInteractorStyleControl.h"
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVProcessModule.h"
#include "vtkPVReaderModule.h"
#include "vtkPVRenderModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSaveBatchScriptDialog.h"
#include "vtkPVSelectCustomReader.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSourceInterfaceDirectories.h"
#include "vtkPVTimerLogDisplay.h"
#include "vtkPVWriter.h"
#include "vtkPVXMLPackageParser.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRectilinearGrid.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkString.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkToolkits.h"
#include "vtkUnstructuredGrid.h"

#ifdef _WIN32
#include "vtkKWRegisteryUtilities.h"
#endif

#include <ctype.h>
#include <sys/stat.h>

#ifndef VTK_USE_ANSI_STDLIB
#define PV_NOCREATE | ios::nocreate
#else
#define PV_NOCREATE 
#endif

#define VTK_PV_VTK_FILTERS_MENU_LABEL "Filter"
#define VTK_PV_VTK_SOURCES_MENU_LABEL "Source"
#define VTK_PV_OPEN_DATA_MENU_LABEL "Open Data"
#define VTK_PV_SAVE_DATA_MENU_LABEL "Save Data"
#define VTK_PV_SELECT_SOURCE_MENU_LABEL "Select"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVWindow);
vtkCxxRevisionMacro(vtkPVWindow, "1.462.2.15");

int vtkPVWindowCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//-----------------------------------------------------------------------------
vtkPVWindow::vtkPVWindow()
{
  this->Interactor = 0;

  this->InteractiveRenderEnabled = 0;
  this->NamesToSources = 0;
  this->SetWindowClass("ParaView");
  this->SetTitle("Kitware ParaView");

  this->CommandFunction = vtkPVWindowCommand;

  // ParaView specific menus:
  // SelectMenu   -> used to select existing data objects
  // GlyphMenu    -> used to select existing glyph objects (cascaded from
  //                 SelectMenu)
  // SourceMenu   -> available source modules
  // FilterMenu   -> available filter modules (depending on the current 
  //                 data object's type)
  this->SourceMenu = vtkKWMenu::New();
  this->FilterMenu = vtkKWMenu::New();
  this->SelectMenu = vtkKWMenu::New();
  this->GlyphMenu = vtkKWMenu::New();

  // This toolbar contains buttons for modifying user interaction
  // mode
  this->InteractorToolbar = vtkKWToolbar::New();
  this->Toolbars->AppendItem(this->InteractorToolbar);

  //this->FlyButton = vtkKWRadioButton::New();
  this->RotateCameraButton = vtkKWRadioButton::New();
  this->TranslateCameraButton = vtkKWRadioButton::New();
    
  // This toolbar contains buttons for instantiating new modules
  this->Toolbar = vtkKWToolbar::New();
  this->Toolbars->AppendItem(this->Toolbar);

  // Keep a list of the toolbar buttons so that they can be 
  // disabled/enabled in certain situations.
  this->ToolbarButtons = vtkArrayMap<const char*, vtkKWPushButton*>::New();

  this->CameraStyle3D = vtkPVInteractorStyle::New();
  this->CameraStyle2D = vtkPVInteractorStyle::New();
  this->CenterOfRotationStyle = vtkPVInteractorStyleCenterOfRotation::New();
  
  this->PickCenterToolbar = vtkKWToolbar::New();
  this->Toolbars->AppendItem(this->PickCenterToolbar);

  this->PickCenterButton = vtkKWPushButton::New();
  this->ResetCenterButton = vtkKWPushButton::New();
  this->HideCenterButton = vtkKWPushButton::New();
  this->CenterEntryOpenCloseButton = vtkKWPushButton::New();
  this->CenterEntryFrame = vtkKWWidget::New();
  this->CenterXLabel = vtkKWLabel::New();
  this->CenterXEntry = vtkKWEntry::New();
  this->CenterYLabel = vtkKWLabel::New();
  this->CenterYEntry = vtkKWEntry::New();
  this->CenterZLabel = vtkKWLabel::New();
  this->CenterZEntry = vtkKWEntry::New();
  
  this->CenterSourceTclName = NULL;
  this->CenterMapperTclName = NULL;
  this->CenterActorTclName = NULL;
    
  this->CurrentPVSource = NULL;

  // Frame used for animations.
  this->AnimationInterface = vtkPVAnimationInterface::New();
  this->AnimationInterface->SetTraceReferenceObject(this);
  this->AnimationInterface->SetTraceReferenceCommand("GetAnimationInterface");
  this->AnimationInterface->SetApplication(this->GetApplication());

  this->TimerLogDisplay = NULL;
  this->ErrorLogDisplay = NULL;

  // Set the extension and the type (name) of the script for
  // this application. They are all Tcl scripts but we give
  // them different names to differentiate them.
  this->SetScriptExtension(".pvs");
  this->SetScriptType("ParaView");

  // Used to store the extensions and descriptions for supported
  // file formats (in Tk dialog format i.e. {ext1 ext2 ...} 
  // {{desc1} {desc2} ...}
  this->FileExtensions = NULL;
  this->FileDescriptions = NULL;

  // The prototypes for source and filter modules. Instances are 
  // created by calling CloneAndInitialize() on these.
  this->Prototypes = vtkArrayMap<const char*, vtkPVSource*>::New();
  // The prototypes for reader modules. Instances are 
  // created by calling ReadFile() on these.
  this->ReaderList = vtkLinkedList<vtkPVReaderModule*>::New();

  // The writer modules.
  this->FileWriterList = vtkLinkedList<vtkPVWriter*>::New();

  // This stores the state of a menu during grab.
  this->MenuState = vtkArrayMap<const char*, int>::New();

  // The writers (used in SaveInBatchScript) mapped to the extensions
  this->Writers = vtkArrayMap<const char*, const char*>::New();
  this->Writers->SetItem(".jpg", "vtkJPEGWriter");
  this->Writers->SetItem(".JPG", "vtkJPEGWriter");
  this->Writers->SetItem(".png", "vtkPNGWriter");
  this->Writers->SetItem(".PNG", "vtkPNGWriter");
  this->Writers->SetItem(".ppm", "vtkPNMWriter");
  this->Writers->SetItem(".PPM", "vtkPNMWriter");
  this->Writers->SetItem(".pnm", "vtkPNMWriter");
  this->Writers->SetItem(".PNM", "vtkPNMWriter");
  this->Writers->SetItem(".tif", "vtkTIFFWriter");
  this->Writers->SetItem(".TIF", "vtkTIFFWriter");

  // Map <name> -> <source collection>
  // These contain the sources and filters which the user manipulate.
  this->SourceLists = vtkArrayMap<const char*, vtkPVSourceCollection*>::New();
  // Add a default collection for user created readers, sources and
  // filters.
  vtkPVSourceCollection* sources = vtkPVSourceCollection::New();
  this->SourceLists->SetItem("Sources", sources);
  sources->Delete();

  // Keep a list of all loaded packages (Tcl libraries) so that
  // they can be written out when writing Tcl scripts.
  this->PackageNames = vtkLinkedList<const char*>::New();

  // This can be used to disable the pop-up dialogs if necessary
  // (usually used from inside regression scripts)
  this->UseMessageDialog = 1;
  // Whether or not to read the default interfaces.
  this->InitializeDefaultInterfaces = 1;

  this->MainView = 0;

  this->PVColorMaps = vtkCollection::New();

  this->CenterActorVisibility = 1;

  this->ShowSourcesLongHelp = 1;

  this->MenusDisabled = 0;
  this->ToolbarButtonsDisabled = 0;

  this->UserInterfaceManager = 0;
  this->ApplicationSettingsInterface = 0;

  this->InteractorTclName = 0;
}

//-----------------------------------------------------------------------------
vtkPVWindow::~vtkPVWindow()
{
  if (this->InteractorTclName)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    pvApp->BroadcastScript("%s SetRenderWindow {}", this->InteractorTclName);
    pvApp->BroadcastScript("%s Delete", this->InteractorTclName);
    this->SetInteractorTclName(NULL);
    this->SetInteractor(NULL);
    }

  if ( this->NamesToSources )
    {
    this->NamesToSources->Delete();
    this->NamesToSources = 0;
    }

  this->PrepareForDelete();

  // First delete the interface panels

  if (this->ApplicationSettingsInterface)
    {
    this->ApplicationSettingsInterface->Delete();
    }

  // Then the interface manager

  if (this->UserInterfaceManager)
    {
    this->UserInterfaceManager->Delete();
    }

  this->RotateCameraButton->Delete();
  this->RotateCameraButton = NULL;
  this->TranslateCameraButton->Delete();
  this->TranslateCameraButton = NULL;

  this->SourceLists->Delete();
  this->SourceLists = 0;
  
  this->ToolbarButtons->Delete();
  this->ToolbarButtons = 0;

  this->PackageNames->Delete();
  this->PackageNames = 0;

  
  if (this->TimerLogDisplay)
    {
    this->TimerLogDisplay->Delete();
    this->TimerLogDisplay = NULL;
    }

  if (this->ErrorLogDisplay)
    {
    this->ErrorLogDisplay->Delete();
    this->ErrorLogDisplay = NULL;
    }

  if (this->FileExtensions)
    {
    delete [] this->FileExtensions;
    this->FileExtensions = NULL;
    }
  if (this->FileDescriptions)
    {
    delete [] this->FileDescriptions;
    this->FileDescriptions = NULL;
    }
  this->Prototypes->Delete();
  this->ReaderList->Delete();
  this->Writers->Delete();
  this->FileWriterList->Delete();
  this->MenuState->Delete();

  if (this->PVColorMaps)
    {
    this->PVColorMaps->Delete();
    this->PVColorMaps = NULL;
    }
}


//-----------------------------------------------------------------------------
void vtkPVWindow::CloseNoPrompt()
{
  if (this->TimerLogDisplay )
    {
    this->TimerLogDisplay->SetMasterWindow(NULL);
    this->TimerLogDisplay->Delete();
    this->TimerLogDisplay = NULL;
    }

  if (this->ErrorLogDisplay )
    {
    this->ErrorLogDisplay->SetMasterWindow(NULL);
    this->ErrorLogDisplay->Delete();
    this->ErrorLogDisplay = NULL;
    }

  this->vtkKWWindow::CloseNoPrompt();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::PrepareForDelete()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp && this->CenterSourceTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->CenterSourceTclName);
    }
  this->SetCenterSourceTclName(NULL);

  if (pvApp && this->CenterMapperTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->CenterMapperTclName);
    }
  this->SetCenterMapperTclName(NULL);

  if (pvApp && this->CenterActorTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->CenterActorTclName);
    }
  this->SetCenterActorTclName(NULL);


  if (this->AnimationInterface)
    {
    // It is very important to stop the animation at this point, otherwise
    // it will never go out of its animation loop and this will block
    // the whole destruction process since the animation object registers
    // itself before the loop. See vtkPVAnimationInterface::Play() code.
    this->AnimationInterface->PrepareForDelete();
    this->AnimationInterface->Delete();
    this->AnimationInterface = NULL;
    }
  
  // Color maps have circular references because they
  // reference renderview.
  if (this->PVColorMaps)
    {
    this->PVColorMaps->Delete();
    this->PVColorMaps = NULL;
    }

  if (this->CurrentPVSource)
    {
    this->CurrentPVSource->UnRegister(this);
    this->CurrentPVSource = NULL;
    }

  if (this->InteractorToolbar)
    {
    this->InteractorToolbar->Delete();
    this->InteractorToolbar = NULL;
    }

  //if (this->FlyStyle)
  //  {
  //  this->FlyStyle->Delete();
  //  this->FlyStyle = NULL;
  //  }

  if (this->CameraStyle3D)
    {
    this->CameraStyle3D->Delete();
    this->CameraStyle3D = NULL;
    }
  if (this->CameraStyle2D)
    {
    this->CameraStyle2D->Delete();
    this->CameraStyle2D = NULL;
    }
  if (this->CenterOfRotationStyle)
    {
    this->CenterOfRotationStyle->Delete();
    this->CenterOfRotationStyle = NULL;
    }
  
  if (this->Toolbar)
    {
    this->Toolbar->Delete();
    this->Toolbar = NULL;
    }

  if (this->PickCenterButton)
    {
    this->PickCenterButton->Delete();
    this->PickCenterButton = NULL;
    }
  
  if (this->ResetCenterButton)
    {
    this->ResetCenterButton->Delete();
    this->ResetCenterButton = NULL;
    }

  if (this->HideCenterButton)
    {
    this->HideCenterButton->Delete();
    this->HideCenterButton = NULL;
    }
  
  if (this->CenterEntryOpenCloseButton)
    {
    this->CenterEntryOpenCloseButton->Delete();
    this->CenterEntryOpenCloseButton = NULL;
    }
  
  if (this->CenterXLabel)
    {
    this->CenterXLabel->Delete();
    this->CenterXLabel = NULL;
    }
  
  if (this->CenterXEntry)
    {
    this->CenterXEntry->Delete();
    this->CenterXEntry = NULL;
    }
  
  if (this->CenterYLabel)
    {
    this->CenterYLabel->Delete();
    this->CenterYLabel = NULL;
    }
  
  if (this->CenterYEntry)
    {
    this->CenterYEntry->Delete();
    this->CenterYEntry = NULL;
    }
  
  if (this->CenterZLabel)
    {
    this->CenterZLabel->Delete();
    this->CenterZLabel = NULL;
    }
  
  if (this->CenterZEntry)
    {
    this->CenterZEntry->Delete();
    this->CenterZEntry = NULL;
    }
  
  if (this->CenterEntryFrame)
    {
    this->CenterEntryFrame->Delete();
    this->CenterEntryFrame = NULL;
    }
  
  if (this->PickCenterToolbar)
    {
    this->PickCenterToolbar->Delete();
    this->PickCenterToolbar = NULL;
    }

//   if (this->FlySpeedLabel)
//     {
//     this->FlySpeedLabel->Delete();
//     this->FlySpeedLabel = NULL;
//     }
  
//   if (this->FlySpeedScale)
//     {
//     this->FlySpeedScale->Delete();
//     this->FlySpeedScale = NULL;
//     }
  
//   if (this->FlySpeedToolbar)
//     {
//     this->FlySpeedToolbar->Delete();
//     this->FlySpeedToolbar = NULL;
//     }

  if (this->MainView)
    {
    // At exit, save the background colour in the registery.
    this->SaveColor(2, "RenderViewBG", 
                    this->MainView->GetBackgroundColor());
    this->MainView->PrepareForDelete();
    this->MainView->Delete();
    this->MainView = NULL;
    }

  if (this->Application)
    {
    this->Application->SetRegisteryValue(2, "RunTime", "CenterActorVisibility",
                                         "%d", this->CenterActorVisibility);
    }

  if (this->SourceMenu)
    {
    this->SourceMenu->Delete();
    this->SourceMenu = NULL;
    }
  
  if (this->FilterMenu)
    {
    this->FilterMenu->Delete();
    this->FilterMenu = NULL;  
    }
  
  if (this->SelectMenu)
    {
    this->SelectMenu->Delete();
    this->SelectMenu = NULL;
    }
  
  if (this->GlyphMenu)
    {
    this->GlyphMenu->Delete();
    this->GlyphMenu = NULL;
    }
}


//-----------------------------------------------------------------------------
void vtkPVWindow::InitializeMenus(vtkKWApplication* vtkNotUsed(app))
{
  // Add view options.

  // View menu: Show the application settings

  char *rbv = 
    this->GetMenuView()->CreateRadioButtonVariable(
      this->GetMenuView(),"Radio");

  this->GetMenuView()->AddRadioButton(
    VTK_PV_APPSETTINGS_MENU_INDEX, 
    VTK_PV_APPSETTINGS_MENU_LABEL, 
    rbv, 
    this, 
    "ShowApplicationSettingsInterface", 
    1,
    "Display the application settings");
  delete [] rbv;

  this->GetMenuView()->AddSeparator();

  // View menu: shows the notebook for the current source and data object.

  rbv = 
    this->GetMenuView()->CreateRadioButtonVariable(
      this->GetMenuView(),"Radio");

  this->GetMenuView()->AddRadioButton(
    VTK_PV_SOURCE_MENU_INDEX, 
    VTK_PV_SOURCE_MENU_LABEL, 
    rbv, 
    this, 
    "ShowCurrentSourcePropertiesCallback", 
    1,
    "Display the properties of the current data source or filter");
  delete [] rbv;

  // View menu: Shows the animation tool.

  rbv = this->GetMenuView()->CreateRadioButtonVariable(
           this->GetMenuView(),"Radio");

  this->GetMenuView()->AddRadioButton(
    VTK_PV_ANIMATION_MENU_INDEX, 
    VTK_PV_ANIMATION_MENU_LABEL, 
    rbv, 
    this, 
    "ShowAnimationProperties", 
    1,
    "Display the interface for creating animations by varying variables "
    "in a loop");
  delete [] rbv;

  // File menu: 

  // We do not need Close in the file menu since we don't
  // support multiple windows (exit is enough)
  this->MenuFile->DeleteMenuItem("Close");
  // Open a data file. Can support multiple file formats (see Open()).
  this->MenuFile->InsertCommand(0, VTK_PV_OPEN_DATA_MENU_LABEL, this, "OpenCallback",0);
  // Save current data in VTK format.
  this->MenuFile->InsertCommand(1, VTK_PV_SAVE_DATA_MENU_LABEL, this, "WriteData",0);

  // Add advanced file options
  int clidx = this->GetFileMenuIndex();
  this->MenuFile->InsertCommand(clidx++, "Save Batch Script", this,
                                "SaveBatchScript", 7,
                                "Write a script which can run "
                                "in batch by ParaView");

  this->MenuFile->InsertSeparator(clidx++);

  this->MenuFile->InsertCommand(clidx++, "Load Session", this, 
                                "LoadScript", 0,
                                "Restore a trace of actions.");
  this->MenuFile->InsertCommand(clidx++, "Save Session State", this,
                                "SaveState", 7,
                                "Write the current state of ParaView "
                                "in a file.");
  this->MenuFile->InsertCommand(clidx++,
                                "Save Session Trace", this, 
                                "SaveTrace", 3,
                                "Save a trace of every action "
                                "since start up.");
  this->MenuFile->InsertCommand(clidx++,
                                "Import Package", this, 
                                "OpenPackage", 3,
                                "Import modules defined in a ParaView package ");

  /*
  // Open XML package
  this->MenuFile->InsertCommand(clidx++, "Open Package", this, 
                                "OpenPackage", 8,
                                "Open a ParaView package and load the "
                                "contents");
  */

  // Select menu: ParaView specific menus.

  // Create the select menu (for selecting user created and default
  // (i.e. glyphs) data objects/sources)
  this->SelectMenu->SetParent(this->GetMenu());
  this->SelectMenu->Create(this->Application, "-tearoff 0");
  this->Menu->InsertCascade(2, VTK_PV_SELECT_SOURCE_MENU_LABEL, this->SelectMenu, 0);
  
  // Create the menu for selecting the glyphs.  
  this->GlyphMenu->SetParent(this->SelectMenu);
  this->GlyphMenu->Create(this->Application, "-tearoff 0");
  this->SelectMenu->AddCascade("Glyphs", this->GlyphMenu, 0,
                                 "Select one of the glyph sources.");  

  // Create the menu for creating data sources.  
  this->SourceMenu->SetParent(this->GetMenu());
  this->SourceMenu->Create(this->Application, "-tearoff 0");
  this->Menu->InsertCascade(3, VTK_PV_VTK_SOURCES_MENU_LABEL, 
                            this->SourceMenu, 0,
                            "Choose a source from a list of "
                            "VTK sources");  
  
  // Create the menu for creating data sources (filters).  
  this->FilterMenu->SetParent(this->GetMenu());
  this->FilterMenu->Create(this->Application, "-tearoff 0");
  this->Menu->InsertCascade(4, VTK_PV_VTK_FILTERS_MENU_LABEL, 
                            this->FilterMenu, 2,
                            "Choose a filter from a list of "
                            "VTK filters");  
  this->Menu->SetState(VTK_PV_VTK_FILTERS_MENU_LABEL, 
                       vtkKWMenu::Disabled);

  // Window menu:

  this->GetMenuWindow()->AddSeparator();

  this->GetMenuWindow()->InsertCommand(
    4, "Command Prompt", this,
    "DisplayCommandPrompt", 8,
    "Display a prompt to interact with the ParaView engine");

  // Log stuff (not traced)
  this->GetMenuWindow()->InsertCommand(
    5, "Timer Log", this, 
    "ShowTimerLog", 2, 
    "Show log of render events and timing");
              
  // Log stuff (not traced)
  this->GetMenuWindow()->InsertCommand(
    5, "Error Log", this, 
    "ShowErrorLog", 2, 
    "Show log of all errors and warnings");

  // Edit menu

  this->GetMenuEdit()->InsertCommand(5, "Delete All Modules", this, 
                                     "DeleteAllSourcesCallback", 
                                     1, "Delete all modules in ParaView");
}

//-----------------------------------------------------------------------------
void vtkPVWindow::InitializeToolbars(vtkKWApplication *app)
{
  this->InteractorToolbar->SetParent(this->GetToolbarFrame());
  this->InteractorToolbar->Create(app);
  this->InteractorToolbar->Pack("-side left");

  this->Toolbar->SetParent(this->GetToolbarFrame());
  this->Toolbar->Create(app);
  this->Toolbar->ResizableOn();
  this->Toolbar->Pack("-side left");
}

//-----------------------------------------------------------------------------
void vtkPVWindow::InitializeInteractorInterfaces(vtkKWApplication *app)
{
  // Set up the button to reset the camera.
  
  vtkKWPushButton* reset_cam = vtkKWPushButton::New();
  reset_cam->SetParent(this->InteractorToolbar->GetFrame());
  reset_cam->Create(app, "-image PVResetViewButton");
  reset_cam->SetCommand(this, "ResetCameraCallback");
  reset_cam->SetBalloonHelpString("Reset the view to show everything visible.");
  this->InteractorToolbar->AddWidget(reset_cam);
  reset_cam->Delete();

  // set up the interactor styles
  // The interactor styles (selection and events) add no trace entries.
  
  // Fly interactor style

//   this->FlyButton->SetParent(this->InteractorToolbar->GetFrame());
//   this->FlyButton->Create(
//     app, "-indicatoron 0 -highlightthickness 0 -image PVFlyButton -selectimage PVFlyButtonActive");
//   this->FlyButton->SetBalloonHelpString(
//     "Fly View Mode\n   Left Button: Fly toward mouse position.\n   Right Button: Fly backward");
//   this->Script("%s configure -command {%s ChangeInteractorStyle 0}",
//                this->FlyButton->GetWidgetName(), this->GetTclName());
//   this->InteractorToolbar->AddWidget(this->FlyButton);

  // Rotate camera interactor style

  this->RotateCameraButton->SetParent(this->InteractorToolbar->GetFrame());
  this->RotateCameraButton->Create(
    app, "-indicatoron 0 -highlightthickness 0 -image PVRotateViewButton -selectimage PVRotateViewButtonActive");
  this->RotateCameraButton->SetBalloonHelpString(
    "3D Movements Interaction Mode\nThis interaction mode can be configured "
    "from View->3D View Properties->Camera");
  this->Script("%s configure -command {%s ChangeInteractorStyle 1}",
               this->RotateCameraButton->GetWidgetName(), this->GetTclName());
  this->InteractorToolbar->AddWidget(this->RotateCameraButton);
  this->RotateCameraButton->SetState(1);

  // Translate camera interactor style

  this->TranslateCameraButton->SetParent(this->InteractorToolbar->GetFrame());
  this->TranslateCameraButton->Create(
    app, "-indicatoron 0 -highlightthickness 0 -image PVTranslateViewButton -selectimage PVTranslateViewButtonActive");
  this->TranslateCameraButton->SetBalloonHelpString(
    "2D Movements Interaction Mode\nThis mode can be used in conjunction with "
    "the Parallel Projection setting (View->3D View Properties->General) to "
    "interact with 2D objects. This interaction mode can be configured "
    "from View->3D View Properties->Camera");
  this->Script("%s configure -command {%s ChangeInteractorStyle 2}", 
               this->TranslateCameraButton->GetWidgetName(), this->GetTclName());
  this->InteractorToolbar->AddWidget(this->TranslateCameraButton);

  this->MainView->ResetCamera();
}

//-----------------------------------------------------------------------------
// Keep a list of the toolbar buttons so that they can be 
// disabled/enabled in certain situations.
void vtkPVWindow::AddToolbarButton(const char* buttonName, 
                                   const char* imageName, 
                                   const char* fileName,
                                   const char* command,
                                   const char* balloonHelp)
{
  if (fileName)
    {
    this->Script("image create photo %s -file {%s}", imageName, fileName);
    }
  vtkKWPushButton* button = vtkKWPushButton::New();
  button->SetParent(this->Toolbar->GetFrame());
  ostrstream opts;
  opts << "-image " << imageName << ends;
  button->Create(this->GetPVApplication(), opts.str());
  opts.rdbuf()->freeze(0);
  button->SetCommand(this, command);
  if (balloonHelp)
    {
    button->SetBalloonHelpString(balloonHelp);
    }
  this->ToolbarButtons->SetItem(buttonName, button);
  this->Toolbar->AddWidget(button);
  button->Delete();
}


//-----------------------------------------------------------------------------
void vtkPVWindow::SetInteractor(vtkPVGenericRenderWindowInteractor *interactor)
{
  // Do not bother referencing.
  this->Interactor = interactor;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::Create(vtkKWApplication *app, char* vtkNotUsed(args))
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("vtkPVWindow::Create needs a vtkPVApplication.");
    return;
    }

  pvApp->SetBalloonHelpDelay(1);

  // Make sure the widget is name appropriately: paraview instead of a number.
  // On X11, the window name is the same as the widget name.
  this->WidgetName = vtkString::Duplicate(".paraview");
  // Invoke super method first.
  this->vtkKWWindow::Create(pvApp,"");

  // Hide the main window until after all user interface is initialized.
  this->Script( "wm withdraw %s", this->GetWidgetName());

  // If we have a user interface manager, it's time to create it

  vtkKWUserInterfaceManager *uim = this->GetUserInterfaceManager();
  if (uim && !uim->IsCreated())
    {
    uim->Create(app);
    }

  // Allow the user to interactively resize the properties parent.
  // The left panel size (Frame1) is restored by vtkKWWindow
  this->MiddleFrame->SetSeparatorSize(5);
  this->MiddleFrame->SetFrame1MinimumSize(200);
  this->MiddleFrame->SetFrame2MinimumSize(200);

  // Put the version in the status bar.
  char version[128];
  sprintf(version,"Version %d.%d", this->GetPVApplication()->GetMajorVersion(),
          this->GetPVApplication()->GetMinorVersion());
  this->SetStatusText(version);

  // Update status image (and gauge height to match)
  
  this->UpdateStatusImage();
  this->ProgressGauge->SetHeight(
    vtkKWTkUtilities::GetPhotoHeight(this->Application->GetMainInterp(), 
                                     this->StatusImageName) - 4);

  // Init menus

  if (pvApp->GetShowSplashScreen())
    {
    pvApp->GetSplashScreen()->SetProgressMessage("Creating UI (menus)...");
    }
  this->InitializeMenus(app);

  // Init toolbars

  if (pvApp->GetShowSplashScreen())
    {
    pvApp->GetSplashScreen()->SetProgressMessage("Creating UI (toolbars)...");
    }
  this->InitializeToolbars(app);

  // Interface for the preferences.

  // Create the main view.
  if (pvApp->GetShowSplashScreen())
    {
    pvApp->GetSplashScreen()->SetProgressMessage("Creating UI (main view)...");
    }
  this->CreateMainView(pvApp);

  if (pvApp->GetShowSplashScreen())
    {
    pvApp->GetSplashScreen()->SetProgressMessage("Creating UI (interactors)...");
    }
  this->InitializeInteractorInterfaces(app);


  // Creating the center of rotation actor here because we have
  // the application here.
  // We do not need to broadcast.  We need Send to root method.
  // Not local for client server option.
  this->SetCenterSourceTclName("pvCenterSource");
  pvApp->BroadcastScript("vtkAxes %s", this->CenterSourceTclName);
  pvApp->BroadcastScript("%s SymmetricOn", this->CenterSourceTclName);
  pvApp->BroadcastScript("%s ComputeNormalsOff", this->CenterSourceTclName);
  this->SetCenterMapperTclName("pvMapperSource");
  pvApp->BroadcastScript("vtkPolyDataMapper %s", this->CenterMapperTclName);
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", this->CenterMapperTclName,
                         this->CenterSourceTclName);
  this->SetCenterActorTclName("pvActorSource");
  pvApp->BroadcastScript("vtkActor %s", this->CenterActorTclName);
  pvApp->BroadcastScript("%s SetMapper %s", this->CenterActorTclName,
                         this->CenterMapperTclName);
  pvApp->BroadcastScript("%s VisibilityOff", this->CenterActorTclName);
  // -------------------------

  this->PickCenterToolbar->SetParent(this->GetToolbarFrame());
  this->PickCenterToolbar->Create(app);
  
  this->PickCenterButton->SetParent(this->PickCenterToolbar->GetFrame());
  this->PickCenterButton->Create(app, "-image PVPickCenterButton");
  this->PickCenterButton->SetCommand(this, "ChangeInteractorStyle 4");
  this->PickCenterButton->SetBalloonHelpString(
    "Pick the center of rotation of the current data set.");
  this->PickCenterToolbar->AddWidget(this->PickCenterButton);
  
  this->ResetCenterButton->SetParent(this->PickCenterToolbar->GetFrame());
  this->ResetCenterButton->Create(app, "-image PVResetCenterButton");
  this->ResetCenterButton->SetCommand(this, "ResetCenterCallback");
  this->ResetCenterButton->SetBalloonHelpString(
    "Reset the center of rotation to the center of the current data set.");
  this->PickCenterToolbar->AddWidget(this->ResetCenterButton);

  this->HideCenterButton->SetParent(this->PickCenterToolbar->GetFrame());
  this->HideCenterButton->Create(app, "-image PVHideCenterButton");
  this->HideCenterButton->SetCommand(this, "ToggleCenterActorCallback");
  this->HideCenterButton->SetBalloonHelpString(
    "Hide the center of rotation to the center of the current data set.");
  this->PickCenterToolbar->AddWidget(this->HideCenterButton);
  
  this->CenterEntryOpenCloseButton->SetParent(
    this->PickCenterToolbar->GetFrame());
  this->CenterEntryOpenCloseButton->Create(app, "-image PVEditCenterButtonOpen");
  this->CenterEntryOpenCloseButton->SetBalloonHelpString(
    "Edit the center of rotation xyz coordinates.");
  this->CenterEntryOpenCloseButton->SetCommand(this, "CenterEntryOpenCallback");
  this->PickCenterToolbar->AddWidget(this->CenterEntryOpenCloseButton);
  
  this->CenterEntryFrame->SetParent(this->PickCenterToolbar->GetFrame());
  this->CenterEntryFrame->Create(app, "frame", "");
  
  this->CenterXLabel->SetParent(this->CenterEntryFrame);
  this->CenterXLabel->Create(app, "");
  this->CenterXLabel->SetLabel("X");
  
  this->CenterXEntry->SetParent(this->CenterEntryFrame);
  this->CenterXEntry->Create(app, "-width 7");
  this->Script("bind %s <KeyPress-Return> {%s CenterEntryCallback}",
               this->CenterXEntry->GetWidgetName(), this->GetTclName());
  //this->CenterXEntry->SetValue(this->CameraStyle3D->GetCenter()[0], 3);
  this->CenterXEntry->SetValue(0.0, 3);
  
  this->CenterYLabel->SetParent(this->CenterEntryFrame);
  this->CenterYLabel->Create(app, "");
  this->CenterYLabel->SetLabel("Y");
  
  this->CenterYEntry->SetParent(this->CenterEntryFrame);
  this->CenterYEntry->Create(app, "-width 7");
  this->Script("bind %s <KeyPress-Return> {%s CenterEntryCallback}",
               this->CenterYEntry->GetWidgetName(), this->GetTclName());
  //this->CenterYEntry->SetValue(this->CameraStyle3D->GetCenter()[1], 3);
  this->CenterYEntry->SetValue(0.0, 3);

  this->CenterZLabel->SetParent(this->CenterEntryFrame);
  this->CenterZLabel->Create(app, "");
  this->CenterZLabel->SetLabel("Z");
  
  this->CenterZEntry->SetParent(this->CenterEntryFrame);
  this->CenterZEntry->Create(app, "-width 7");
  this->Script("bind %s <KeyPress-Return> {%s CenterEntryCallback}",
               this->CenterZEntry->GetWidgetName(), this->GetTclName());
  //this->CenterZEntry->SetValue(this->CameraStyle3D->GetCenter()[2], 3);
  this->CenterZEntry->SetValue(0.0, 3);

  this->Script("pack %s %s %s %s %s %s -side left",
               this->CenterXLabel->GetWidgetName(),
               this->CenterXEntry->GetWidgetName(),
               this->CenterYLabel->GetWidgetName(),
               this->CenterYEntry->GetWidgetName(),
               this->CenterZLabel->GetWidgetName(),
               this->CenterZEntry->GetWidgetName());

  if (pvApp->GetRegisteryValue(2, "RunTime", "CenterActorVisibility", 0))
    {
    if (
      (this->CenterActorVisibility = 
       pvApp->GetIntRegisteryValue(2, "RunTime", "CenterActorVisibility")))
      {
      this->ShowCenterActor();
      }
    else
      {
      this->HideCenterActor();
      }
    }
  
  pvApp->BroadcastScript("%s AddActor %s", pvApp->GetRenderModule()->GetRendererTclName(),
                         this->CenterActorTclName);

  // Create a dummy interactor on the satellites so they han have 3d widgets.
  pvApp->BroadcastScript("vtkPVGenericRenderWindowInteractor pvRenderWindowInteractor");
  pvApp->BroadcastScript("pvRenderWindowInteractor SetRenderWindow %s", 
                         pvApp->GetRenderModule()->GetRenderWindowTclName());
  pvApp->BroadcastScript("pvRenderWindowInteractor SetInteractorStyle {}"); 
  this->SetInteractorTclName("pvRenderWindowInteractor");  
  this->Script("%s SetInteractor pvRenderWindowInteractor", this->GetTclName());

  // Only on client/proc0 (Only they render directly).
  this->Script("%s SetPVRenderView %s", this->InteractorTclName,
               this->MainView->GetTclName());
  this->ChangeInteractorStyle(1);

  // Configure the window, i.e. setup the interactors
  // We need this update or the window size will be invalid.
  // This fixes the small initial window bug.
  this->Script("update");
  int *windowSize = this->MainView->GetRenderWindowSize();
  this->Configure(windowSize[0], windowSize[1]);
 
  // set up bindings for the interactor  
  const char *wname = this->MainView->GetVTKWidget()->GetWidgetName();
  const char *tname = this->GetTclName();
  this->Script("bind %s <Motion> {%s MouseAction 2 0 %%x %%y 0 0}", wname, tname);
  this->Script("bind %s <B1-Motion> {%s MouseAction 2 1 %%x %%y 0 0}", wname, tname);
  this->Script("bind %s <B2-Motion> {%s MouseAction 2 2 %%x %%y 0 0}", wname, tname);
  this->Script("bind %s <B3-Motion> {%s MouseAction 2 3 %%x %%y 0 0}", wname, tname);
  this->Script("bind %s <Shift-B1-Motion> {%s MouseAction 2 1 %%x %%y 1 0}", 
               wname, tname);
  this->Script("bind %s <Shift-B2-Motion> {%s MouseAction 2 2 %%x %%y 1 0}", 
               wname, tname);
  this->Script("bind %s <Shift-B3-Motion> {%s MouseAction 2 3 %%x %%y 1 0}", 
               wname, tname);
  this->Script("bind %s <Control-B1-Motion> {%s MouseAction 2 1 %%x %%y 0 1}", 
               wname, tname);
  this->Script("bind %s <Control-B2-Motion> {%s MouseAction 2 2 %%x %%y 0 1}", 
               wname, tname);
  this->Script("bind %s <Control-B3-Motion> {%s MouseAction 2 3 %%x %%y 0 1}", 
               wname, tname);
  
  this->Script("bind %s <Any-ButtonPress> {%s MouseAction 0 %%b %%x %%y 0 0}",
               wname, tname);
  this->Script("bind %s <Shift-Any-ButtonPress> {%s MouseAction 0 %%b %%x %%y 1 0}",
               wname, tname);
  this->Script("bind %s <Control-Any-ButtonPress> {%s MouseAction 0 %%b %%x %%y 0 1}",
               wname, tname);
  this->Script("bind %s <Any-ButtonRelease> {%s MouseAction 1 %%b %%x %%y 0 0}",
               wname, tname);
  this->Script("bind %s <Shift-Any-ButtonRelease> {%s MouseAction 1 %%b %%x %%y 1 0}",
               wname, tname);
  this->Script("bind %s <Control-Any-ButtonRelease> {%s MouseAction 1 %%b %%x %%y 0 1}",
               wname, tname);
  //this->Script("bind %s <Motion> {%s MouseAction 2 0 %%x %%y 0 0}",
  //             wname, tname);
  this->Script("bind %s <Configure> {%s Configure %%w %%h}",
               wname, tname);
  
  // Interface for the animation tool.
  this->AnimationInterface->SetWindow(this);
  this->AnimationInterface->SetView(this->GetMainView());
  this->AnimationInterface->SetParent(this->GetPropertiesParent());
  this->AnimationInterface->Create(app, "-relief flat");

  this->AddRecentFilesToMenu("Exit",this);

  // File->Open Data File is disabled unless reader modules are loaded.
  // AddFileType() enables this entry.
  this->MenuFile->SetState(VTK_PV_OPEN_DATA_MENU_LABEL, vtkKWMenu::Disabled);

  if (this->InitializeDefaultInterfaces)
    {
    vtkPVSourceCollection* sources = vtkPVSourceCollection::New();
    this->SourceLists->SetItem("GlyphSources", sources);
    sources->Delete();

    // We need an application before we can read the interface.
    this->ReadSourceInterfaces();
    
    // Create the extract grid button
    vtkPVSource* extract = 0;
    if (this->Prototypes->GetItem("ExtractGrid", extract) == VTK_OK)
      {
      extract->SetToolbarModule(1);
      this->AddToolbarButton("ExtractGrid", "PVExtractGridButton", 0,
                             "ExtractGridCallback",
                             "Extract a subgrid from a structured data set.");
      }

    vtkPVSource *pvs=0;
    // Create the sources that can be used for glyphing.
    // ===== Arrow
    pvs = this->CreatePVSource("ArrowSource", "GlyphSources", 0, 0);
    pvs->IsPermanentOn();
    pvs->HideDisplayPageOn();
    pvs->HideInformationPageOn();
    pvs->Accept(1);
    pvs->SetTraceReferenceObject(this);
    {
    ostrstream s;
    s << "GetPVSource GlyphSources " << pvs->GetName() << ends;
    pvs->SetTraceReferenceCommand(s.str());
    s.rdbuf()->freeze(0);
    }
    
    // ===== Cone
    pvs = this->CreatePVSource("ConeSource", "GlyphSources", 0, 0);
    pvs->IsPermanentOn();
    pvs->HideDisplayPageOn();
    pvs->HideInformationPageOn();
    pvs->Accept(1);
    pvs->SetTraceReferenceObject(this);
    {
    ostrstream s;
    s << "GetPVSource GlyphSources " << pvs->GetName() << ends;
    pvs->SetTraceReferenceCommand(s.str());
    s.rdbuf()->freeze(0);
    }
    
    // ===== Sphere
    pvs = this->CreatePVSource("SphereSource", "GlyphSources", 0, 0);
    pvs->IsPermanentOn();
    pvs->HideDisplayPageOn();
    pvs->HideInformationPageOn();
    pvs->Accept(1);
    pvs->SetTraceReferenceObject(this);
    {
    ostrstream s;
    s << "GetPVSource GlyphSources " << pvs->GetName() << ends;
    pvs->SetTraceReferenceCommand(s.str());
    s.rdbuf()->freeze(0);
    }

    // We need an initial current source, why not use sphere ? 
    this->SetCurrentPVSource(pvs);

    }
  else
    {
    char* str = getenv("PV_INTERFACE_PATH");
    if (str)
      {
      this->ReadSourceInterfacesFromDirectory(str);
      }
    }

  // The filter buttons are initially disabled.
  this->DisableToolbarButtons();

  // Show glyph sources in menu.
  this->UpdateSelectMenu();
  // Show the sources (in Advanced).
  this->UpdateSourceMenu();

  // Update preferences
  if (pvApp->GetShowSplashScreen())
    {
    pvApp->GetSplashScreen()->SetProgressMessage("Creating UI (preferences)...");
    }

  // Update toolbar aspect
  this->UpdateToolbarAspect();

  // Make the 3D View Settings the current one.
  this->Script("%s invoke \"%s\"", 
               this->GetMenuView()->GetWidgetName(),
               VTK_PV_VIEW_MENU_LABEL);

  this->Script( "wm deiconify %s", this->GetWidgetName());

  this->Script("wm protocol %s WM_DELETE_WINDOW { %s Exit }",
               this->GetWidgetName(), this->GetTclName());

  if ( this->MainView )
    {
    this->MainView->SetupCameraManipulators();     
    }

}

//-----------------------------------------------------------------------------
// The prototypes for source and filter modules. Instances are 
// created by calling CloneAndInitialize() on these.
void vtkPVWindow::AddPrototype(const char* name, vtkPVSource* proto)
{
  this->Prototypes->SetItem(name, proto);
}

//-----------------------------------------------------------------------------
// Keep a list of all loaded packages (Tcl libraries) so that
// they can be written out when writing Tcl scripts.
void vtkPVWindow::AddPackageName(const char* name)
{
  this->PackageNames->AppendItem(name);
}

void vtkPVWindow::CenterEntryOpenCallback()
{
  this->Script("%s configure -image PVEditCenterButtonClose", 
               this->CenterEntryOpenCloseButton->GetWidgetName() );
  this->CenterEntryOpenCloseButton->SetBalloonHelpString(
    "Finish editing the center of rotation xyz coordinates.");

  this->CenterEntryOpenCloseButton->SetCommand(
    this, "CenterEntryCloseCallback");

  this->PickCenterToolbar->InsertWidget(this->CenterEntryOpenCloseButton,
                                        this->CenterEntryFrame);
}

void vtkPVWindow::CenterEntryCloseCallback()
{
  this->Script("%s configure -image PVEditCenterButtonOpen", 
               this->CenterEntryOpenCloseButton->GetWidgetName() );
  this->CenterEntryOpenCloseButton->SetBalloonHelpString(
    "Edit the center of rotation xyz coordinates.");

  this->CenterEntryOpenCloseButton->SetCommand(
    this, "CenterEntryOpenCallback");

  this->PickCenterToolbar->RemoveWidget(this->CenterEntryFrame);
}


//-----------------------------------------------------------------------------
void vtkPVWindow::CenterEntryCallback()
{
  float x = this->CenterXEntry->GetValueAsFloat();
  float y = this->CenterYEntry->GetValueAsFloat();
  float z = this->CenterZEntry->GetValueAsFloat();
  this->SetCenterOfRotation(x, y, z);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetCenterOfRotation(float x, float y, float z)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->CenterXEntry->SetValue(x, 4);
  this->CenterYEntry->SetValue(y, 4);
  this->CenterZEntry->SetValue(z, 4);
  this->CameraStyle3D->SetCenterOfRotation(x, y, z);
  pvApp->BroadcastScript("%s SetPosition %f %f %f", 
                         this->CenterActorTclName,
                         x, y, z);
  this->MainView->EventuallyRender();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::HideCenterActor()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  this->Script("%s configure -image PVShowCenterButton", 
               this->HideCenterButton->GetWidgetName() );
  this->HideCenterButton->SetBalloonHelpString(
    "Show the center of rotation to the center of the current data set.");
  pvApp->BroadcastScript("%s VisibilityOff", this->CenterActorTclName);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ShowCenterActor()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->CenterActorVisibility)
    {
    this->Script("%s configure -image PVHideCenterButton", 
                 this->HideCenterButton->GetWidgetName() );
    this->HideCenterButton->SetBalloonHelpString(
      "Hide the center of rotation to the center of the current data set.");
    pvApp->BroadcastScript("%s VisibilityOn", this->CenterActorTclName);
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ToggleCenterActorCallback()
{
  if (this->CenterActorVisibility)
    {
    this->CenterActorVisibility=0;
    this->HideCenterActor();
    }
  else
    {
    this->CenterActorVisibility=1;
    this->ShowCenterActor();
    }

  this->AddTraceEntry("$kw(%s) ToggleCenterActorCallback", this->GetTclName());

  this->MainView->EventuallyRender();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ResetCenterCallback()
{
  if ( ! this->CurrentPVSource)
    {
    return;
    }
  
  double bounds[6];
  this->CurrentPVSource->GetDataInformation()->GetBounds(bounds);

  float center[3];
  center[0] = (bounds[0]+bounds[1])/2.0;
  center[1] = (bounds[2]+bounds[3])/2.0;
  center[2] = (bounds[4]+bounds[5])/2.0;

  this->SetCenterOfRotation(center[0], center[1], center[2]);
  this->CenterXEntry->SetValue(center[0], 3);
  this->CenterYEntry->SetValue(center[1], 3);
  this->CenterZEntry->SetValue(center[2], 3);
  this->ResizeCenterActor();
  this->MainView->EventuallyRender();
}

//-----------------------------------------------------------------------------
// void vtkPVWindow::FlySpeedScaleCallback()
// {
//   this->FlyStyle->SetSpeed(this->FlySpeedScale->GetValue());
// }

//-----------------------------------------------------------------------------
void vtkPVWindow::ResizeCenterActor()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int first = 1;
  double bounds[6];
  double tmp[6];
  vtkPVSource *pvs;
 
  // Loop though all sources/Data objects and compute total bounds.
  vtkPVSourceCollection* col = this->GetSourceList("Sources");
  if (col == NULL)
    {
    return;
    }
  vtkCollectionIterator *it = col->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>( it->GetObject() );
    if (pvs->GetVisibility())
      {
      if (first)
        {
        pvs->GetDataInformation()->GetBounds(bounds);
        first = 0;
        }
      else
        {
        pvs->GetDataInformation()->GetBounds(tmp);
        if (tmp[0] < bounds[0]) {bounds[0] = tmp[0];} 
        if (tmp[1] > bounds[1]) {bounds[1] = tmp[1];} 
        if (tmp[2] < bounds[2]) {bounds[2] = tmp[2];} 
        if (tmp[3] > bounds[3]) {bounds[3] = tmp[3];} 
        if (tmp[4] < bounds[4]) {bounds[4] = tmp[4];} 
        if (tmp[5] > bounds[5]) {bounds[5] = tmp[5];} 
        }
      }
    it->GoToNextItem();
    }

  it->Delete();
  if ((! first) && (bounds[0] <= bounds[1]) && 
      (bounds[2] <= bounds[3]) && (bounds[4] <= bounds[5]))
    {
    pvApp->BroadcastScript("%s SetScale %f %f %f", this->CenterActorTclName,
                           0.25 * (bounds[1]-bounds[0]),
                           0.25 * (bounds[3]-bounds[2]),
                           0.25 * (bounds[5]-bounds[4]));
    }
  else
    {
    pvApp->BroadcastScript("%s SetScale 1 1 1", this->CenterActorTclName);
    this->MainView->ResetCamera();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ChangeInteractorStyle(int index)
{
  this->PickCenterToolbar->Unpack();
  //this->FlySpeedToolbar->Unpack();;
  
  switch (index)
    {
    case 1:
      //this->FlyButton->SetState(0);
      this->TranslateCameraButton->SetState(0);
      // Camera styles are not duplicated on satellites.
      // Cameras are synchronized before each render.
      this->Interactor->SetInteractorStyle(this->CameraStyle3D);
      this->PickCenterToolbar->Pack("-side left");
      this->ResizeCenterActor();
      this->ShowCenterActor();
      break;
    case 2:
      //this->FlyButton->SetState(0);
      this->RotateCameraButton->SetState(0);
      this->Interactor->SetInteractorStyle(this->CameraStyle2D);
      this->HideCenterActor();
      break;
    case 3:
      vtkErrorMacro("Trackball no longer suported.");
      break;
    case 4:
      this->Interactor->SetInteractorStyle(this->CenterOfRotationStyle);
      this->HideCenterActor();
      break;
    }
  this->MainView->EventuallyRender();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::MouseAction(int action,int button, 
                              int x,int y, int shift,int control)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if ( action == 0 )
    {
    if (button == 1)
      {
      // Send the same event to the satellite to synchronize the 3D widgets.
      // Maybe I should itegrate this into the PV interactor.
      pvApp->BroadcastScript("pvRenderWindowInteractor SatelliteLeftPress %d %d %d %d", 
                             x, y, control, shift);
      }
    else if (button == 2)
      {
      // Send the same event to the satellite to synchronize the 3D widgets.
      // Maybe I should itegrate this into the PV interactor.
      pvApp->BroadcastScript("pvRenderWindowInteractor SatelliteMiddlePress %d %d %d %d", 
                             x, y, control, shift);
      }
    else if (button == 3)
      {
      // Send the same event to the satellite to synchronize the 3D widgets.
      // Maybe I should itegrate this into the PV interactor.
      pvApp->BroadcastScript("pvRenderWindowInteractor SatelliteRightPress %d %d %d %d", 
                             x, y, control, shift);
      }    
    }
  else if ( action == 1 )
    {
    if (button == 1)
      {
      // Send the same event to the satellite to synchronize the 3D widgets.
      // Maybe I should itegrate this into the PV interactor.
      pvApp->BroadcastScript("pvRenderWindowInteractor SatelliteLeftRelease %d %d %d %d", 
                             x, y, control, shift);
      }
    else if (button == 2)
      {
      // Send the same event to the satellite to synchronize the 3D widgets.
      // Maybe I should itegrate this into the PV interactor.
      pvApp->BroadcastScript("pvRenderWindowInteractor SatelliteMiddleRelease %d %d %d %d", 
                             x, y, control, shift);
      }
    else if (button == 3)
      {
      // Send the same event to the satellite to synchronize the 3D widgets.
      // Maybe I should itegrate this into the PV interactor.
      pvApp->BroadcastScript("pvRenderWindowInteractor SatelliteRightRelease %d %d %d %d", 
                             x, y, control, shift);
      }    

    vtkCamera* cam = this->MainView->GetRenderer()->GetActiveCamera();
    //float* parallelScale = cam->GetParallelScale();
    double* position      = cam->GetPosition();
    double* focalPoint    = cam->GetFocalPoint();
    double* viewUp        = cam->GetViewUp();

    this->AddTraceEntry(
      "$kw(%s) SetCameraState "
      "%.3lf %.3lf %.3lf  %.3lf %.3lf %.3lf  %.3lf %.3lf %.3lf", 
      this->MainView->GetTclName(), 
      position[0], position[1], position[2], 
      focalPoint[0], focalPoint[1], focalPoint[2], 
      viewUp[0], viewUp[1], viewUp[2]);
    }
  else
    {
    // Send the same event to the satellite to synchronize the 3D widgets.
    // Maybe I should itegrate this into the PV interactor.
    pvApp->BroadcastScript("pvRenderWindowInteractor SatelliteMove %d %d", x, y);
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::Configure(int width, int height)
{
  this->MainView->Configured();
  this->Interactor->UpdateSize(width, height);
  this->Interactor->ConfigureEvent();
}

//-----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::GetPVSource(const char* listname, char* sourcename)
{
  vtkPVSourceCollection* col = this->GetSourceList(listname);
  if (col)
    {    
    vtkPVSource *pvs;
    vtkCollectionIterator *it = col->NewIterator();
    it->InitTraversal();
    while ( !it->IsDoneWithTraversal() )
      {
      pvs = static_cast<vtkPVSource*>( it->GetObject() );
      if (strcmp(sourcename, pvs->GetName()) == 0)
        {
        it->Delete();
        return pvs;
        }
      it->GoToNextItem();
      }
    it->Delete();
    }
  return 0;
}


//-----------------------------------------------------------------------------
void vtkPVWindow::CreateMainView(vtkPVApplication *pvApp)
{
  vtkPVRenderView *view;
  
  view = vtkPVRenderView::New();
  view->CreateRenderObjects(pvApp);
  this->MainView = view;
  this->MainView->SetParent(this->ViewFrame);
  this->MainView->SetPropertiesParent(this->GetPropertiesParent());
  this->AddView(this->MainView);
  this->MainView->Create(this->Application,"-width 200 -height 200");
  this->MainView->MakeSelected();
  this->MainView->ShowViewProperties();
  this->MainView->SetupBindings();
  this->MainView->AddBindings(); // additional bindings in PV not in KW
  

  vtkPVInteractorStyleControl *iscontrol3D = view->GetManipulatorControl3D();
  iscontrol3D->SetManipulatorCollection(
    this->CameraStyle3D->GetCameraManipulators());
  vtkPVInteractorStyleControl *iscontrol2D = view->GetManipulatorControl2D();
  iscontrol2D->SetManipulatorCollection(
    this->CameraStyle2D->GetCameraManipulators());
  
  float rgb[3];
  this->RetrieveColor(2, "RenderViewBG", rgb); 
  if (rgb[0] == -1)
    {
    rgb[0] = 0.33;
    rgb[1] = 0.35;
    rgb[2] = 0.43;
    }
  this->MainView->SetBackgroundColor(rgb);
  this->Script( "pack %s -expand yes -fill both", 
                this->MainView->GetWidgetName());  

  this->MenuHelp->AddSeparator();
  this->MenuHelp->AddCommand("Play Demo", this, "PlayDemo", 0);
}


//-----------------------------------------------------------------------------
void vtkPVWindow::PlayDemo()
{
  this->PlayDemo(0);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::PlayDemo(int fromDashboard)
{
  const char* demoDataPath;
  const char* demoScriptPath;
  vtkPVApplication* pvApp = this->GetPVApplication();


  this->Script("catch {unset pvDemoFromDashboard}");
  if (fromDashboard)
    {
    this->Script("set pvDemoFromDashboard 1");
    }

  // Server path
  pvApp->GetProcessModule()->RootScript("$Application GetDemoPath");
  demoDataPath = pvApp->GetProcessModule()->GetRootResult();
  // Client path
  demoScriptPath = pvApp->GetDemoPath();

  if (demoDataPath && demoScriptPath)
    {
    char temp1[1024];
    sprintf(temp1, "%s/Demo1.pvs", 
            demoScriptPath);

    this->Script("set DemoDir {%s}", demoDataPath);
    this->LoadScript(temp1);
    }
  else
    {
    if (this->UseMessageDialog)
      {
      vtkKWMessageDialog::PopupMessage(
        this->Application, this,
        "Warning", 
        "Could not find Demo1.pvs in the installation or\n"
        "build directory. Please make sure that ParaView\n"
        "is installed properly.",
        vtkKWMessageDialog::WarningIcon);
      }
    else
      {
      vtkWarningMacro("Could not find Demo1.pvs in the installation or "
                      "build directory. Please make sure that ParaView "
                      "is installed properly.");
      }
    }
}

//-----------------------------------------------------------------------------
// Try to open a file for reading, return error on failure.
int vtkPVWindow::CheckIfFileIsReadable(const char* fileName)
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->RootScript("file readable {%s}", fileName);
  const char* result = pm->GetRootResult();
  if(result && strcmp("1", result) == 0)
    {
    return VTK_OK;
    }
  return VTK_ERROR;
}


//-----------------------------------------------------------------------------
// Prompts the user for a filename and calls Open().
void vtkPVWindow::OpenCallback()
{
  char *openFileName = NULL;

  if (!this->FileExtensions)
    {
    const char* error = "There are no reader modules "
      "defined, please start ParaView with "
      "the default interface or load reader "
      "modules.";
    if (this->UseMessageDialog)
      {
      vtkKWMessageDialog::PopupMessage(this->Application, this,
                                       "Error",  error,
                                       vtkKWMessageDialog::ErrorIcon);
      }
    else
      {
      vtkErrorMacro(<<error);
      }
    return;
    }

  ostrstream str;
  str << "{{ParaView Files} {" << this->FileExtensions << "}} "
      << this->FileDescriptions << " {{All Files} {*}}" << ends;

  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkKWLoadSaveDialog* loadDialog = pm->NewLoadSaveDialog();
  this->RetrieveLastPath(loadDialog, "OpenPath");
  loadDialog->Create(this->Application,0);
  loadDialog->SetParent(this);
  loadDialog->SetTitle("Open ParaView File");
  loadDialog->SetDefaultExtension(".vtp");
  loadDialog->SetFileTypes(str.str());
  str.rdbuf()->freeze(0);  
  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( loadDialog->Invoke() )
    {
    openFileName = vtkString::Duplicate(loadDialog->GetFileName());
    }
  this->SetEnabled(enabled);

  // Store last path
  if ( openFileName && vtkString::Length(openFileName) > 0 )
    {
    if  (this->Open(openFileName, 1) == VTK_OK)
      {
      this->SaveLastPath(loadDialog, "OpenPath");
      }
    }
  
  loadDialog->Delete();
  delete [] openFileName;
}

vtkKWUserInterfaceManager* vtkPVWindow::GetUserInterfaceManager()
{
  if (!this->UserInterfaceManager)
    {
    this->UserInterfaceManager = vtkKWUserInterfaceNotebookManager::New();
    this->UserInterfaceManager->SetNotebook(this->Notebook);
    }
  return this->UserInterfaceManager;
}

vtkKWApplicationSettingsInterface* vtkPVWindow::GetApplicationSettingsInterface()
{
  // If not created, create the application settings interface, connect it
  // to the current window, and manage it with the current interface manager.

  if (!this->ApplicationSettingsInterface)
    {
    this->ApplicationSettingsInterface= vtkPVApplicationSettingsInterface::New();
    this->ApplicationSettingsInterface->SetWindow(this);

    vtkKWUserInterfaceManager *uim = this->GetUserInterfaceManager();
    if (uim)
      {
      this->ApplicationSettingsInterface->SetUserInterfaceManager(uim);
      }
    }
  return this->ApplicationSettingsInterface;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetShowSourcesLongHelp(int v)
{
  vtkPVApplicationSettingsInterface *asi = 
    vtkPVApplicationSettingsInterface::SafeDownCast(
      this->GetApplicationSettingsInterface());
  if (asi)
    {
    asi->Update();
    }

  if (this->ShowSourcesLongHelp == v)
    {
    return;
    }
  this->ShowSourcesLongHelp = v;
  this->Modified();

  // Update the properties of all the sources previously created
  // so that the Description label can be removed/brought back.
  
  vtkPVSource *pvs;
  vtkPVSourceCollection *col = this->GetSourceList("Sources");
  vtkCollectionIterator *cit = col->NewIterator();
  cit->InitTraversal();
  while (!cit->IsDoneWithTraversal())
    {
    pvs = static_cast<vtkPVSource*>(cit->GetObject()); 
    pvs->UpdateProperties();
    cit->GoToNextItem();
    }
  cit->Delete();
}

//-----------------------------------------------------------------------------
int vtkPVWindow::Open(char *openFileName, int store)
{
  if (this->CheckIfFileIsReadable(openFileName) != VTK_OK)
    {
    ostrstream error;
    error << "Can not open file " << openFileName << " for reading." << ends;
    if (this->UseMessageDialog)
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetApplication(), this, "Open Error", error.str(), 
        vtkKWMessageDialog::ErrorIcon | vtkKWMessageDialog::Beep);
      }
    else
      {
      vtkErrorMacro(<<error.str());
      }
    error.rdbuf()->freeze(0);
    return VTK_ERROR;
    }


  // Ask each reader module if it can read the file. This first
  // one which says OK gets to read the file.
  vtkLinkedListIterator<vtkPVReaderModule*>* it = 
    this->ReaderList->NewIterator();
  while(!it->IsDoneWithTraversal())
    {
    vtkPVReaderModule* rm = 0;
    int retVal = it->GetData(rm);
    if (retVal == VTK_OK && rm->CanReadFile(openFileName) &&
        this->OpenWithReader(openFileName, rm) == VTK_OK )
      {
      if ( store )
        {
        ostrstream str;
        str << "OpenCustom \"" << rm->GetModuleName() << "\"" <<ends;
        this->AddRecentFile(NULL, openFileName, this, str.str());
        str.rdbuf()->freeze(0);
        }
      it->Delete();
      return VTK_OK;
      }
    it->GoToNextItem();
    }
  it->Delete();

  
  ostrstream error;
  error << "Could not find an appropriate reader for file "
        << openFileName << ". Would you like to manually select "
        << "the reader for this file?" << ends;
  error.rdbuf()->freeze(0);     
  if (this->UseMessageDialog)
    {
    if ( vtkKWMessageDialog::PopupOkCancel(this->Application, this,
                                           "Open Error",  error.str(),
                                           vtkKWMessageDialog::ErrorIcon |
                                           vtkKWMessageDialog::CancelDefault |
                                           vtkKWMessageDialog::Beep ) )
      {
      vtkPVSelectCustomReader* dialog = vtkPVSelectCustomReader::New();
      vtkPVReaderModule* reader = dialog->SelectReader(this, openFileName);
      if ( !reader || this->OpenWithReader(openFileName, reader) != VTK_OK )
        {
        ostrstream error;
        error << "Can not open file " << openFileName << " for reading." << ends;
        if (this->UseMessageDialog)
          {
          vtkKWMessageDialog::PopupMessage(
            this->GetApplication(), this, "Open Error", error.str(), 
            vtkKWMessageDialog::ErrorIcon | vtkKWMessageDialog::Beep);
          }
        else
          {
          vtkErrorMacro(<<error);
          }
        }
      else if ( store )
        {
        ostrstream str;
        str << "OpenCustom \"" << reader->GetModuleName() << "\"" <<ends;
        this->AddRecentFile(NULL, openFileName, this, str.str());
        str.rdbuf()->freeze(0);
        dialog->Delete();
        return VTK_OK;
        }
      // Cleanup
      dialog->Delete();
      }    
    }
  else
    {
    vtkErrorMacro(<<error.str());
    }

  return VTK_ERROR;
}

//-----------------------------------------------------------------------------
vtkPVReaderModule* vtkPVWindow::InitializeReadCustom(const char* proto,
                                                     const char* fileName)
{
  if ( !proto || vtkString::Length(proto) == 0 || 
       !fileName || vtkString::Length(fileName) == 0 )
    {
    return 0;
    }

  vtkLinkedListIterator<vtkPVReaderModule*>* it = 
    this->ReaderList->NewIterator();
  while(!it->IsDoneWithTraversal())
    {
    vtkPVReaderModule* rm = 0;
    int retVal = it->GetData(rm);
    if (retVal == VTK_OK && rm && vtkString::Equals(rm->GetModuleName(), proto))
      {
      it->Delete();
      return this->InitializeRead(rm, fileName);
      }
    it->GoToNextItem();
    }
  it->Delete();
  return 0;

}

//-----------------------------------------------------------------------------
// N.B. The order in which the traces are added should not be changed.
// The custom reader modules rely on this order to add their own traces
// correctly.
vtkPVReaderModule* vtkPVWindow::InitializeRead(vtkPVReaderModule* proto,
                                               const char *fileName)
{
  vtkPVReaderModule* clone = 0;
  if (proto->Initialize(fileName, clone) != VTK_OK)
    {
    return 0;
    }
  this->GetPVApplication()->AddTraceEntry(
    "set kw(%s) [$kw(%s) InitializeReadCustom \"%s\" \"%s\"]", 
    clone->GetTclName(), this->GetTclName(), proto->GetModuleName(), fileName);

  return clone;
}

//-----------------------------------------------------------------------------
// N.B. The order in which the traces are added should not be changed.
// The custom reader modules rely on this order to add their own traces
// correctly.
int vtkPVWindow::ReadFileInformation(vtkPVReaderModule* clone,
                                     const char *fileName)
{
  int retVal = clone->ReadFileInformation(fileName);

  if (retVal == VTK_OK)
    {
    this->GetPVApplication()->AddTraceEntry(
      "$kw(%s) ReadFileInformation $kw(%s) \"%s\"", 
      this->GetTclName(), clone->GetTclName(), fileName);
    }
  else
    {
    clone->Delete();
    }

  return retVal;
}

//-----------------------------------------------------------------------------
// N.B. The order in which the traces are added should not be changed.
// The custom reader modules rely on this order to add their own traces
// correctly.
int vtkPVWindow::FinalizeRead(vtkPVReaderModule* clone, const char *fileName)
{
  this->GetPVApplication()->AddTraceEntry(
    "$kw(%s) FinalizeRead $kw(%s) \"%s\"", 
    this->GetTclName(), clone->GetTclName(), fileName);

  return clone->Finalize(fileName);
}

//-----------------------------------------------------------------------------
int vtkPVWindow::OpenCustom(const char* reader, const char* filename)
{
  if ( !reader || vtkString::Length(reader) == 0 || 
       !filename || vtkString::Length(filename) == 0 )
    {
    return VTK_ERROR;
    }
  vtkLinkedListIterator<vtkPVReaderModule*>* it = 
    this->ReaderList->NewIterator();
  while(!it->IsDoneWithTraversal())
    {
    vtkPVReaderModule* rm = 0;
    int retVal = it->GetData(rm);
    if (retVal == VTK_OK && rm && 
        vtkString::Equals(rm->GetModuleName(), reader) &&
        this->OpenWithReader(filename, rm) == VTK_OK )
      {
      it->Delete();
      return VTK_OK;
      }
    it->GoToNextItem();
    }
  it->Delete();
  return VTK_ERROR;
}

//-----------------------------------------------------------------------------
int vtkPVWindow::OpenWithReader(const char *fileName, 
                                vtkPVReaderModule* reader)
{
  vtkPVReaderModule* clone = this->InitializeRead(reader, fileName);
  if (!clone)
    {
    return VTK_ERROR;
    }
  int retVal;
  retVal = this->ReadFileInformation(clone, fileName);
  if (retVal != VTK_OK)
    {
    return retVal;
    }
  retVal = this->FinalizeRead(clone, fileName);
  if (retVal != VTK_OK)
    {
    // Clone should delete itself on an error
    return retVal;
    }
  return VTK_OK;

}

//-----------------------------------------------------------------------------
void vtkPVWindow::WriteVTKFile(const char* filename, int ghostLevel,
                               int timeSeries)
{
  if(!this->CurrentPVSource)
    {
    return;
    }
  
  // Check the number of processes.
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numParts = this->GetCurrentPVSource()->GetNumberOfParts();
  int numProcs = pvApp->GetProcessModule()->GetNumberOfPartitions();
  int parallel = (numProcs > 1);
  
  // Find the writer that supports this file name and data type.
  vtkPVWriter* writer = this->FindPVWriter(filename, parallel, numParts);
  
  // Make sure a writer is available for this file type.
  if(!writer)
    {
    ostrstream msg;
    msg << "No writers support";
    
    if(parallel)
      {
      msg << " parallel writing of ";
      }
    else
      {
      msg << " serial writing of ";
      }
    
    msg << this->GetCurrentPVSource()->GetDataInformation()->GetDataSetTypeAsString()
        << " to file with name \"" << filename << "\"" << ends;
    
    if (this->UseMessageDialog)
      {
      vtkKWMessageDialog::PopupMessage(
        this->Application, this, "Error Saving File", 
        msg.str(), 
        vtkKWMessageDialog::ErrorIcon);
      }
    else
      {
      vtkErrorMacro(<< msg.str());
      }
    msg.rdbuf()->freeze(0);
    return;
    }
  
  // Now that we can safely write the file, add the trace entry.
  pvApp->AddTraceEntry("$kw(%s) WriteVTKFile \"%s\" %d", this->GetTclName(),
                       filename, ghostLevel);
  
  // Actually write the file.
  writer->Write(filename, this->GetCurrentPVSource(), numProcs, ghostLevel,
                timeSeries);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::WriteData()
{
  // Make sure there are data to write.
  if(!this->GetCurrentPVSource())
    {
    vtkKWMessageDialog::PopupMessage(
      this->Application, this, "Error Saving File", 
      "No data set is selected.", 
      vtkKWMessageDialog::ErrorIcon);
    return;
    }
  
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->RootScript("%s GetClassName",
                 this->GetCurrentPVSource()->GetPart()->GetVTKDataTclName());
  // Instantiator does not work for static builds and VTK objects.
  vtkDataSet* data;
  const char* dataClassName = pm->GetRootResult();
  if (strcmp(dataClassName, "vtkImageData") == 0)
    {
    data = vtkImageData::New();
    }
  else if (strcmp(dataClassName, "vtkStructuredPoints") == 0)
    {
    data = vtkStructuredPoints::New();
    }
  else if (strcmp(dataClassName, "vtkStructuredGrid") == 0)
    {
    data = vtkStructuredGrid::New();
    }
  else if (strcmp(dataClassName, "vtkRectilinearGrid") == 0)
    {
    data = vtkRectilinearGrid::New();
    }
  else if (strcmp(dataClassName, "vtkPolyData") == 0)
    {
    data = vtkPolyData::New();
    }
  else if (strcmp(dataClassName, "vtkUnstructuredGrid") == 0)
    {
    data = vtkUnstructuredGrid::New();
    }
  else
    {
    vtkKWMessageDialog::PopupMessage(
      this->Application, this, "Error Saving File", 
      "Error getting data type from root node.",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

  // Check the number of processes.
  vtkPVApplication *pvApp = this->GetPVApplication();
  int parallel = (pvApp->GetProcessModule()->GetNumberOfPartitions() > 1);
  int numParts = this->GetCurrentPVSource()->GetNumberOfParts();
  const char* defaultExtension = 0;
  
  ostrstream typesStr;
  
  // Build list of file types supporting this data type.
  vtkLinkedListIterator<vtkPVWriter*>* it =
    this->FileWriterList->NewIterator();
  while(!it->IsDoneWithTraversal())
    {
    vtkPVWriter* wm = 0;
    if((it->GetData(wm) == VTK_OK) && wm->CanWriteData(data, parallel,
                                                       numParts))
      {
      const char* desc = wm->GetDescription();
      const char* ext = wm->GetExtension();

      typesStr << " {{" << desc << "} {" << ext << "}}";
      if(!defaultExtension)
        {
        defaultExtension = ext;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();
  
  data->Delete();
  
  // Make sure we have at least one writer.
  if(!defaultExtension)
    {
    ostrstream msg;
    msg << "No writers support";
    
    if(parallel)
      {
      msg << " parallel writing of ";
      }
    else
      {
      msg << " serial writing of ";
      }
    
    msg << this->GetCurrentPVSource()->GetDataInformation()->GetDataSetTypeAsString()
        << "." << ends;

    vtkKWMessageDialog::PopupMessage(
      this->Application, this, "Error Saving File", 
      msg.str(),
      vtkKWMessageDialog::ErrorIcon);
    msg.rdbuf()->freeze(0);
    return;
    }
  
  typesStr << ends;
  char* types = vtkString::Duplicate(typesStr.str());
  typesStr.rdbuf()->freeze(0);
  
  vtkKWLoadSaveDialog* saveDialog = pm->NewLoadSaveDialog();
  
  this->RetrieveLastPath(saveDialog, "SaveDataFile");
  saveDialog->SaveDialogOn();
  saveDialog->SetParent(this);
  saveDialog->SetTitle(VTK_PV_SAVE_DATA_MENU_LABEL);
  saveDialog->SetDefaultExtension(defaultExtension);
  saveDialog->SetFileTypes(types);
  saveDialog->Create(this->Application, 0);
  // Ask the user for the filename.  Default the extension to the
  // first writer supported.

  delete [] types;

  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( saveDialog->Invoke() &&
       vtkString::Length(saveDialog->GetFileName())>0 )
    {
    const char* filename = saveDialog->GetFileName();
    
    // If the current source is a reader and can provide time steps, ask
    // the user whether to write the whole time series.
    int timeSeries = 0;
    vtkPVReaderModule* reader =
      vtkPVReaderModule::SafeDownCast(this->GetCurrentPVSource());
    if(reader && (reader->GetNumberOfTimeSteps() > 0) &&
       vtkKWMessageDialog::PopupYesNo(
         this->Application, this, "Timesteps",
         "The current source provides multiple time steps.  "
         "Do you want to save all time steps?", 0))
      {
      timeSeries = 1;
      }
    
    // Choose ghost level.
    int ghostLevel = 0;    
    if(parallel)
      {
      vtkPVGhostLevelDialog* dlg = vtkPVGhostLevelDialog::New();
      dlg->Create(this->Application, "");
      dlg->SetMasterWindow(this);
      dlg->SetTitle("Select ghost levels");

      if ( dlg->Invoke() )
        {
        ghostLevel = dlg->GetGhostLevel();
        }
      dlg->Delete();
      if(ghostLevel < 0)
        {
        ghostLevel = 0;
        }
      }
  
    // Write the file.
    this->WriteVTKFile(filename, ghostLevel, timeSeries);
    this->SaveLastPath(saveDialog, "SaveDataFile");
    }
  this->SetEnabled(enabled);
  saveDialog->Delete();
}

//-----------------------------------------------------------------------------
vtkPVWriter* vtkPVWindow::FindPVWriter(const char* fileName, int parallel,
                                       int numParts)
{
  // Find the writer that supports this file name and data type.
  vtkPVWriter* writer = 0;
  
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->RootScript("%s GetClassName",
                 this->GetCurrentPVSource()->GetPart()->GetVTKDataTclName());

  vtkDataSet* data;
  const char* dataClassName = pm->GetRootResult();
  if (strcmp(dataClassName, "vtkImageData") == 0)
    {
    data = vtkImageData::New();
    }
  else if (strcmp(dataClassName, "vtkStructuredPoints") == 0)
    {
    data = vtkStructuredPoints::New();
    }
  else if (strcmp(dataClassName, "vtkStructuredGrid") == 0)
    {
    data = vtkStructuredGrid::New();
    }
  else if (strcmp(dataClassName, "vtkRectilinearGrid") == 0)
    {
    data = vtkRectilinearGrid::New();
    }
  else if (strcmp(dataClassName, "vtkPolyData") == 0)
    {
    data = vtkPolyData::New();
    }
  else if (strcmp(dataClassName, "vtkUnstructuredGrid") == 0)
    {
    data = vtkUnstructuredGrid::New();
    }

  vtkLinkedListIterator<vtkPVWriter*>* it =
    this->FileWriterList->NewIterator();
  while(!it->IsDoneWithTraversal())
    {
    vtkPVWriter* wm = 0;
    if((it->GetData(wm) == VTK_OK) && wm->CanWriteData(data, parallel,
                                                       numParts))
      {
      if(vtkString::EndsWith(fileName, wm->GetExtension()))
        {
        writer = wm;
        break;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();
  data->Delete();
  return writer;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SaveBatchScript()
{
  vtkKWLoadSaveDialog* exportDialog = vtkKWLoadSaveDialog::New();
  this->RetrieveLastPath(exportDialog, "SaveBatchLastPath");
  exportDialog->SetParent(this);
  exportDialog->Create(this->Application,0);
  exportDialog->SaveDialogOn();
  exportDialog->SetTitle("Save Batch Script");
  exportDialog->SetDefaultExtension(".pvb");
  exportDialog->SetFileTypes("{{ParaView Batch Script} {.pvb}} {{All Files} {*}}");
  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( exportDialog->Invoke() && 
       vtkString::Length(exportDialog->GetFileName())>0)
    {
    this->SaveBatchScript(exportDialog->GetFileName());
    this->SaveLastPath(exportDialog, "SaveBatchLastPath");
    }
  this->SetEnabled(enabled);
  exportDialog->Delete();
}

//-----------------------------------------------------------------------------
const char* vtkPVWindow::ExtractFileExtension(const char* fname)
{
  if (!fname)
    {
    return 0;
    }

  int pos = vtkString::Length(fname)-1;
  while (pos > 0)
    {
    if ( fname[pos] == '.' )
      {
      return fname+pos;
      }
    pos--;
    }
  return 0;
}


//-----------------------------------------------------------------------------
void vtkPVWindow::SaveBatchScript(const char* filename)
{
  vtkPVSource *pvs;
  int offScreenFlag = 0;
  int animationFlag = 0;
  const char *imageFileName = NULL;
  const char *geometryFileName = NULL;
  ofstream* file;
  char* ptr;
  char* fileExt = NULL;
  char* fileRoot = NULL;
  char* filePath = new char[strlen(filename)+1];
  
  // Extract directory, root and extension.
  strcpy(filePath, filename);
  ptr = filePath;
  while (*ptr)
    {
    if (*ptr == '/')
      {
      fileRoot = ptr;
      }
    if (*ptr == '.')
      {
      fileExt = ptr;
      }
    ++ptr;
    }
  if (fileRoot)
    {
    *fileRoot = '\0';
    ++fileRoot;
    }
  if (fileExt)
    {
    *fileExt = '\0';
    ++fileExt;
    }

  // We may want different questions if there is no animation.
  const char *script = this->AnimationInterface->GetScript();
  if (script && script[0] && this->AnimationInterface->GetScriptAvailable() && 
    this->AnimationInterface->IsAnimationValid())
    {
    animationFlag = 1;
    }

  // SaveBatchScriptDialog
  vtkPVSaveBatchScriptDialog* dialog = vtkPVSaveBatchScriptDialog::New();
  dialog->SetMasterWindow(this);
  dialog->SetFileRoot(fileRoot);
  dialog->SetFilePath(filePath);
  dialog->Create(this->GetPVApplication());    

  delete [] filePath;
  filePath = NULL;
  fileRoot = NULL;
  fileExt = NULL;
  ptr = NULL;

  if (dialog->Invoke() == 0)
    { // Cancel condition.
    dialog->Delete();
    dialog = NULL;
    return;
    }

  // Should I continue to save the batch script if it
  // does not save an image or geometry?
  offScreenFlag = dialog->GetOffScreen();
  imageFileName = dialog->GetImagesFileName();
  geometryFileName = dialog->GetGeometryFileName();

  file = new ofstream(filename, ios::out);
  if (file->fail())
    {
    vtkErrorMacro("Could not open file " << filename);
    dialog->Delete();
    dialog = NULL;
    delete file;
    return;
    }

  *file << "# ParaView Version " 
        << this->GetPVApplication()->GetMajorVersion()
        << "." << this->GetPVApplication()->GetMinorVersion() << "\n\n";

  if (this->PackageNames->GetNumberOfItems() > 0)
    {
    *file << vtkPVApplication::LoadComponentProc << endl;
    vtkLinkedListIterator<const char*>* it = this->PackageNames->NewIterator();
    while (!it->IsDoneWithTraversal())
      {
      const char* name = 0;
      if (it->GetData(name) == VTK_OK && name)
        {
        *file << "::paraview::load_component " << name << endl;
        }
      it->GoToNextItem();
      }
    it->Delete();
    }
  *file << endl << endl;


  const char* extension = 0;
  const char* writerName = 0;
  if (imageFileName && vtkString::Length(imageFileName) > 0)
    {
    extension = this->ExtractFileExtension(imageFileName);
    if ( !extension)
      {
      vtkKWMessageDialog::PopupMessage(this->Application, this,
                                       "Error",  "Filename has no extension."
                                       " Can not requested identify file"
                                       " format."
                                       " No image file will be generated.",
                                       vtkKWMessageDialog::ErrorIcon);
      imageFileName = NULL;
      }
    else
      {
      if ( this->Writers->GetItem(extension, writerName) != VTK_OK )
        {
        writerName = 0;
        ostrstream err;
        err << "Unrecognized extension: " << extension << "." 
            << " No image file will be generated." << ends;
        vtkKWMessageDialog::PopupMessage(this->Application, this,
                                         "Error",  err.str(),
                                         vtkKWMessageDialog::ErrorIcon);
        err.rdbuf()->freeze(0);
        imageFileName = NULL;
        }
      }
    }

  // Save the renderer stuff.
  this->GetMainView()->SaveInBatchScript(file);
  if (offScreenFlag)
    {
    *file << "RenWin1 SetOffScreenRendering 1\n\n";
    }    

  // Save out the VTK data pipeline.
  vtkArrayMapIterator<const char*, vtkPVSourceCollection*>* it =
    this->SourceLists->NewIterator();
  // Mark all sources as not visited.
  while( !it->IsDoneWithTraversal() )
    {    
    vtkPVSourceCollection* col = 0;
    if (it->GetData(col) == VTK_OK && col)
      {
      vtkCollectionIterator *cit = col->NewIterator();
      cit->InitTraversal();
      while ( !cit->IsDoneWithTraversal() )
        {
        pvs = static_cast<vtkPVSource*>(cit->GetObject()); 
        pvs->SetVisitedFlag(0);
        cit->GoToNextItem();
        }
      cit->Delete();
      }
    it->GoToNextItem();
    }
  it->Delete();
  // Mark all color maps as not visited.
  vtkPVColorMap *cm;
  this->PVColorMaps->InitTraversal();
  while( (cm = (vtkPVColorMap*)(this->PVColorMaps->GetNextItemAsObject())) )
    {    
    cm->SetVisitedFlag(0);
    }
  // Loop through sources saving the visible sources.
  vtkPVSourceCollection* modules = this->GetSourceList("Sources");
  vtkCollectionIterator* cit = modules->NewIterator();
  cit->InitTraversal();
  while ( !cit->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>(cit->GetObject()); 
    pvs->SaveInBatchScript(file);
    cit->GoToNextItem();
    }
  cit->Delete();

  // Set up the composite manager.
  // We do not know if it will be run in parallel.
  *file << "\n\nvtkPVBatchCompositeManager compManager\n\t";
  *file << "compManager SetRenderWindow RenWin1 \n\t";
  *file << "compManager InitializePieces\n\n";
  *file << "if {[catch {set myProcId [[compManager GetController] "
      "GetLocalProcessId]}]} {set myProcId 0 } \n";
  *file << "if {[catch {set numberOfProcs [[compManager GetController] "
      "GetNumberOfProcesses]}]} {set numberOfProcs 1 } \n\n";

  // Create the image writer if necessary.
  if (imageFileName && vtkString::Length(imageFileName) > 0)
    {
    if ( extension && writerName)
      {
      *file << writerName << " ImageWriter\n\t";
      *file << "ImageWriter SetInput [compManager GetOutput]\n\n";
      }
    }

  if (geometryFileName)
    {
    //*file << "if {$numberOfProcs > 1} {\n";
    //*file << "\tvtkXMLPPolyDataWriter GeometryWriter\n";
    //*file << "\tGeometryWriter SetNumberOfPieces $numberOfProcs" << endl;
    //*file << "\tGeometryWriter SetStartPiece $myProcId\n";
    //*file << "\tGeometryWriter SetEndPiece $myProcId\n";
    //*file << "} else {\n";
    //*file << "\tvtkXMLPolyDataWriter GeometryWriter\n";
    //*file << "}\n";
    //*file << "GeometryWriter SetDataModeToBinary" << endl;
    //*file << "GeometryWriter EncodeAppendedDataOff" << endl;
    *file << "vtkCollectPolyData CollectionFilter\n";
    *file << "vtkPolyData TempPolyData\n";
    *file << "vtkXMLPolyDataWriter GeometryWriter\n";
    *file << "\tGeometryWriter SetInput TempPolyData\n";
    *file << "[CollectionFilter GetOutput] SetUpdateNumberOfPieces $numberOfProcs\n";
    *file << "[CollectionFilter GetOutput] SetUpdatePiece $myProcId\n";

    }

  if (animationFlag)
    {
    this->AnimationInterface->SaveInBatchScript(file, imageFileName, 
                                                geometryFileName);
    }
  else
    { // Just do one frame.
    if (imageFileName)
      {
      *file << "RenWin1 Render\n";
      *file << "compManager Composite\n";
      *file << "if {$myProcId == 0} {\n";
      *file << "\t" << "ImageWriter SetFileName {" << imageFileName << "}\n";
      *file << "\t" << "ImageWriter Write\n";
      *file << "}\n\n";
      }

    if (geometryFileName)
      {
      this->SaveGeometryInBatchFile(file, geometryFileName, -1);
      }
    }
  *file << "vtkCommand DeleteAllObjects\n";

  dialog->Delete();
  dialog = NULL;
  delete file;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SaveGeometryInBatchFile(ofstream *file, 
                                          const char* filename,
                                          int timeIdx) 
{
  vtkPVSource* source;
  const char* sourceName;
  int numParts, partIdx;
  vtkPVPart* part;
  char *fileName;
  vtkPVSourceCollection *sources;
  char* fileExt = NULL;
  char* fileRoot = NULL;
  char* filePath = new char[strlen(filename)+1];
  char* ptr;
  
  // Extract directory, root and extension.
  strcpy(filePath, filename);
  ptr = filePath;
  while (*ptr)
    {
    if (*ptr == '/')
      {
      fileRoot = ptr;
      }
    if (*ptr == '.')
      {
      fileExt = ptr;
      }
    ++ptr;
    }
  if (fileRoot)
    {
    *fileRoot = '\0';
    ++fileRoot;
    }
  if (fileExt)
    {
    *fileExt = '\0';
    ++fileExt;
    }

  fileName = new char[strlen(fileRoot) + 30];      

  // Loop through visible sources.
  sources = this->GetSourceList("Sources");
  sources->InitTraversal();
  while ( (source = sources->GetNextPVSource()) )
    {
    if (source->GetVisibility())
      {
      sourceName = source->GetName();
      numParts = source->GetNumberOfParts();
      for ( partIdx = 0; partIdx < numParts; ++partIdx)
        {
        part = source->GetPart(partIdx);
        // Create a file name for the geometry (without extension).
        if (numParts == 1 && timeIdx >= 0)
          {
          sprintf(fileName, "%s/%s%sT%04d", 
                  filePath, fileRoot, sourceName, timeIdx);
          }
        else if (numParts != 1 && timeIdx >= 0)
          {
          sprintf(fileName, "%s/%s%sP%dT%04d", 
                  filePath, fileRoot, sourceName, partIdx, timeIdx);
          }
        else if (numParts == 1 && timeIdx < 0)
          {
          sprintf(fileName, "%s/%s%s", 
                  filePath, fileRoot, sourceName);
          }
        else if (numParts != 1 && timeIdx < 0)
          {
          sprintf(fileName, "%s/%s%sP%d", 
                  filePath, fileRoot, sourceName, partIdx);
          }
        //*file << "GeometryWriter SetInput [" << part->GetGeometryTclName() << " GetOutput]\n";
        //*file << "if {$numberOfProcs > 1} {\n";
        //*file << "\tGeometryWriter SetFileName {" << fileName << ".pvtp}\n";
        //*file << "} else {\n";
        //*file << "\tGeometryWriter SetFileName {" << fileName << ".vtp}\n";
        //*file << "}\n";
        //*file << "GeometryWriter Write\n";

        *file << "CollectionFilter SetInput [" << part->GetGeometryTclName() << " GetOutput]\n";
        *file << "[CollectionFilter GetOutput] Update\n";
        *file << "TempPolyData ShallowCopy [CollectionFilter GetOutput]\n";
        *file << "if {$myProcId == 0} {\n";
        *file << "\tGeometryWriter SetFileName {" << fileName << ".vtp}\n";
        *file << "\tGeometryWriter Write\n";
        *file << "}\n";
        }
      }
    }

  delete [] fileName;
  fileName = NULL;
  delete [] filePath;
}


//-----------------------------------------------------------------------------
void vtkPVWindow::SaveState()
{
  vtkKWLoadSaveDialog* exportDialog = vtkKWLoadSaveDialog::New();
  this->RetrieveLastPath(exportDialog, "SaveStateLastPath");
  exportDialog->SetParent(this);
  exportDialog->Create(this->Application,0);
  exportDialog->SaveDialogOn();
  exportDialog->SetTitle("Save State");
  exportDialog->SetDefaultExtension(".pvs");
  exportDialog->SetFileTypes("{{ParaView State} {.pvs}} {{All Files} {*}}");
  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( exportDialog->Invoke() && 
       vtkString::Length(exportDialog->GetFileName())>0)
    {
    this->SaveState(exportDialog->GetFileName());
    this->SaveLastPath(exportDialog, "SaveStateLastPath");
    }
  this->SetEnabled(enabled);
  exportDialog->Delete();
}
//-----------------------------------------------------------------------------
void vtkPVWindow::SaveState(const char* filename)
{
  vtkPVSource *pvs;
  ofstream* file;    
  vtkPVColorMap *cm;
  
  file = new ofstream(filename, ios::out);
  if (file->fail())
    {
    vtkErrorMacro("Could not open file " << filename);
    delete file;
    return;
    }

  *file << "# ParaView State Version " 
        << this->GetPVApplication()->GetMajorVersion()
        << "." << this->GetPVApplication()->GetMinorVersion() << "\n\n";

  /*
  if (this->PackageNames->GetNumberOfItems() > 0)
    {
    *file << vtkPVApplication::LoadComponentProc << endl;
    vtkLinkedListIterator<const char*>* it = this->PackageNames->NewIterator();
    while (!it->IsDoneWithTraversal())
      {
      const char* name = 0;
      if (it->GetData(name) == VTK_OK && name)
        {
        *file << "::paraview::load_component " << name << endl;
        }
      it->GoToNextItem();
      }
    it->Delete();
    }
  *file << endl << endl;
  */

  *file << "set kw(" << this->GetTclName() << ") [$Application GetMainWindow]" << endl;
  *file << "set kw(" << this->GetMainView()->GetTclName() 
        << ") [$kw(" << this->GetTclName() << ") GetMainView]" << endl;
  *file << "set kw(" << this->AnimationInterface->GetTclName() 
        << ") [$kw(" << this->GetTclName() << ") GetAnimationInterface]" << endl;

  vtkArrayMapIterator<const char*, vtkPVSourceCollection*>* it =
    this->SourceLists->NewIterator();

  // Mark all sources as not visited.
  while( !it->IsDoneWithTraversal() )
    {    
    vtkPVSourceCollection* col = 0;
    if (it->GetData(col) == VTK_OK && col)
      {
      vtkCollectionIterator *cit = col->NewIterator();
      cit->InitTraversal();
      while ( !cit->IsDoneWithTraversal() )
        {
        pvs = static_cast<vtkPVSource*>(cit->GetObject()); 
        pvs->SetVisitedFlag(0);
        cit->GoToNextItem();
        }
      cit->Delete();
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Loop through sources saving the visible sources.
  vtkPVSourceCollection* modules = this->GetSourceList("Sources");
  vtkCollectionIterator* cit = modules->NewIterator();
  cit->InitTraversal();
  while ( !cit->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>(cit->GetObject()); 
    pvs->SaveState(file);
    cit->GoToNextItem();
    }
  cit->Delete();

  // Save the state of all of the color maps.
  this->PVColorMaps->InitTraversal();
  while( (cm = (vtkPVColorMap*)(this->PVColorMaps->GetNextItemAsObject())) )
    {    
    cm->SaveState(file);
    }

  // Save the view at the end so camera get set properly.
  this->GetMainView()->SaveState(file);

  // Save state of the animation interface
  this->AnimationInterface->SaveState(file);

  delete file;
  file = NULL;
}


//-----------------------------------------------------------------------------
// Not implemented yet.
void vtkPVWindow::SaveWorkspace()
{
}

//-----------------------------------------------------------------------------
void vtkPVWindow::UpdateSourceMenu()
{
  if (!this->SourceMenu)
    {
    vtkWarningMacro("Source menu does not exist. Can not update.");
    return;
    }

  // Remove all of the entries from the source menu to avoid
  // adding things twice.
  this->SourceMenu->DeleteAllMenuItems();

  // Create all of the menu items for sources with no inputs.
  vtkArrayMapIterator<const char*, vtkPVSource*>* it = 
    this->Prototypes->NewIterator();
  vtkPVSource* proto;
  const char* key = 0;
  int numFilters = 0;
  while ( !it->IsDoneWithTraversal() )
    {
    proto = 0;
    if (it->GetData(proto) == VTK_OK)
      {
      // Check if this is a source (or a toolbar module). We do not want to 
      // add those to the source lists.
      if (proto && proto->GetNumberOfInputProperties() == 0)
        {
        numFilters++;
        char methodAndArgs[150];
        it->GetKey(key);
        sprintf(methodAndArgs, "CreatePVSource %s", key);
        const char* menuName = proto->GetMenuName();
        if (!menuName)
          {
          menuName = key;
          }
        this->SourceMenu->AddCommand(menuName, this, methodAndArgs,
                                     proto->GetShortHelp());
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  // If there are no filters, disable the menu.
  if (numFilters > 0)
    {
    this->Menu->SetState(VTK_PV_VTK_SOURCES_MENU_LABEL, 
                         vtkKWMenu::Normal);
    }
  else
    {
    this->Menu->SetState(VTK_PV_VTK_SOURCES_MENU_LABEL, 
                         vtkKWMenu::Disabled);
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::UpdateFilterMenu()
{
  if (!this->FilterMenu)
    {
    vtkWarningMacro("Filter menu does not exist. Can not update.");
    return;
    }

  // Remove all of the entries from the filter menu.
  this->FilterMenu->DeleteAllMenuItems();
  this->DisableToolbarButtons();

  if (this->CurrentPVSource &&
      !this->CurrentPVSource->GetIsPermanent() && 
      !this->CurrentPVSource->GetHideDisplayPage() )
    {
    // Add all the appropriate filters to the filter menu.
    vtkArrayMapIterator<const char*, vtkPVSource*>* it = 
      this->Prototypes->NewIterator();
    vtkPVSource* proto;
    const char* key = 0;
    int numSources = 0;
    while ( !it->IsDoneWithTraversal() )
      {
      proto = 0;
      if (it->GetData(proto) == VTK_OK)
        {
        // Check if this is an appropriate filter by comparing
        // it's input type with the current data object's type.
        if (proto && proto->GetInputProperty(0) &&
            proto->GetInputProperty(0)->GetIsValidInput(this->CurrentPVSource, proto))
          {
          it->GetKey(key);
          
          numSources++;
          char methodAndArgs[150];
          sprintf(methodAndArgs, "CreatePVSource %s", key);
          
          const char* menuName = proto->GetMenuName();
          if (!menuName)
            {
            menuName = key;
            }
          
          if (numSources % 25 == 0 )
            {
            this->FilterMenu->AddGeneric("command", menuName, this, 
                                         methodAndArgs, "-columnbreak 1", 
                                         proto->GetShortHelp());
            }
          else
            {
            this->FilterMenu->AddGeneric("command", menuName, this, 
                                         methodAndArgs, 0, 
                                         proto->GetShortHelp());
            }
          }

        if (proto->GetToolbarModule())
          {
          this->EnableToolbarButton(key);
          }
        }
      it->GoToNextItem();
      }
    it->Delete();
    
    // If there are no sources, disable the menu.
    if (numSources > 0)
      {
      this->Menu->SetState(VTK_PV_VTK_FILTERS_MENU_LABEL, 
                           vtkKWMenu::Normal);
      }
    else
      {
      this->Menu->SetState(VTK_PV_VTK_FILTERS_MENU_LABEL, 
                           vtkKWMenu::Disabled);
      }
    }
  else
    {
    // If there is no current data, disable the menu.
    this->DisableToolbarButtons();
    this->Menu->SetState(VTK_PV_VTK_FILTERS_MENU_LABEL, 
                         vtkKWMenu::Disabled);
    }
  
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DisableFilterMenu()
{
  this->DisableToolbarButtons();
  this->Menu->SetState(VTK_PV_VTK_FILTERS_MENU_LABEL,
                       vtkKWMenu::Disabled);
}

//-----------------------------------------------------------------------------
vtkPVSource* vtkPVWindow::GetPreviousPVSource(int idx)
{
  vtkPVSourceCollection* col = GetSourceList("Sources");
  if (col)
    {
    int pos = col->IsItemPresent(this->GetCurrentPVSource());
    return vtkPVSource::SafeDownCast(col->GetItemAsObject(pos-1-idx));
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetCurrentPVSourceCallback(vtkPVSource *pvs)
{
  this->SetCurrentPVSource(pvs);

  if (pvs)
    {
    pvs->SetAcceptButtonColorToWhite();
    if (pvs->InitializeTrace(NULL))
      {
      this->GetPVApplication()->AddTraceEntry(
        "$kw(%s) SetCurrentPVSourceCallback $kw(%s)", 
        this->GetTclName(), pvs->GetTclName());
      }
    }
  else
    {
    this->GetPVApplication()->AddTraceEntry(
      "$kw(%s) SetCurrentPVSourceCallback {}", this->GetTclName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetCurrentPVSource(vtkPVSource *pvs)
{

  if (pvs == this->CurrentPVSource)
    {
    this->ShowCurrentSourceProperties();
    return;
    }

  // Handle selection.
  if (this->CurrentPVSource)
    {
    // If there is a new current source, we tell the old one
    // not to unpack itself, since the new one will do this
    // anyway. This is a work-around for some packing problems.
    if (pvs)
      {
      this->CurrentPVSource->Deselect(0);
      }
    else
      {
      this->CurrentPVSource->Deselect(1);
      }
    }
  if (pvs)
    {
    pvs->Select();
    }

  // Handle reference counting
  if (pvs)
    {
    pvs->Register(this);
    }
  if (this->CurrentPVSource)
    {
    this->CurrentPVSource->UnRegister(this);
    this->CurrentPVSource = NULL;
    }

  // Set variable.
  this->CurrentPVSource = pvs;

  // Other stuff
  this->UpdateFilterMenu();

  // This will update the parameters.  
  // I doubt the conditional is still necessary.
  if (pvs)
    {
    this->ShowCurrentSourceProperties();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::AddPVSource(const char* listname, vtkPVSource *pvs)
{
  if (pvs == NULL)
    {
    return;
    }

  vtkPVSourceCollection* col = this->GetSourceList(listname);
  if (col && col->IsItemPresent(pvs) == 0)
    {
    col->AddItem(pvs);
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::RemovePVSource(const char* listname, vtkPVSource *pvs)
{ 
  if (pvs)
    { 
    vtkPVSourceCollection* col = this->GetSourceList(listname);
    if (col)
      {
      col->RemoveItem(pvs);
      this->UpdateSelectMenu();
      }
    }
}


//-----------------------------------------------------------------------------
vtkPVSourceCollection* vtkPVWindow::GetSourceList(const char* listname)
{
  vtkPVSourceCollection* col=0;
  if (this->SourceLists->GetItem(listname, col) == VTK_OK)
    {
    return col;
    }
  return 0;
}


//-----------------------------------------------------------------------------
void vtkPVWindow::ResetCameraCallback()
{

  this->GetPVApplication()->AddTraceEntry("$kw(%s) ResetCameraCallback", 
                                          this->GetTclName());
  this->MainView->ResetCamera();
  this->MainView->EventuallyRender();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::UpdateSelectMenu()
{
  if (!this->SelectMenu)
    {
    vtkWarningMacro("Selection menu does not exist. Can not update.");
    return;
    }

  vtkPVSource *source;
  char methodAndArg[512];

  this->SelectMenu->DeleteAllMenuItems();

  int numGlyphs=0;
  this->GlyphMenu->DeleteAllMenuItems();
  vtkPVSourceCollection* glyphSources = this->GetSourceList("GlyphSources");
  if (glyphSources)
    {
    vtkCollectionIterator *it = glyphSources->NewIterator();
    it->InitTraversal();
    while ( !it->IsDoneWithTraversal() )
      {
      source = static_cast<vtkPVSource*>(it->GetObject());
      sprintf(methodAndArg, "SetCurrentPVSourceCallback %s", 
              source->GetTclName());
      this->GlyphMenu->AddCommand(source->GetName(), this, methodAndArg,
                                  source->GetVTKSource() ?
                                  source->GetVTKSource()->GetClassName()+3
                                  : 0);
      numGlyphs++;
      it->GoToNextItem();
      }
    it->Delete();
    }


  vtkPVSourceCollection* sources = this->GetSourceList("Sources");
  if ( sources )
    {
    vtkCollectionIterator *it = sources->NewIterator();
    it->InitTraversal();
    while ( !it->IsDoneWithTraversal() )
      {
      int numSources = 0;
      source = static_cast<vtkPVSource*>(it->GetObject());
      sprintf(methodAndArg, "SetCurrentPVSourceCallback %s", 
              source->GetTclName());
      this->SelectMenu->AddCommand(source->GetName(), this, methodAndArg,
                                   source->GetVTKSource() ?
                                   source->GetVTKSource()->GetClassName()+3
                                   : 0);
      numSources++;
      it->GoToNextItem();
      }
    it->Delete();
    }
  
  if (numGlyphs > 0)
    {
    this->SelectMenu->AddCascade("Glyphs", this->GlyphMenu, 0,
                                 "Select one of the glyph sources.");  
    }

  // Disable or enable the menu.
  this->EnableSelectMenu();
}

//-----------------------------------------------------------------------------
// Disable or enable the select menu. Checks if there are any valid
// entries in the menu, disables the menu if there none, enables it
// otherwise.
void vtkPVWindow::EnableSelectMenu()
{
  int numSources;
  vtkPVSourceCollection* sources = this->GetSourceList("Sources");
  if (sources)
    {
    numSources =  sources->GetNumberOfItems();
    }
  else
    {
    numSources = 0;
    }

  int numGlyphs;
  sources = this->GetSourceList("GlyphSources");
  if (sources)
    {
    numGlyphs =  sources->GetNumberOfItems();
    }
  else
    {
    numGlyphs = 0;
    }
  
  if (numSources == 0)
    {
    this->Menu->SetState(VTK_PV_SELECT_SOURCE_MENU_LABEL, vtkKWMenu::Disabled);
    this->GetMenuView()->SetState(VTK_PV_SOURCE_MENU_LABEL, 
                                  vtkKWMenu::Disabled);
    }
  else
    {
    this->Menu->SetState(VTK_PV_SELECT_SOURCE_MENU_LABEL, vtkKWMenu::Normal);
    this->GetMenuView()->SetState(VTK_PV_SOURCE_MENU_LABEL, 
                                  vtkKWMenu::Normal);
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::EnableNavigationWindow()
{
  this->MainView->GetNavigationFrame()->EnabledOff();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DisableNavigationWindow()
{
  this->MainView->GetNavigationFrame()->EnabledOn();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DisableMenus()
{
  if (this->MenusDisabled)
    {
    return;
    }
  this->MenusDisabled = 1;

  // First store the state of all menu items.
  this->Menu->StoreMenuState(this->MenuState);

  int numMenus = this->Menu->GetNumberOfItems();
  // deactivating menus and toolbar buttons (except the interactors)
  for (int i = 0; i <= numMenus; i++)
    {
    this->Menu->SetState(i, vtkKWMenu::Disabled);
    }
}


//-----------------------------------------------------------------------------
void vtkPVWindow::EnableMenus()
{
  if (!this->MenusDisabled)
    {
    return;
    }
  // Now restore the state of all menu items
  this->Menu->RestoreMenuState(this->MenuState);
  this->MenusDisabled = 0;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DisableToolbarButtons()
{
  this->ToolbarButtonsDisabled = 1;
  vtkArrayMapIterator<const char*, vtkKWPushButton*>* it = 
    this->ToolbarButtons->NewIterator();
  while ( !it->IsDoneWithTraversal() )
    {
    vtkKWPushButton* button = 0;
    if (it->GetData(button) == VTK_OK && button)
      {
      button->EnabledOff();
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::EnableToolbarButton(const char* buttonName)
{
  vtkKWPushButton *button = 0;
  if ( this->ToolbarButtons->GetItem(buttonName, button) == VTK_OK &&
       button )
    {
    button->EnabledOn();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DisableToolbarButton(const char* buttonName)
{
  vtkKWPushButton *button = 0;
  if ( this->ToolbarButtons->GetItem(buttonName, button) == VTK_OK &&
       button )
    {
    button->EnabledOff();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::EnableToolbarButtons()
{
  if (this->CurrentPVSource == NULL)
    {
    return;
    }

  vtkArrayMapIterator<const char*, vtkKWPushButton*>* it = 
    this->ToolbarButtons->NewIterator();
  while ( !it->IsDoneWithTraversal() )
    {
    vtkKWPushButton* button = 0;
    const char* key = 0;
    if (it->GetData(button) == VTK_OK && button && it->GetKey(key) && key)
      {
      vtkPVSource* proto = 0;
      if ( this->Prototypes->GetItem(key, proto) == VTK_OK && proto)
        {
        if (proto->GetInputProperty(0) &&
            proto->GetInputProperty(0)->GetIsValidInput(this->CurrentPVSource, proto) )
          {
          button->EnabledOn();
          }
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  this->ToolbarButtonsDisabled = 0;


}

//-----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::ExtractGridCallback()
{
  if (this->CurrentPVSource == NULL)
    { // This should not be able to happen but ...
    return NULL;
    }

  int type = this->CurrentPVSource->GetDataInformation()->GetDataSetType();
  if (type == VTK_IMAGE_DATA || type == VTK_STRUCTURED_POINTS)
    {
    return this->CreatePVSource("ExtractVOI"); 
    }
  if (type == VTK_STRUCTURED_GRID)
    {
    return this->CreatePVSource("ExtractGrid");
    }
  if (type == VTK_RECTILINEAR_GRID)
    {
    return this->CreatePVSource("ExtractRectilinearGrid");
    }
  vtkErrorMacro("Unknown data type.");
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ShowCurrentSourcePropertiesCallback()
{
  this->GetPVApplication()->AddTraceEntry(
    "$kw(%s) ShowCurrentSourcePropertiesCallback", this->GetTclName());

  this->ShowCurrentSourceProperties();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ShowCurrentSourceProperties()
{
  // Bring up the properties panel
  
  this->ShowProperties();
  
  // We need to update the view-menu radio button too!

  this->GetMenuView()->CheckRadioButton(
    this->GetMenuView(), "Radio", VTK_PV_SOURCE_MENU_INDEX);

  this->MainView->GetSplitFrame()->UnpackSiblings();

  this->Script("pack %s -side top -fill both -expand t",
               this->MainView->GetSplitFrame()->GetWidgetName());

  if (!this->GetCurrentPVSource())
    {
    return;
    }

  this->GetCurrentPVSource()->Pack();
  this->GetCurrentPVSource()->RaiseSourcePage();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ShowAnimationProperties()
{
  this->GetPVApplication()->AddTraceEntry("$kw(%s) ShowAnimationProperties",
                                          this->GetTclName());

  this->AnimationInterface->PrepareAnimationInterface(this);

  // Bring up the properties panel
  this->ShowProperties();
  
  // We need to update the properties-menu radio button too!
  this->GetMenuView()->CheckRadioButton(
    this->GetMenuView(), "Radio", VTK_PV_ANIMATION_MENU_INDEX);

  // Get rid of the page already packed.
  this->AnimationInterface->UnpackSiblings();

  // Put our page in.
  this->Script("pack %s -anchor n -side top -expand t -fill x",
               this->AnimationInterface->GetWidgetName());
}

//-----------------------------------------------------------------------------
vtkPVApplication *vtkPVWindow::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->Application);
}


//-----------------------------------------------------------------------------
void vtkPVWindow::ShowTimerLog()
{
  if ( ! this->TimerLogDisplay )
    {
    this->TimerLogDisplay = vtkPVTimerLogDisplay::New();
    this->TimerLogDisplay->SetTitle("Performance Log");
    this->TimerLogDisplay->SetMasterWindow(this);
    this->TimerLogDisplay->Create(this->GetPVApplication());
    }
  
  this->TimerLogDisplay->Display();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ShowErrorLog()
{
  this->CreateErrorLogDisplay();  
  this->ErrorLogDisplay->Display();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::CreateErrorLogDisplay()
{
  if ( ! this->ErrorLogDisplay )
    {
    this->ErrorLogDisplay = vtkPVErrorLogDisplay::New();
    this->ErrorLogDisplay->SetTitle("Error Log");
    this->ErrorLogDisplay->SetMasterWindow(this);
    this->ErrorLogDisplay->Create(this->GetPVApplication());
    }  
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SaveTrace()
{
  vtkKWLoadSaveDialog* exportDialog = vtkKWLoadSaveDialog::New();
  this->RetrieveLastPath(exportDialog, "SaveTracePath");
  exportDialog->SetParent(this);
  exportDialog->Create(this->Application,0);
  exportDialog->SaveDialogOn();
  exportDialog->SetTitle("Save ParaView Trace");
  exportDialog->SetDefaultExtension(".pvs");
  exportDialog->SetFileTypes("{{ParaView Trace} {.pvs}} {{All Files} {*}}");
  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( exportDialog->Invoke() && 
       vtkString::Length(exportDialog->GetFileName())>0 &&
       this->SaveTrace(exportDialog->GetFileName()) )
    {
    this->SaveLastPath(exportDialog, "SaveTracePath");
    }
  this->SetEnabled(enabled);
  exportDialog->Delete();
}

//-----------------------------------------------------------------------------
int vtkPVWindow::SaveTrace(const char* filename)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  ofstream *trace = pvApp->GetTraceFile();

  if (vtkString::Length(filename) <= 0)
    {
    return 0;
    }

  if (trace)
    {
    trace->close();
    }
  
  const int bufferSize = 4096;
  char buffer[bufferSize];

  ofstream newTrace(filename);
  ifstream oldTrace(pvApp->GetTraceFileName());
  
  while(oldTrace)
    {
    oldTrace.read(buffer, bufferSize);
    if(oldTrace.gcount())
      {
      newTrace.write(buffer, oldTrace.gcount());
      }
    }

  if (trace)
    {
    trace->open(pvApp->GetTraceFileName(), ios::in | ios::app );
    }
  return 1;
}

//-----------------------------------------------------------------------------
// Create a new data object/source by cloning a module prototype.
vtkPVSource *vtkPVWindow::CreatePVSource(const char* moduleName,
                                         const char* sourceList,
                                         int addTraceEntry,
                                         int grabFocus)
{
  vtkPVSource *pvs = 0;
  vtkPVSource* clone = 0;
  int success;

  if ( this->Prototypes->GetItem(moduleName, pvs) == VTK_OK ) 
    {
    if (grabFocus)
      {
      this->DisableToolbarButtons();
      this->DisableMenus();
      }

    // Make the cloned source current only if it is going into
    // the Sources list.
    if (sourceList && strcmp(sourceList, "Sources") != 0)
      {
      success = pvs->CloneAndInitialize(0, clone);
      }
    else
      {
      success = pvs->CloneAndInitialize(1, clone);
      }

    if (success != VTK_OK)
      {
      this->EnableToolbarButtons();
      this->EnableMenus();
      vtkErrorMacro("Cloning operation for " << moduleName
                    << " failed.");
      return 0;
      }

    if (!clone)
      {
      this->EnableToolbarButtons();
      this->EnableMenus();
      return 0;
      }
    
    if (grabFocus)
      {
      clone->GrabFocus();
      }

    if (addTraceEntry)
      {
      if (clone->GetTraceInitialized() == 0)
        { 
        if (sourceList)
          {
          this->GetPVApplication()->AddTraceEntry(
            "set kw(%s) [$kw(%s) CreatePVSource %s %s]", 
            clone->GetTclName(), this->GetTclName(),
            moduleName, sourceList);
          }
        else
          {
          this->GetPVApplication()->AddTraceEntry(
            "set kw(%s) [$kw(%s) CreatePVSource %s]", 
            clone->GetTclName(), this->GetTclName(),
            moduleName);
          }
        clone->SetTraceInitialized(1);
        }
      }

    clone->UpdateParameterWidgets();

    vtkPVSourceCollection* col = 0;
    if(sourceList)
      {
      col = this->GetSourceList(sourceList);
      if ( col )
        {
        col->AddItem(clone);
        }
      else
        {
        vtkWarningMacro("The specified source list (" 
                        << (sourceList ? sourceList : "Sources") 
                        << ") could not be found.");
        }
      }
    else
      {
      this->AddPVSource("Sources", clone);
      }
    clone->Delete();
    }
  else
    {
    vtkErrorMacro("Prototype for " << moduleName << " could not be found.");
    }
  
  return clone;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::LoadScript(const char *name)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);

  pvApp->SetRunningParaViewScript(1);
  this->vtkKWWindow::LoadScript(name);
  pvApp->SetRunningParaViewScript(0);
}

//-----------------------------------------------------------------------------
int vtkPVWindow::OpenPackage()
{
  int res = 0;
  vtkKWLoadSaveDialog* loadDialog = vtkKWLoadSaveDialog::New();
  this->RetrieveLastPath(loadDialog, "PackagePath");
  loadDialog->SetParent(this);
  loadDialog->Create(this->Application,0);
  loadDialog->SetTitle("Open ParaView Package");
  loadDialog->SetDefaultExtension(".xml");
  loadDialog->SetFileTypes("{{ParaView Package Files} {*.xml}} {{All Files} {*}}");
  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( loadDialog->Invoke() && this->OpenPackage(loadDialog->GetFileName()) )
    {
    this->SaveLastPath(loadDialog, "PackagePath");
    res = 1;
    }
  this->SetEnabled(enabled);
  loadDialog->Delete();
  return res;
}

//-----------------------------------------------------------------------------
int vtkPVWindow::OpenPackage(const char* openFileName)
{
  if ( this->CheckIfFileIsReadable(openFileName) != VTK_OK )
    {
    return VTK_ERROR;
    }

  this->ReadSourceInterfacesFromFile(openFileName);

  // Store last path
  if ( openFileName && vtkString::Length(openFileName) > 0 )
    {
    char *pth = vtkString::Duplicate(openFileName);
    int pos = vtkString::Length(openFileName);
    // Strip off the file name
    while (pos && pth[pos] != '/' && pth[pos] != '\\')
      {
      pos--;
      }
    pth[pos] = '\0';
    // Store in the registery
    this->GetApplication()->SetRegisteryValue(
      2, "RunTime", "PackagePath", pth);
    delete [] pth;
    }

  // Initialize a couple of variables in the trace file.
  this->GetApplication()->AddTraceEntry(
    "$kw(%s) OpenPackage \"%s\"", this->GetTclName(), openFileName);

  return VTK_OK;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ReadSourceInterfaces()
{
  // Add special sources.
  
  // Setup our built in source interfaces.
  vtkPVInitialize* initialize = vtkPVInitialize::New();
  initialize->Initialize(this);
  initialize->Delete();
  
  // A list of standard directories in which to find interfaces.  The
  // first directory in this list that is found is the only one used.
  static const char* standardDirectories[] =
    {
#ifdef VTK_PV_BINARY_CONFIG_DIR
      VTK_PV_BINARY_CONFIG_DIR,
#endif
#ifdef VTK_PV_SOURCE_CONFIG_DIR
      VTK_PV_SOURCE_CONFIG_DIR,
#endif
#ifdef VTK_PV_INSTALL_CONFIG_DIR
      VTK_PV_INSTALL_CONFIG_DIR,
#endif
      0
    };
  
  // Parse input files from the first directory found to exist.
  int found=0;
  for(const char** dir=standardDirectories; !found && *dir; ++dir)
    {
    found = this->ReadSourceInterfacesFromDirectory(*dir);
    }
  if(!found)
    {
    // Don't complain for now.  We can choose desired behavior later.
    // vtkWarningMacro("Could not find any directories for standard interface files.");
    }

  char* str = getenv("PV_INTERFACE_PATH");
  if (str)
    {
    this->ReadSourceInterfacesFromDirectory(str);
    }

}

//-----------------------------------------------------------------------------
void vtkPVWindow::ReadSourceInterfacesFromString(const char* str)
{
  // Setup our built in source interfaces.
  vtkPVXMLPackageParser* parser = vtkPVXMLPackageParser::New();
  parser->Parse(str);
  parser->StoreConfiguration(this);
  parser->Delete();

  this->UpdateSourceMenu();
  this->UpdateFilterMenu();
  this->Toolbar->UpdateWidgets();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ReadSourceInterfacesFromFile(const char* file)
{
  vtkPVXMLPackageParser* parser = vtkPVXMLPackageParser::New();
  parser->SetFileName(file);
  if(parser->Parse())
    {
    parser->StoreConfiguration(this);
    }
  parser->Delete();

  this->UpdateSourceMenu();
  this->UpdateFilterMenu();
  this->Toolbar->UpdateWidgets();
}

//-----------------------------------------------------------------------------
// Walk through the list of .xml files in the given directory and
// parse each one for sources and filters.  Returns whether the
// directory was found.
int vtkPVWindow::ReadSourceInterfacesFromDirectory(const char* directory)
{
  vtkDirectory* dir = vtkDirectory::New();
  if(!dir->Open(directory))
    {
    dir->Delete();
    return 0;
    }
  
  for(int i=0; i < dir->GetNumberOfFiles(); ++i)
    {
    const char* file = dir->GetFile(i);
    int extPos = vtkString::Length(file)-4;
    
    // Look for the ".xml" extension.
    if((extPos > 0) && vtkString::Equals(file+extPos, ".xml"))
      {
      char* fullPath 
        = new char[vtkString::Length(file)+vtkString::Length(directory)+2];
      strcpy(fullPath, directory);
      strcat(fullPath, "/");
      strcat(fullPath, file);
      
      this->ReadSourceInterfacesFromFile(fullPath);
      
      delete [] fullPath;
      }
    }
  
  dir->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::WizardCallback()
{
  return;
//    if (this->GetModuleLoaded("vtkARLTCL.pvm") == 0)
//      {
//      return;
//      }

//    vtkPVWizard *w = vtkPVWizard::New();
//    w->SetParent(this);
//    w->Create(this->Application, "");
//    w->Invoke(this);
//    w->Delete();
}


//-----------------------------------------------------------------------------
void vtkPVWindow::AddFileType(const char *description, const char *ext,
                              vtkPVReaderModule* prototype)
{
  int length = 0;
  char *newStr;
  
  if (ext == NULL)
    {
    vtkErrorMacro("Missing extension.");
    return;
    }
  if (description == NULL)
    {
    description = "";
    }

  // First add to the extension string.
  if (this->FileExtensions)
    {
    length = vtkString::Length(this->FileExtensions);
    }
  length += vtkString::Length(ext) + 5;
  newStr = new char [length];
#ifdef _WIN32
  if (this->FileExtensions == NULL)
    {  
    sprintf(newStr, "*%s", ext);
    }
  else
    {
    sprintf(newStr, "%s;*%s", this->FileExtensions, ext);
    }
#else
  if (this->FileExtensions == NULL)
    {  
    sprintf(newStr, "%s", ext);
    }
  else
    {
    sprintf(newStr, "%s %s", this->FileExtensions, ext);
    }
#endif
  if (this->FileExtensions)
    {
    delete [] this->FileExtensions;
    }
  this->FileExtensions = newStr;
  newStr = NULL;

  // Now add to the description string.
  length = 0;
  if (this->FileDescriptions)
    {
    length = vtkString::Length(this->FileDescriptions);
    }
  length += vtkString::Length(description) + vtkString::Length(ext) + 10;
  newStr = new char [length];
  if (this->FileDescriptions == NULL)
    {  
    sprintf(newStr, "{{%s} {%s}}", description, ext);
    }
  else
    {
    sprintf(newStr, "%s {{%s} {%s}}", 
            this->FileDescriptions, description, ext);
    }
  if (this->FileDescriptions)
    {
    delete [] this->FileDescriptions;
    }
  this->FileDescriptions = newStr;
  newStr = NULL;

  int found=0;
  vtkLinkedListIterator<vtkPVReaderModule*>* it = 
    this->ReaderList->NewIterator();
  while(!it->IsDoneWithTraversal())
    {
    vtkPVReaderModule* rm = 0;
    it->GetData(rm);
    if ( rm == prototype )
      {
      found = 1;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  if (!found)
    {
    this->ReaderList->AppendItem(prototype);
    }
  this->MenuFile->SetState(VTK_PV_OPEN_DATA_MENU_LABEL, vtkKWMenu::Normal);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::AddFileWriter(vtkPVWriter* writer)
{
  writer->SetApplication(this->GetPVApplication());
  this->FileWriterList->AppendItem(writer);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::WarningMessage(const char* message)
{
  this->Script("bell");
  this->CreateErrorLogDisplay();
  char *wmessage = vtkString::Duplicate(message);
  this->InvokeEvent(vtkKWEvent::WarningMessageEvent, wmessage);
  delete [] wmessage;
  this->ErrorLogDisplay->AppendError(message);
  this->SetErrorIcon(2);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ErrorMessage(const char* message)
{  
  this->Script("bell");
  this->CreateErrorLogDisplay();
  char *wmessage = vtkString::Duplicate(message);
  this->InvokeEvent(vtkKWEvent::ErrorMessageEvent, wmessage);
  delete [] wmessage;
  this->ErrorLogDisplay->AppendError(message);
  this->SetErrorIcon(2);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ProcessErrorClick()
{
  this->Superclass::ProcessErrorClick();
  this->ShowErrorLog();
}

//-----------------------------------------------------------------------------
vtkPVColorMap* vtkPVWindow::GetPVColorMap(const char* parameterName, 
                                          int numberOfComponents)
{
  vtkPVColorMap *cm;

  if (parameterName == NULL || parameterName[0] == '\0')
    {
    vtkErrorMacro("Requesting color map for NULL parameter.");
    return NULL;
    }

  vtkCollectionIterator* it = this->PVColorMaps->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    cm = static_cast<vtkPVColorMap*>(it->GetObject());
    if (cm->MatchArrayName(parameterName, numberOfComponents))
      {
      it->Delete();
      return cm;
      }
    it->GoToNextItem();
    }
  it->Delete();
  
  cm = vtkPVColorMap::New();
  cm->SetParent(this->GetMainView()->GetPropertiesParent());
  cm->SetPVRenderView(this->GetMainView());
  cm->SetArrayName(parameterName);
  cm->SetNumberOfVectorComponents(numberOfComponents);
  cm->Create(this->GetPVApplication());
  cm->SetTraceReferenceObject(this);
  cm->SetScalarBarTitleInternal(parameterName);
  cm->ResetScalarRangeInternal();

  this->PVColorMaps->AddItem(cm);
  cm->Delete();

  return cm;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::AddManipulator(const char* rotypes, const char* name, 
                                 vtkPVCameraManipulator* pcm)
{
  if ( !pcm || !this->MainView )
    {
    return;
    }

  char *types = vtkString::Duplicate(rotypes);
  char t[100];
  int res = 1;

  istrstream str(types);
  str.width(100);
  while(str >> t)
    {
    if ( vtkString::Equals(t, "2D") )
      {
      this->MainView->GetManipulatorControl2D()->AddManipulator(name, pcm);
      }
    else if (vtkString::Equals(t, "3D") )
      {
      this->MainView->GetManipulatorControl3D()->AddManipulator(name, pcm);
      }
    else
      {
      vtkErrorMacro("Unknonwn type of manipulator: " << name << " - " << t);
      res = 0;
      break;
      }
    str.width(100);
    }
  delete [] types;
  if ( res )
    {
    this->MainView->UpdateCameraManipulators();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::AddManipulatorArgument(const char* rotypes, const char* name,
                                         const char* variable, 
                                         vtkPVWidget* widget)
{
  if ( !rotypes || !name || !variable || !widget || !this->MainView )
    {
    return;
    }

  char *types = vtkString::Duplicate(rotypes);
  char t[100];
  int res = 1;

  istrstream str(types);
  str.width(100);
  while(str >> t)
    {
    if ( vtkString::Equals(t, "2D") )
      {
      this->MainView->GetManipulatorControl2D()->AddArgument(variable, 
                                                             name, widget);
      }
    else if (vtkString::Equals(t, "3D") )
      {
      this->MainView->GetManipulatorControl3D()->AddArgument(variable, 
                                                             name, widget);
      }
    else
      {
      vtkErrorMacro("Unknonwn type of manipulator: " << name << " - " << t);
      res = 0;
      break;
      }
    str.width(100);
    }
  delete [] types;
  if ( res )
    {
    this->MainView->UpdateCameraManipulators();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::AddToNamesToSources(const char* name, vtkPVSource* source)
{
  if ( !this->NamesToSources )
    {
    this->NamesToSources = vtkArrayMap<const char*, vtkPVSource*>::New();
    }
  this->NamesToSources->SetItem(name, source);
}

//-----------------------------------------------------------------------------
vtkPVSource* vtkPVWindow::GetSourceFromName(const char* name)
{
  if ( !this->NamesToSources )
    {
    return 0;
    }
  vtkPVSource* source = 0;
  this->NamesToSources->GetItem(name, source);
  return source;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DeleteSourceAndOutputs(vtkPVSource* source)
{
  if ( !source )
    {
    return;
    }
  while ( source->GetNumberOfPVConsumers() > 0 )
    {
    vtkPVSource* consumer = source->GetPVConsumer(0);
    if ( consumer )
      {
      this->DeleteSourceAndOutputs(consumer);
      }
    }
  source->DeleteCallback();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DeleteAllSourcesCallback()
{
  vtkPVSourceCollection* col = this->GetSourceList("Sources");
  if ( col->GetNumberOfItems() <= 0 )
    {
    return;
    }
  if ( vtkKWMessageDialog::PopupYesNo(
         this->Application, this, "DeleteAllTheSources",
         "Delete All Modules", 
         "Are you sure you want to delete all the modules?", 
         vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::RememberYes |
         vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault ))
    {
    this->DeleteAllSources();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DeleteAllSources()
{
  vtkPVApplication* pvApp = static_cast<vtkPVApplication*>(this->Application);
  pvApp->AddTraceEntry("# User selected delete all modules");
  vtkPVSourceCollection* col = this->GetSourceList("Sources");
  while ( col->GetNumberOfItems() > 0 )
    {
    vtkPVSource* source = col->GetLastPVSource();
    if ( !source )
      {
      break;
      }
    this->DeleteSourceAndOutputs(source);
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetInteractiveRenderEnabled(int s)
{
  this->InteractiveRenderEnabled = s;
  vtkPVGenericRenderWindowInteractor* rwi = this->Interactor;
  rwi->SetInteractiveRenderEnabled(s);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::RemoveAllCaches()
{
  // Loop through all parts ...
}

//-----------------------------------------------------------------------------
void vtkPVWindow::CacheUpdate(int idx, int total)
{
  vtkPVSourceCollection* col = this->GetSourceList("Sources");
  vtkPVSource* pvs;
  vtkPVPart* pvp;
  int partIdx, numParts;

  col->InitTraversal();
  while ( (pvs=col->GetNextPVSource()) )
    {
    if (pvs->GetVisibility())
      {
      numParts = pvs->GetNumberOfParts();
      for (partIdx = 0; partIdx < numParts; ++partIdx)
        {
        pvp = pvs->GetPart(partIdx);
        pvp->GetPartDisplay()->CacheUpdate(idx, total);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CenterXEntry: " << this->GetCenterXEntry() << endl;
  os << indent << "CenterYEntry: " << this->GetCenterYEntry() << endl;
  os << indent << "CenterZEntry: " << this->GetCenterZEntry() << endl;
  os << indent << "FilterMenu: " << this->GetFilterMenu() << endl;
//  os << indent << "FlyStyle: " << this->GetFlyStyle() << endl;
  os << indent << "InteractorStyleToolbar: " << this->GetInteractorToolbar() 
     << endl;
  os << indent << "MainView: " << this->GetMainView() << endl;
  os << indent << "CameraStyle2D: " << this->CameraStyle2D << endl;
  os << indent << "CameraStyle3D: " << this->CameraStyle3D << endl;
  os << indent << "SelectMenu: " << this->SelectMenu << endl;
  os << indent << "SourceMenu: " << this->SourceMenu << endl;
  os << indent << "Toolbar: " << this->GetToolbar() << endl;
  os << indent << "PickCenterToolbar: " << this->GetPickCenterToolbar() << endl;
//  os << indent << "FlySpeedToolbar: " << this->GetFlySpeedToolbar() << endl;
  os << indent << "Interactor: " << this->Interactor << endl;
  os << indent << "GlyphMenu: " << this->GlyphMenu << endl;
  os << indent << "InitializeDefaultInterfaces: " 
     << this->InitializeDefaultInterfaces << endl;
  os << indent << "UseMessageDialog: " << this->UseMessageDialog << endl;
  os << indent << "InteractiveRenderEnabled: " 
     << (this->InteractiveRenderEnabled?"on":"off") << endl;
  os << indent << "ShowSourcesLongHelp: " 
     << (this->ShowSourcesLongHelp?"on":"off") << endl;
  os << indent << "AnimationInterface: " << this->AnimationInterface << endl;
}
