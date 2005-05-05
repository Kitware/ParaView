/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWindow.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVWindow.h"

#include "vtkObjectFactory.h"
#include "vtkArrayMap.txx"
#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkDataSet.h"
#include "vtkDirectory.h"
#include "vtkImageData.h"
#include "vtkKWBalloonHelpManager.h"
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWNotebook.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWPushButton.h"
#include "vtkKWPushButtonWithMenu.h"
#include "vtkKWRadioButton.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWToolbar.h"
#include "vtkKWToolbarSet.h"
#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkPVApplication.h"
#include "vtkPVApplicationSettingsInterface.h"
#include "vtkPVCameraManipulator.h"
#include "vtkPVColorMap.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVErrorLogDisplay.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVGhostLevelDialog.h"
#include "vtkPVInitialize.h"
#include "vtkPVInputProperty.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVInteractorStyleCenterOfRotation.h"
#include "vtkPVInteractorStyleControl.h"
#include "vtkSMPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVReaderModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSaveBatchScriptDialog.h"
#include "vtkPVSelectCustomReader.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSourceInterfaceDirectories.h"
#include "vtkPVTimerLogDisplay.h"
#include "vtkPVVolumeAppearanceEditor.h"
#include "vtkPVWriter.h"
#include "vtkPVXMLPackageParser.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkStdString.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkToolkits.h"
#include "vtkUnstructuredGrid.h"
#include "vtkClientServerStream.h"
#include "vtkTimerLog.h"
#include "vtkPVRenderViewProxyImplementation.h"
#include "vtkSMApplication.h"
#include "vtkPVGUIClientOptions.h"
#include <vtkstd/map>
#include "vtkSMAxesProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMProxyProperty.h"

#include "vtkPVConfig.h"  // Needed for PARAVIEW_USE_LOOKMARKS
#ifdef PARAVIEW_USE_LOOKMARKS
#  include "vtkPVLookmarkManager.h"
#endif

#include "Resources/vtkPVLogoSmall.h"

#ifndef _WIN32
# include <unistd.h>
#endif

#include <ctype.h>
#include <sys/stat.h>

#include <kwsys/SystemTools.hxx>

#ifndef VTK_USE_ANSI_STDLIB
# define PV_NOCREATE | ios::nocreate
#else
# define PV_NOCREATE 
#endif

#define VTK_PV_SHOW_HORZPANE_LABEL "Show Animation Tracks"
#define VTK_PV_HIDE_HORZPANE_LABEL "Hide Animation Tracks"

#define VTK_PV_VTK_FILTERS_MENU_LABEL "Filter"
#define VTK_PV_VTK_SOURCES_MENU_LABEL "Source"
#define VTK_PV_OPEN_DATA_MENU_LABEL "Open Data"
#define VTK_PV_SAVE_DATA_MENU_LABEL "Save Data"
#define VTK_PV_SELECT_SOURCE_MENU_LABEL "Select"

#define VTK_PV_TOOLBARS_INTERACTION_LABEL "Interaction"
#define VTK_PV_TOOLBARS_TOOLS_LABEL       "Tools"
#define VTK_PV_TOOLBARS_CAMERA_LABEL      "Camera"

#define VTK_PV_ENABLE_OLD_ANIMATION_INTERFACE 0

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVWindow);
vtkCxxRevisionMacro(vtkPVWindow, "1.696");

int vtkPVWindowCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//-----------------------------------------------------------------------------
vtkPVWindow::vtkPVWindow()
{
  this->LastProgress = 0;
  this->ExpectProgress = 0;
  
  this->InDemo = 0;
  this->Interactor = 0;

  this->InteractiveRenderEnabled = 0;
  this->SetWindowClass("ParaView");
  char* title = getenv("PARAVIEW_TITLE");
  if( title )
    {
    this->SetTitle( title );
    }
  else
    {
    this->SetTitle( "Kitware ParaView" );
    }

  this->CommandFunction = vtkPVWindowCommand;
  this->ModifiedEnableState = 0;

  this->TraceHelper = vtkPVTraceHelper::New();
  this->TraceHelper->SetObject(this);

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
  this->PreferencesMenu = 0;
  
  // This toolbar contains buttons for modifying user interaction
  // mode
  this->InteractorToolbar = vtkKWToolbar::New();

  this->ResetCameraButton = vtkKWPushButtonWithMenu::New();
  
  this->RotateCameraButton = vtkKWRadioButton::New();
  this->TranslateCameraButton = vtkKWRadioButton::New();
   
  // This toolbar contains buttons for instantiating new modules
  this->Toolbar = vtkKWToolbar::New();

  // Keep a list of the toolbar buttons so that they can be 
  // disabled/enabled in certain situations.
  this->ToolbarButtons = vtkArrayMap<const char*, vtkKWPushButton*>::New();
  
  // A menu to control toolbar button visibility.
  this->ToolbarMenuButton = vtkKWMenuButton::New();

  this->CameraStyle3D = vtkPVInteractorStyle::New();
  this->CameraStyle2D = vtkPVInteractorStyle::New();
  this->CenterOfRotationStyle = vtkPVInteractorStyleCenterOfRotation::New();

  this->PickCenterToolbar = vtkKWToolbar::New();

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
  
  this->CenterAxesProxy = 0;
  this->CenterAxesProxyName = 0;

  this->CurrentPVSource = NULL;

  this->AnimationManager = vtkPVAnimationManager::New();
  this->AnimationManager->GetTraceHelper()->SetReferenceHelper(
    this->GetTraceHelper());
  this->AnimationManager->GetTraceHelper()->SetReferenceCommand(
    "GetAnimationManager");
  this->AnimationManager->SetApplication(this->GetApplication());
  
  this->LowerToolbars = vtkKWToolbarSet::New();
  
  this->LowerFrame = vtkKWSplitFrame::New();
  this->LowerFrame->SetFrame1Size(480);
  this->LowerFrame->SetFrame1MinimumSize(0);
  this->LowerFrame->SetFrame2MinimumSize(0);

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

  // Keep a list of all loaded packages so they can be saved
  // for state or batch.
  this->PackageFiles = vtkLinkedList<const char*>::New();

  // This can be used to disable the pop-up dialogs if necessary
  // (usually used from inside regression scripts)
  this->UseMessageDialog = 1;
  // Whether or not to read the default interfaces.
  this->InitializeDefaultInterfaces = 1;

  this->MainView = 0;

  this->PVColorMaps = vtkCollection::New();

  this->VolumeAppearanceEditor = NULL;
  
  this->CenterActorVisibility = 1;

  this->MenusDisabled = 0;
  this->ToolbarButtonsDisabled = 0;

  this->UserInterfaceManager = 0;
  this->ApplicationSettingsInterface = 0;

  this->InteractorID.ID = 0;
  this->ServerFileListingID.ID = 0;

  this->SaveVisibleSourcesOnlyFlag = 0;
  #ifdef PARAVIEW_USE_LOOKMARKS
  this->PVLookmarkManager = NULL;
  #endif
}

//-----------------------------------------------------------------------------
vtkPVWindow::~vtkPVWindow()
{
  vtkClientServerStream stream;
  if(this->ServerFileListingID.ID)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    vtkPVProcessModule *pm = pvApp->GetProcessModule();
    if (pm)
      {
      pm->DeleteStreamObject(this->ServerFileListingID, stream);
      pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
      }
    }

  this->PrepareForDelete();

  if (this->TraceHelper)
    {
    this->TraceHelper->Delete();
    this->TraceHelper = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::PrepareForDelete()
{
  this->Superclass::PrepareForDelete();

  this->SetInteractor(NULL);

  // First delete the interface panels

  if (this->ApplicationSettingsInterface)
    {
    this->ApplicationSettingsInterface->Delete();
    this->ApplicationSettingsInterface = NULL;
    }

  // Then the interface manager

  if (this->UserInterfaceManager)
    {
    this->UserInterfaceManager->Delete();
    this->UserInterfaceManager = NULL;
    }

  if (this->ResetCameraButton)
    {
    this->ResetCameraButton->Delete();
    this->ResetCameraButton = NULL;
    }

  if (this->RotateCameraButton)
    {
    this->RotateCameraButton->Delete();
    this->RotateCameraButton = NULL;
    }

  if (this->TranslateCameraButton)
    {
    this->TranslateCameraButton->Delete();
    this->TranslateCameraButton = NULL;
    }

  if (this->SourceLists)
    {
    this->SourceLists->Delete();
    this->SourceLists = 0;
    }

  if (this->ToolbarButtons)
    {
    this->ToolbarButtons->Delete();
    this->ToolbarButtons = NULL;
    }

  if (this->PackageFiles)
    {
    this->PackageFiles->Delete();
    this->PackageFiles = NULL;
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

  if (this->Prototypes)
    {
    this->Prototypes->Delete();
    this->Prototypes = NULL;
    }

  if (this->ReaderList)
    {
    this->ReaderList->Delete();
    this->ReaderList = NULL;
    }

  if (this->Writers)
    {
    this->Writers->Delete();
    this->Writers = NULL;
    }

  if (this->FileWriterList)
    {
    this->FileWriterList->Delete();
    this->FileWriterList = NULL;
    }
    
  if (this->MenuState)
    {
    this->MenuState->Delete();
    this->MenuState = NULL;
    }

  if (this->CenterAxesProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("axes",
      this->CenterAxesProxyName);
    this->SetCenterAxesProxyName(NULL);
    }

  if (this->CenterAxesProxy)
    {
    this->CenterAxesProxy->Delete();
    this->CenterAxesProxy = NULL;
    }

  if (this->GetApplication())
    {
    int state;
    if (this->ResetCameraButton)
      {
      state = this->ResetCameraButton->GetCheckButtonState("ViewAngle");
      this->GetApplication()->SetRegistryValue(
        2, "RunTime", "ResetViewResetsViewAngle", "%d", state);
      state = this->ResetCameraButton->GetCheckButtonState("CenterOfRotation");
      this->GetApplication()->SetRegistryValue(
        2, "RunTime", "ResetViewResetsCenterOfRotation", "%d", state);
      }
    }

  if (this->AnimationManager)
    {
    this->AnimationManager->PrepareForDelete();
    this->AnimationManager->Delete();
    this->AnimationManager = NULL;
    }

  // Color maps have circular references because they
  // reference renderview.

  if (this->PVColorMaps)
    {
    this->PVColorMaps->Delete();
    this->PVColorMaps = NULL;
    }

  if ( this->VolumeAppearanceEditor )
    {
    this->VolumeAppearanceEditor->Delete();
    this->VolumeAppearanceEditor = NULL;
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

  if (this->ToolbarMenuButton)
    {
    this->ToolbarMenuButton->Delete();
    this->ToolbarMenuButton = NULL;
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

  if (this->LowerFrame)
    {
    this->LowerFrame->Delete();
    this->LowerFrame = NULL;
    }

  if (this->MainView)
    {
    this->MainView->Close();
    this->MainView->SetParentWindow(NULL);
    this->MainView->Delete();
    //this->MainView = NULL;
    }

  this->DeleteAllSources();
  if (this->SourceLists)
    {
    this->SourceLists->Delete();
    this->SourceLists = NULL;
    }

  if (this->GetApplication())
    {
    this->GetApplication()->SetRegistryValue(
      2, "RunTime", "CenterActorVisibility", "%d", 
      this->CenterActorVisibility);
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

  if (this->PreferencesMenu)
    {
    this->PreferencesMenu->Delete();
    this->PreferencesMenu=NULL;
    }

  if (this->LowerToolbars)
    {
    this->LowerToolbars->Delete();
    this->LowerToolbars = NULL;
    }

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

#ifdef PARAVIEW_USE_LOOKMARKS
  if (this->PVLookmarkManager )
    {
    this->PVLookmarkManager->SetMasterWindow(NULL);
    this->PVLookmarkManager->Delete();
    this->PVLookmarkManager = NULL;
    }
#endif
}

//-----------------------------------------------------------------------------
void vtkPVWindow::InitializeMenus(vtkKWApplication* vtkNotUsed(app))
{
  // Add view options.

  // View menu: Show the application settings

  char *rbv = 
    this->GetViewMenu()->CreateRadioButtonVariable(
      this->GetViewMenu(),"Radio");

  this->GetViewMenu()->AddRadioButton(
    VTK_PV_APPSETTINGS_MENU_INDEX, 
    VTK_PV_APPSETTINGS_MENU_LABEL, 
    rbv, 
    this, 
    "ShowApplicationSettingsInterface", 
    1,
    "Display the application settings");
  delete [] rbv;

  this->GetViewMenu()->AddSeparator();

  // View menu: shows the notebook for the current source and data object.

  rbv = 
    this->GetViewMenu()->CreateRadioButtonVariable(
      this->GetViewMenu(),"Radio");

  this->GetViewMenu()->AddRadioButton(
    VTK_PV_SOURCE_MENU_INDEX, 
    VTK_PV_SOURCE_MENU_LABEL, 
    rbv, 
    this, 
    "ShowCurrentSourcePropertiesCallback", 
    1,
    "Display the properties of the current data source or filter");
  delete [] rbv;

  // View menu: Shows the V animation tool
  rbv = this->GetViewMenu()->CreateRadioButtonVariable(
    this->GetViewMenu(), "Radio");

  this->GetViewMenu()->AddRadioButton(
    VTK_PV_ANIMATION_MENU_INDEX + 1,
    " Keyframe Animation",
    rbv,
    this,
    "ShowAnimationPanes",
    1,
    "Display the interface for creating animations");
  delete [] rbv;
  
  // File menu: 

  // We do not need Close in the file menu since we don't
  // support multiple windows (exit is enough)

  this->FileMenu->DeleteMenuItem("Close");

  int clidx = this->GetFileMenuIndex();

  // Open a data file. Can support multiple file formats (see Open()).

  this->FileMenu->InsertCommand(
    clidx++, VTK_PV_OPEN_DATA_MENU_LABEL, this, "OpenCallback",0);

  // Save current data in VTK format.

  this->FileMenu->InsertCommand(
    clidx++, VTK_PV_SAVE_DATA_MENU_LABEL, this, "WriteData",0);

  this->FileMenu->InsertSeparator(clidx++);

  // Add advanced file options

  this->FileMenu->InsertCommand(clidx++, "Load Session", this, 
                                "LoadScript", 0,
                                "Restore a trace of actions.");
  this->FileMenu->InsertCommand(clidx++, "Save Session State", this,
                                "SaveState", 7,
                                "Write the current state of ParaView "
                                "in a file.");
  this->FileMenu->InsertCommand(clidx++,
                                "Save Session Trace", this, 
                                "SaveTrace", 3,
                                "Save a trace of every action "
                                "since start up.");
  this->FileMenu->InsertCommand(clidx++, "Save Batch Script", this,
                                "SaveBatchScript", 7,
                                "Write a script which can run "
                                "in batch by ParaView");
  this->FileMenu->InsertCommand(clidx++, "Save SM State", this,
                                "SaveSMState", 6,
                                "Server the server manager state "
                                "as xml.");
  this->FileMenu->InsertCommand(clidx++,
                                "Import Package", this, 
                                "OpenPackage", 3,
                                "Import modules defined in a ParaView package ");
  
  this->PreferencesMenu = vtkKWMenu::New();
  this->PreferencesMenu->SetParent(this->FileMenu);
  this->PreferencesMenu->SetTearOff(0);
  this->PreferencesMenu->Create(this->GetApplication(), "");  

  //this->FileMenu->InsertCascade(clidx++,"Preferences", this->PreferencesMenu, 8);
  this->FileMenu->InsertSeparator(clidx++);

  this->FileMenu->InsertCommand(clidx++, "Save Animation", this,
    "SaveAnimation", 5, "Save animation as a movie or images.");
  
  this->FileMenu->InsertCommand(clidx++, "Save Geometry", this,
    "SaveGeometry", 5, "Save geometry from each frame. This will create "
    "a series of .vtp files.");
  
  this->FileMenu->InsertSeparator(clidx++);

  
  this->AddRecentFilesMenu(NULL, this);
  
  clidx = this->GetFileMenuIndex();

  this->FileMenu->InsertSeparator(clidx++);

  /*
  // Open XML package
  this->FileMenu->InsertCommand(clidx++, "Open Package", this, 
                                "OpenPackage", 8,
                                "Open a ParaView package and load the "
                                "contents");
  */

  // Select menu: ParaView specific menus.

  // Create the select menu (for selecting user created and default
  // (i.e. glyphs) data objects/sources)
  this->SelectMenu->SetParent(this->GetMenu());
  this->SelectMenu->Create(this->GetApplication(), "-tearoff 0");
  this->GetMenu()->InsertCascade(2, VTK_PV_SELECT_SOURCE_MENU_LABEL, this->SelectMenu, 0);
  
  // Create the menu for selecting the glyphs.  
  this->GlyphMenu->SetParent(this->SelectMenu);
  this->GlyphMenu->Create(this->GetApplication(), "-tearoff 0");
  this->SelectMenu->AddCascade("Glyphs", this->GlyphMenu, 0,
                                 "Select one of the glyph sources.");  

  // Create the menu for creating data sources.  
  this->SourceMenu->SetParent(this->GetMenu());
  this->SourceMenu->Create(this->GetApplication(), "-tearoff 0");
  this->GetMenu()->InsertCascade(3, VTK_PV_VTK_SOURCES_MENU_LABEL, 
                            this->SourceMenu, 0,
                            "Choose a source from a list of "
                            "VTK sources");  
  
  // Create the menu for creating data sources (filters).  
  this->FilterMenu->SetParent(this->GetMenu());
  this->FilterMenu->Create(this->GetApplication(), "-tearoff 0");
  this->GetMenu()->InsertCascade(4, VTK_PV_VTK_FILTERS_MENU_LABEL, 
                            this->FilterMenu, 2,
                            "Choose a filter from a list of "
                            "VTK filters");  

  // Window menu:

  this->GetWindowMenu()->AddSeparator();

  this->GetWindowMenu()->InsertCommand(
    4, "Command Prompt", this,
    "DisplayCommandPrompt", 8,
    "Display a prompt to interact with the ParaView engine");

  // Log stuff (not traced)
  this->GetWindowMenu()->InsertCommand(
    5, "Timer Log", this, 
    "ShowTimerLog", 2, 
    "Show log of render events and timing");
              
  // Log stuff (not traced)
  this->GetWindowMenu()->InsertCommand(
    5, "Error Log", this, 
    "ShowErrorLog", 2, 
    "Show log of all errors and warnings");

#ifdef PARAVIEW_USE_LOOKMARKS
  // Display Lookmark Manager
  this->GetWindowMenu()->InsertCommand(
    4, "Lookmark Manager", this, 
    "DisplayLookmarkManager", "Create and Manage Your Lookmarks");
#endif

  // Preferences sub-menu

  // Edit menu

  this->GetEditMenu()->InsertCommand(5, "Delete All Modules", this, 
                                     "DeleteAllSourcesCallback", 
                                     1, "Delete all modules in ParaView");

  this->GetEditMenu()->InsertCommand(6, "Delete All Keyframes", this,
    "DeleteAllKeyframesCallback", 11, "Delete all key frames in Animation");
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetToolbarVisibility(const char* identifier, int state)
{
  if (!strcmp(identifier, "tools"))
    {
    this->Superclass::SetToolbarVisibility(this->Toolbar, 
      VTK_PV_TOOLBARS_TOOLS_LABEL, state);
    }
  else if(!strcmp(identifier, "camera"))
    {
    this->Superclass::SetToolbarVisibility(this->PickCenterToolbar,
      VTK_PV_TOOLBARS_CAMERA_LABEL, state);
    }
  else if (!strcmp(identifier, "interaction"))
    {
    this->Superclass::SetToolbarVisibility(this->InteractorToolbar, 
      VTK_PV_TOOLBARS_INTERACTION_LABEL, state);
    }
}
//-----------------------------------------------------------------------------
void vtkPVWindow::InitializeToolbars(vtkKWApplication *app)
{
  this->AddToolbar(this->InteractorToolbar, VTK_PV_TOOLBARS_INTERACTION_LABEL);
  this->AddToolbar(this->Toolbar, VTK_PV_TOOLBARS_TOOLS_LABEL);
  this->AddToolbar(this->PickCenterToolbar, VTK_PV_TOOLBARS_CAMERA_LABEL);

  this->InteractorToolbar->SetParent(this->Toolbars->GetToolbarsFrame());
  this->InteractorToolbar->Create(app);

  this->Toolbar->SetParent(this->Toolbars->GetToolbarsFrame());
  this->Toolbar->Create(app);
  this->Toolbar->ResizableOn();
  //this->ToolbarMenuButton->SetParent(this->Toolbar->GetFrame());
  this->ToolbarMenuButton->SetParent(this->Toolbar);
  this->ToolbarMenuButton->Create(app, 
                                 "-image PVToolbarPullDownArrow -relief flat");
  this->ToolbarMenuButton->IndicatorOff();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::InitializeInteractorInterfaces(vtkKWApplication *app)
{
  // Set up the button to reset the camera.
  
  this->ResetCameraButton->SetParent(this->InteractorToolbar->GetFrame());
  this->ResetCameraButton->Create(app, "-image PVResetViewButton");
  this->ResetCameraButton->SetCommand(this, "ResetCameraCallback");
  this->ResetCameraButton->SetBalloonHelpString("Reset the view to show everything visible.");
  this->InteractorToolbar->AddWidget(this->ResetCameraButton);
  
  // Create options and popup option menu for the reset camera button.
  int state = 0;
  if (app->GetRegistryValue(2, "RunTime", "ResetViewResetsCenterOfRotation", 0))
    {
    state = app->GetIntRegistryValue(2, "RunTime", "ResetViewResetsCenterOfRotation");
    }
  this->ResetCameraButton->AddCheckButton("Reset Center Of Rotation", "CenterOfRotation", state,
                                          "Button sets the center opf rotation to center of visible modules.");
  state = 0;
  if (app->GetRegistryValue(2, "RunTime", "ResetViewResetsViewAngle", 0))
    {
    state = app->GetIntRegistryValue(2, "RunTime", "ResetViewResetsViewAngle");
    }
  this->ResetCameraButton->AddCheckButton("Reset View Angle", "ViewAngle", state,
                                          "Button sets the view plane normal to the default z axis.");
  
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
                                   const char* balloonHelp,
                                   int buttonVisibility)
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
  // Add the button to the toolbar configuration menu.
  vtkKWMenu* menu = this->ToolbarMenuButton->GetMenu();
  char* var = menu->CreateCheckButtonVariable(this, buttonName);
  ostrstream checkCommand;
  checkCommand << "ToolbarMenuCheckCallback " << buttonName << ends;
  menu->AddCheckButton(buttonName,var,this, checkCommand.str(),
    "Show/Hide button in toolbar.");
  // Look in the registry to see if a button should be visible.
  vtkKWApplication* app = this->GetApplication();
  if (app->GetRegistryValue(2, "RunTime", buttonName, 0))
    {
    buttonVisibility = app->GetIntRegistryValue(2, "RunTime", buttonName);
    }
  menu->CheckCheckButton(this, buttonName, buttonVisibility);

  // Lets see if we can put an image with the check button.
  int index = menu->GetNumberOfItems()-1;
  menu->ConfigureItem(index, opts.str());  
  checkCommand.rdbuf()->freeze(0);
  delete[] var;

  // Clean up.
  opts.rdbuf()->freeze(0);
  button->SetCommand(this, command);
  if (balloonHelp)
    {
    button->SetBalloonHelpString(balloonHelp);
    }
  this->ToolbarButtons->SetItem(buttonName, button);
  if (buttonVisibility)
    {
    this->Toolbar->AddWidget(button);
    }
  button->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ToolbarMenuCheckCallback(const char* buttonName)
{
  int checkValue;
  vtkKWMenu* menu;
  
  menu = this->ToolbarMenuButton->GetMenu();
  checkValue = menu->GetCheckButtonValue(this,buttonName);
  // Find the toolbar button widget.
  vtkKWPushButton *button = 0;
  if ( this->ToolbarButtons->GetItem(buttonName, button) == VTK_OK && button )
    {
    // Save in the registry
    vtkPVApplication* pvApp = this->GetPVApplication();
    pvApp->SetRegistryValue(2, "RunTime", buttonName, 
                             "%d", checkValue);
    if (checkValue)
      {
      this->Toolbar->AddWidget(button);
      // First disable the button.  Enable will only turn it on.
      button->EnabledOff();
      // This is the only method that checks for the correct type.
      this->EnableToolbarButtons();
      }
    else
      {
      this->Toolbar->RemoveWidget(button);
      }
    }
  this->UpdateEnableState();
}


//-----------------------------------------------------------------------------
void vtkPVWindow::SetInteractor(vtkPVGenericRenderWindowInteractor *interactor)
{
  // Do not bother referencing.
  this->Interactor = interactor;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::Create(vtkKWApplication *app, const char* vtkNotUsed(args))
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVWindow already created");
    return;
    }

  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);
  if (pvApp == NULL)
    {
    vtkErrorMacro("vtkPVWindow::Create needs a vtkPVApplication.");
    return;
    }
  // Set the render module on the CenterOfRotationStyle
  this->CenterOfRotationStyle->SetRenderModuleProxy(
    pvApp->GetRenderModuleProxy());
  this->CenterOfRotationStyle->SetPVWindow(this);
 
  // Make sure the widget is name appropriately: paraview instead of a number.
  // On X11, the window name is the same as the widget name.

  this->WidgetName = kwsys::SystemTools::DuplicateString(".paraview");

  // Allow the user to interactively resize the properties parent.
  // Set the left panel size (Frame1) for this app. Do it now before
  // the superclass eventually restores the size from the registry

  this->MiddleFrame->SetFrame1MinimumSize(200);
  this->MiddleFrame->SetFrame2MinimumSize(200);
  this->MiddleFrame->SetFrame1Size(380);
  this->MiddleFrame->SetSeparatorSize(5);

  // Invoke super method

  this->Superclass::Create(app, NULL);

  // Hide the main window until after all user interface is initialized.

  this->Withdraw();

  // Add Show lower pane to menu.
  this->GetWindowMenu()->AddCommand(VTK_PV_SHOW_HORZPANE_LABEL, this,
    "ToggleHorizontalPaneVisibilityCallback", 0);

  this->LowerToolbars->SetParent(this->GetViewFrame());
  this->LowerToolbars->Create(app,0);
  this->LowerToolbars->ShowBottomSeparatorOff();

  this->LowerFrame->Frame2VisibilityOff();

  this->LowerFrame->SetSeparatorSize(5);
  this->LowerFrame->SetOrientationToVertical();
  this->LowerFrame->SetParent(this->GetViewFrame());
  this->LowerFrame->Create(app);
  this->Script("pack %s -side top -fill both -expand t",
               this->LowerFrame->GetWidgetName());
  int hvisibility = 0;
  if (app->HasRegistryValue(2, "RunTime", "HorizontalPaneVisibility"))
    {
    hvisibility = app->GetIntRegistryValue(2, "RunTime", "HorizontalPaneVisibility");
    }
  this->SetHorizontalPaneVisibility(hvisibility);
  
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pvApp->GetBalloonHelpManager()->SetDelay(1);

  // Put the version in the status bar.
  char version[128];
  sprintf(version,"Version %d.%d", this->GetPVApplication()->GetMajorVersion(),
          this->GetPVApplication()->GetMinorVersion());
  this->SetStatusText(version);

  // Update gauge height to match status image
  
  this->ProgressGauge->SetHeight(
    vtkKWTkUtilities::GetPhotoHeight(this->StatusImage) - 4);

  // Init menus

  int use_splash = 
    (app->GetShowSplashScreen() && app->GetNumberOfWindows() == 1);
  if (use_splash)
    {
    pvApp->GetSplashScreen()->SetProgressMessage("Creating UI (menus)...");
    }
  this->InitializeMenus(app);

  // Init toolbars

  if (use_splash)
    {
    pvApp->GetSplashScreen()->SetProgressMessage("Creating UI (toolbars)...");
    }
  this->InitializeToolbars(app);

  this->SetInteractor(vtkPVGenericRenderWindowInteractor::SafeDownCast(
      this->GetPVApplication()->GetRenderModuleProxy()->GetInteractor()));

  // Create the main view.
  if (use_splash)
    {
    pvApp->GetSplashScreen()->SetProgressMessage("Creating UI (main view)...");
    }
  this->CreateMainView(pvApp);

  if (use_splash)
    {
    pvApp->GetSplashScreen()->SetProgressMessage("Creating UI (interactors)...");
    }
  this->InitializeInteractorInterfaces(app);
  this->PickCenterToolbar->SetParent(this->Toolbars->GetToolbarsFrame());
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

  // Creating the center of rotation actor here because we have the
  // application here.
  // Create Axes proxy
  vtkSMProxyManager *pxm = vtkSMObject::GetProxyManager();
  this->CenterAxesProxy = vtkSMAxesProxy::SafeDownCast(
    pxm->NewProxy("axes","Axes"));
  this->SetCenterAxesProxyName("CenterAxes");
  pxm->RegisterProxy("axes",this->CenterAxesProxyName, this->CenterAxesProxy);
  this->CenterAxesProxy->UpdateVTKObjects();
 
  // Add the axes proxy to the render module.
  vtkSMRenderModuleProxy* rm = this->GetPVApplication()->GetRenderModuleProxy();
  if (rm)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      rm->GetProperty("Displays"));
    if (pp)
      {
      pp->AddProxy(this->CenterAxesProxy);
      rm->UpdateVTKObjects();
      }
    }
  
  this->CenterEntryFrame->SetParent(this->PickCenterToolbar->GetFrame());
  this->CenterEntryFrame->Create(app, "frame", "");
  
  this->CenterXLabel->SetParent(this->CenterEntryFrame);
  this->CenterXLabel->Create(app, "");
  this->CenterXLabel->SetText("X");
  
  this->CenterXEntry->SetParent(this->CenterEntryFrame);
  this->CenterXEntry->Create(app, "-width 7");
  this->Script("bind %s <KeyPress-Return> {%s CenterEntryCallback}",
               this->CenterXEntry->GetWidgetName(), this->GetTclName());
  //this->CenterXEntry->SetValue(this->CameraStyle3D->GetCenter()[0], 3);
  this->CenterXEntry->SetValue(0.0);
  
  this->CenterYLabel->SetParent(this->CenterEntryFrame);
  this->CenterYLabel->Create(app, "");
  this->CenterYLabel->SetText("Y");
  
  this->CenterYEntry->SetParent(this->CenterEntryFrame);
  this->CenterYEntry->Create(app, "-width 7");
  this->Script("bind %s <KeyPress-Return> {%s CenterEntryCallback}",
               this->CenterYEntry->GetWidgetName(), this->GetTclName());
  //this->CenterYEntry->SetValue(this->CameraStyle3D->GetCenter()[1], 3);
  this->CenterYEntry->SetValue(0.0);

  this->CenterZLabel->SetParent(this->CenterEntryFrame);
  this->CenterZLabel->Create(app, "");
  this->CenterZLabel->SetText("Z");
  
  this->CenterZEntry->SetParent(this->CenterEntryFrame);
  this->CenterZEntry->Create(app, "-width 7");
  this->Script("bind %s <KeyPress-Return> {%s CenterEntryCallback}",
               this->CenterZEntry->GetWidgetName(), this->GetTclName());
  //this->CenterZEntry->SetValue(this->CameraStyle3D->GetCenter()[2], 3);
  this->CenterZEntry->SetValue(0.0);

  this->Script("pack %s %s %s %s %s %s -side left",
               this->CenterXLabel->GetWidgetName(),
               this->CenterXEntry->GetWidgetName(),
               this->CenterYLabel->GetWidgetName(),
               this->CenterYEntry->GetWidgetName(),
               this->CenterZLabel->GetWidgetName(),
               this->CenterZEntry->GetWidgetName());

  this->Script("bind %s <Control-KeyPress-Return> {%s AcceptCurrentSource}",
               this->GetWidgetName(), 
               this->GetTclName());
  this->Script("bind %s <Control-KeyPress-q> {%s Close}",
               this->GetWidgetName(), 
               this->GetTclName());

  if (pvApp->GetRegistryValue(2, "RunTime", "CenterActorVisibility", 0))
    {
    if (
      (this->CenterActorVisibility = 
       pvApp->GetIntRegistryValue(2, "RunTime", "CenterActorVisibility")))
      {
      this->ShowCenterActor();
      }
    else
      {
      this->HideCenterActor();
      }
    }
  vtkPVRenderViewProxyImplementation* proxy = 
    vtkPVRenderViewProxyImplementation::New();
  proxy->SetPVRenderView(this->MainView);
  this->Interactor->SetPVRenderView(proxy);
  proxy->Delete();
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
  this->Script("bind %s <Configure> {%s Configure %%w %%h}",
               wname, tname);
  
  // We need keyboard focus to get key events in the render window.
  // we are using the p key for picking.
  this->Script("bind %s <Enter> {focus %s}",
               wname, wname);
  this->Script("bind %s <KeyPress> {%s KeyAction %%K %%x %%y}",
               wname, tname);

  // Interface for the animation tool.
  this->AnimationManager->SetParent(this);
  this->AnimationManager->SetHorizantalParent(this->LowerFrame->GetFrame2());
  this->AnimationManager->SetVerticalParent(this->GetPropertiesParent());
  this->AnimationManager->Create(app, "-relief flat");
  this->AnimationManager->ShowHAnimationInterface();
  
  // File->Open Data File is disabled unless reader modules are loaded.
  // AddFileType() enables this entry.
  this->FileMenu->SetState(VTK_PV_OPEN_DATA_MENU_LABEL, vtkKWMenu::Disabled);

  if (app->GetSaveWindowGeometry())
    {
    this->RestorePVWindowGeometry();
    }

  if (this->InitializeDefaultInterfaces)
    {
    vtkPVSourceCollection* sources = vtkPVSourceCollection::New();
    this->SourceLists->SetItem("GlyphSources", sources);
    sources->Delete();

    // We need an application before we can read the interface.
    this->ReadSourceInterfaces();
    
    vtkPVSource *pvs=0;
    // Create the sources that can be used for glyphing.
    // ===== Arrow
    pvs = this->CreatePVSource("ArrowSource", "GlyphSources", 0, 0);
    if (pvs)
      {
      pvs->IsPermanentOn();
      pvs->Accept(1);
      pvs->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
      ostrstream s;
      s << "GetPVSource GlyphSources " << pvs->GetName() << ends;
      pvs->GetTraceHelper()->SetReferenceCommand(s.str());
      s.rdbuf()->freeze(0);
      }
    else
      {
      vtkErrorMacro("Could not create glyph source: ArrowSource");
      }
    
    // ===== Cone
    pvs = this->CreatePVSource("ConeSource", "GlyphSources", 0, 0);
    if (pvs)
      {
      pvs->IsPermanentOn();
      pvs->Accept(1);
      pvs->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
      ostrstream s;
      s << "GetPVSource GlyphSources " << pvs->GetName() << ends;
      pvs->GetTraceHelper()->SetReferenceCommand(s.str());
      s.rdbuf()->freeze(0);
      }
    else
      {
      vtkErrorMacro("Could not create glyph source: ConeSource");
      }
    
    // ==== Line
    pvs = this->CreatePVSource("LineSource", "GlyphSources", 0, 0);
    if (pvs)
      {
      pvs->IsPermanentOn();
      pvs->Accept(1);
      pvs->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
      ostrstream s;
      s << "GetPVSource GlyphSources " << pvs->GetName() << ends;
      pvs->GetTraceHelper()->SetReferenceCommand(s.str());
      s.rdbuf()->freeze(0);
      }
    else
      {
      vtkErrorMacro("Could not create glyph source: LineSource");
      }

    
    // ===== Sphere
    pvs = this->CreatePVSource("SphereSource", "GlyphSources", 0, 0);
    if (pvs)
      {
      pvs->IsPermanentOn();
      pvs->Accept(1);
      pvs->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
      ostrstream s;
      s << "GetPVSource GlyphSources " << pvs->GetName() << ends;
      pvs->GetTraceHelper()->SetReferenceCommand(s.str());
      s.rdbuf()->freeze(0);
      }
    else
      {
      vtkErrorMacro("Could not create glyph source: SphereSource");
      }

    // ===== Glyph
    pvs = this->CreatePVSource("GlyphSource2D", "GlyphSources", 0, 0);
    if (pvs)
      {
      pvs->IsPermanentOn();
      pvs->Accept(1);
      pvs->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
      ostrstream s;
      s << "GetPVSource GlyphSources " << pvs->GetName() << ends;
      pvs->GetTraceHelper()->SetReferenceCommand(s.str());
      s.rdbuf()->freeze(0);
      }
    else
      {
      vtkErrorMacro("Could not create glyph source: GlyphSource2D");
      }
    }
  else
    {
    char* str = getenv("PV_INTERFACE_PATH");
    if (str)
      {
      this->ReadSourceInterfacesFromDirectory(str);
      }
    }

  // Show glyph sources in menu.
  this->UpdateSelectMenu();
  // Show the sources (in Advanced).
  this->UpdateSourceMenu();

  // Update preferences
  if (use_splash)
    {
    pvApp->GetSplashScreen()->SetProgressMessage("Creating UI (preferences)...");
    }

  // Update toolbar aspect
  this->UpdateToolbarState();

  // Make the 3D View Settings the current one.
  this->Script("%s invoke \"%s\"", 
               this->GetViewMenu()->GetWidgetName(),
               VTK_PV_VIEW_MENU_LABEL);

  // Create per-node log files.
  if (pvApp->GetIntRegistryValue(2,"RunTime",
                                 VTK_PV_ASI_CREATE_LOG_FILES_REG_KEY))
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID()
           << "CreateLogFile"
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::DATA_SERVER|vtkProcessModule::RENDER_SERVER, stream);
    }
  
  if ( this->MainView )
    {
    this->MainView->SetupCameraManipulators();     
    }

  this->DeIconify();

  // Enable Drag&Drop from files if tkdnd is available
  if (!app->EvaluateBooleanExpression("catch {package require tkdnd}"))
    {
    this->Script("dnd bindtarget %s Files <Drop> {%s Open %%D}",
                 this->GetWidgetName(), this->GetTclName());
    }
  
  // Update the enable state
  this->UpdateEnableState();
  
  // Lets see if this works.
  this->Script("pack %s -side right -anchor se", this->ToolbarMenuButton->GetWidgetName());

  if ( ! this->TimerLogDisplay )
    {
    this->TimerLogDisplay = vtkPVTimerLogDisplay::New();
    this->TimerLogDisplay->SetTitle("Performance Log");
    this->TimerLogDisplay->SetMasterWindow(this);
    this->TimerLogDisplay->Create(this->GetPVApplication());
    }
  
}

//----------------------------------------------------------------------------
void vtkPVWindow::UpdateStatusImage()
{
  if (!this->StatusImage || !this->StatusImage->IsCreated())
    {
    return;
    }

  this->Superclass::UpdateStatusImage();

  kwsys_stl::string image_name(
    this->Script("%s cget -image", this->StatusImage->GetWidgetName()));

  // Update status image
  
  if (!vtkKWTkUtilities::UpdatePhoto(
        this->StatusImage->GetApplication(),
        image_name.c_str(),
        image_PVLogoSmall, 
        image_PVLogoSmall_width, 
        image_PVLogoSmall_height,
        image_PVLogoSmall_pixel_size,
        image_PVLogoSmall_buffer_length))
    {
    vtkWarningMacro("Error updating status image!" << image_name.c_str());
    }
}
//-----------------------------------------------------------------------------
void vtkPVWindow::AcceptCurrentSource()
{
  if (this->CurrentPVSource)
    {
    this->CurrentPVSource->AcceptCallback();
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
  this->GetTraceHelper()->AddEntry("$kw(%s) SetCenterOfRotation %f %f %f",
                      this->GetTclName(), x, y, z);
  
  this->CenterXEntry->SetValue(x);
  this->CenterYEntry->SetValue(y);
  this->CenterZEntry->SetValue(z);
  this->CameraStyle3D->SetCenterOfRotation(x, y, z);
  this->CameraStyle2D->SetCenterOfRotation(x, y, z);
  
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->CenterAxesProxy->GetProperty("Position"));
  if (!dvp)
    {
    vtkErrorMacro("CenterAxesProxy does not have property Position");
    return;
    }
  dvp->SetElement(0, x);
  dvp->SetElement(1, y);
  dvp->SetElement(2, z);
  this->CenterAxesProxy->UpdateVTKObjects(); 
  this->MainView->EventuallyRender();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::HideCenterActor()
{
  this->Script("%s configure -image PVShowCenterButton", 
               this->HideCenterButton->GetWidgetName() );
  this->HideCenterButton->SetBalloonHelpString(
    "Show the center of rotation to the center of the current data set.");

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CenterAxesProxy->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("CenterAxesProxy does not have property Visibility");
    return;
    }
  ivp->SetElement(0, 0);
  this->CenterAxesProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ShowCenterActor()
{
  if (this->CenterActorVisibility)
    {
    this->Script("%s configure -image PVHideCenterButton", 
      this->HideCenterButton->GetWidgetName() );
    this->HideCenterButton->SetBalloonHelpString(
      "Hide the center of rotation to the center of the current data set.");

    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->CenterAxesProxy->GetProperty("Visibility"));
    if (!ivp)
      {
      vtkErrorMacro("CenterAxesProxy does not have property Visibility");
      return;
      }
    ivp->SetElement(0, 1);
    this->CenterAxesProxy->UpdateVTKObjects();
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

  this->GetTraceHelper()->AddEntry("$kw(%s) ToggleCenterActorCallback", this->GetTclName());

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
  this->CenterXEntry->SetValue(center[0]);
  this->CenterYEntry->SetValue(center[1]);
  this->CenterZEntry->SetValue(center[2]);
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
    pvs = static_cast<vtkPVSource*>( it->GetCurrentObject() );
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
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->CenterAxesProxy->GetProperty("Scale"));
  if (!dvp)
    {
    vtkErrorMacro("CenterAxesProxy does not have property Scale");
    return;
    }
      
  if ((! first) && (bounds[0] <= bounds[1]) && 
      (bounds[2] <= bounds[3]) && (bounds[4] <= bounds[5]))
    {
    dvp->SetElements3(0.25 * (bounds[1]-bounds[0]),
      0.25 * (bounds[3]-bounds[2]),  0.25 * (bounds[5]-bounds[4]));
    }
  else
    {
    dvp->SetElements3(1,1,1);
    this->MainView->ResetCamera();
    }
  this->CenterAxesProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ChangeInteractorStyle(int index)
{
  int pick_toolbar_vis = 0;
  
  switch (index)
    {
    case 1:
      this->TranslateCameraButton->SetState(0);
      // Camera styles are not duplicated on satellites.
      // Cameras are synchronized before each render.
      this->Interactor->SetInteractorStyle(this->CameraStyle3D);
      pick_toolbar_vis = 1;
      this->ResizeCenterActor();
      this->ShowCenterActor();
      break;
    case 2:
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

  this->Superclass::SetToolbarVisibility(
    this->PickCenterToolbar, VTK_PV_TOOLBARS_CAMERA_LABEL, pick_toolbar_vis);
  this->MainView->EventuallyRender();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::MouseAction(int action,int button, 
                              int x,int y, int shift,int control)
{
  if ( !this->MainView->GetEnabled()  )
    {
    return;
    }
  if ( action == 0 )
    {
    if (button == 1)
      {
      this->Interactor->OnLeftPress(x, y, control, shift);
      }
    else if (button == 2)
      {
      this->Interactor->OnMiddlePress(x, y, control, shift);
      }
    else if (button == 3)
      {
      this->Interactor->OnRightPress(x, y, control, shift);
      }
    }
  else if ( action == 1 )
    {
    if (button == 1)
      {
      this->Interactor->OnLeftRelease(x, y, control, shift);
      }
    else if (button == 2)
      {
      this->Interactor->OnMiddleRelease(x, y, control, shift);
      }
    else if (button == 3)
      {
      this->Interactor->OnRightRelease(x, y, control, shift);
      }    
    vtkCamera* cam = this->MainView->GetRenderer()->GetActiveCamera();
    //float* parallelScale = cam->GetParallelScale();
    double* position      = cam->GetPosition();
    double* focalPoint    = cam->GetFocalPoint();
    double* viewUp        = cam->GetViewUp();

    this->GetTraceHelper()->AddEntry(
      "$kw(%s) SetCameraState "
      "%.3lf %.3lf %.3lf  %.3lf %.3lf %.3lf  %.3lf %.3lf %.3lf", 
      this->MainView->GetTclName(), 
      position[0], position[1], position[2], 
      focalPoint[0], focalPoint[1], focalPoint[2], 
      viewUp[0], viewUp[1], viewUp[2]);
    }
  else
    { 
    this->Interactor->OnMove(x, y);
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::KeyAction(char keyCode, int x, int y)
{
  if ( !this->MainView->GetEnabled()  )
    {
    return;
    }
  this->Interactor->OnKeyPress(keyCode, x, y);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::Configure(int vtkNotUsed(width), int vtkNotUsed(height))
{
  this->MainView->Configured();
  // The above Configured call could have changed the size of the render
  // window, so get the size from there instead of using the input width and
  // height.
  int *size = this->MainView->GetRenderWindowSize();
  this->Interactor->UpdateSize(size[0], size[1]);
  this->Interactor->ConfigureEvent();
}

//-----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::GetPVSource(const char* listname, const char* sourcename)
{
  vtkPVSourceCollection* col = this->GetSourceList(listname);
  if (col)
    {    
    vtkPVSource *pvs;
    vtkCollectionIterator *it = col->NewIterator();
    it->InitTraversal();
    while ( !it->IsDoneWithTraversal() )
      {
      pvs = static_cast<vtkPVSource*>( it->GetCurrentObject() );
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
  this->MainView = view;
  this->MainView->SetParent(this->LowerFrame->GetFrame1());
  this->MainView->SetPropertiesParent(this->GetPropertiesParent());
  this->MainView->SetParentWindow(this);
  this->MainView->Create(this->GetApplication(),"-width 200 -height 200");
  this->MainView->CreateRenderObjects(pvApp);
  this->MainView->MakeSelected();
  this->MainView->Select(this);
  this->MainView->ShowViewProperties();
  this->MainView->SetupBindings();
  this->MainView->Register(this);
  
  this->CameraStyle3D->SetCurrentRenderer(this->MainView->GetRenderer());
  this->CameraStyle2D->SetCurrentRenderer(this->MainView->GetRenderer());
  
  vtkPVInteractorStyleControl *iscontrol3D = view->GetManipulatorControl3D();
  iscontrol3D->SetManipulatorCollection(
    this->CameraStyle3D->GetCameraManipulators());
  vtkPVInteractorStyleControl *iscontrol2D = view->GetManipulatorControl2D();
  iscontrol2D->SetManipulatorCollection(
    this->CameraStyle2D->GetCameraManipulators());
  
  this->Script( "pack %s -expand yes -fill both", 
                this->MainView->GetWidgetName());  

  int menu_idx = this->GetHelpMenuIndex();
  this->HelpMenu->InsertSeparator(menu_idx++);
  this->HelpMenu->InsertCommand(
    menu_idx, "Play Demo", this, "PlayDemo", 0);
}


//-----------------------------------------------------------------------------
void vtkPVWindow::PlayDemo()
{ 
  vtkPVApplication* pvApp = this->GetPVApplication();
  pvApp->PlayDemo(0);
}


//-----------------------------------------------------------------------------
int vtkPVWindow::CheckIfFileIsReadable(const char* fileName)
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkClientServerStream stream;
  if(!this->ServerFileListingID.ID)
    {
    this->ServerFileListingID = 
      pm->NewStreamObject("vtkPVServerFileListing", stream);
    }
  stream << vtkClientServerStream::Invoke
         << this->ServerFileListingID << "FileIsReadable"<< fileName
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  int readable = 0;
  if(!pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &readable))
    {
    vtkErrorMacro("Error checking whether file is readable on server.");
    }
  return readable;
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
      vtkKWMessageDialog::PopupMessage(this->GetApplication(), this,
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

  vtkKWLoadSaveDialog* loadDialog = this->GetPVApplication()->NewLoadSaveDialog();
  this->RetrieveLastPath(loadDialog, "OpenPath");
  loadDialog->Create(this->GetApplication(),0);
  loadDialog->SetParent(this);
  loadDialog->SetTitle("Open ParaView File");
  loadDialog->SetDefaultExtension(".vtp");
  loadDialog->SetFileTypes(str.str());
  str.rdbuf()->freeze(0);  
  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( loadDialog->Invoke() )
    {
    openFileName = 
      kwsys::SystemTools::DuplicateString(loadDialog->GetFileName());
    }
  this->SetEnabled(enabled);
  
  // Store last path
  if ( openFileName && strlen(openFileName) > 0 )
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
int vtkPVWindow::Open(char *openFileNameUnSafe, int store)
{
  // Clean filename 
  // (Ex: {} are added when a filename with a space is dropped on ParaView

  char *openFileName = 
    kwsys::SystemTools::RemoveChars(openFileNameUnSafe, "{}");

  if (!this->CheckIfFileIsReadable(openFileName))
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
    delete [] openFileName;
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
        this->AddRecentFile(openFileName, this, str.str());
        str.rdbuf()->freeze(0);
        }
      it->Delete();
      delete [] openFileName;
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
    if ( vtkKWMessageDialog::PopupOkCancel(this->GetApplication(), this,
                                           "Open Error",  error.str(),
                                           vtkKWMessageDialog::ErrorIcon |
                                           vtkKWMessageDialog::CancelDefault |
                                           vtkKWMessageDialog::Beep ) )
      {
      vtkPVSelectCustomReader* dialog = vtkPVSelectCustomReader::New();
      vtkPVReaderModule* reader = dialog->SelectReader(this, openFileName);
      if ( !reader || this->OpenWithReader(openFileName, reader) != VTK_OK )
        {
        ostrstream errorstr;
        errorstr << "Can not open file " << openFileName << " for reading." << ends;
        if (this->UseMessageDialog)
          {
          vtkKWMessageDialog::PopupMessage(
            this->GetApplication(), this, "Open Error", errorstr.str(), 
            vtkKWMessageDialog::ErrorIcon | vtkKWMessageDialog::Beep);
          }
        else
          {
          vtkErrorMacro(<<errorstr);
          }
        errorstr.rdbuf()->freeze(0);
        }
      else if ( store )
        {
        ostrstream str;
        str << "OpenCustom \"" << reader->GetModuleName() << "\"" <<ends;
        this->AddRecentFile(openFileName, this, str.str());
        str.rdbuf()->freeze(0);
        dialog->Delete();
        delete [] openFileName;
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

  delete [] openFileName;
  return VTK_ERROR;
}

//-----------------------------------------------------------------------------
vtkPVReaderModule* vtkPVWindow::InitializeReadCustom(const char* proto,
                                                     const char* fileName)
{
  if ( !proto || strlen(proto) == 0 || 
       !fileName || strlen(fileName) == 0 )
    {
    return 0;
    }

  vtkLinkedListIterator<vtkPVReaderModule*>* it = 
    this->ReaderList->NewIterator();
  while(!it->IsDoneWithTraversal())
    {
    vtkPVReaderModule* rm = 0;
    int retVal = it->GetData(rm);
    if (retVal == VTK_OK && 
        rm && rm->GetModuleName() && !strcmp(rm->GetModuleName(), proto))
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
  this->GetTraceHelper()->AddEntry(
    "set kw(%s) [$kw(%s) InitializeReadCustom \"%s\" \"%s\"]", 
    clone->GetTclName(), this->GetTclName(), proto->GetModuleName(), fileName);

  if (clone)
    {
    proto->RegisterProxy(0, clone);
    }
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
    this->GetTraceHelper()->AddEntry(
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
  this->GetTraceHelper()->AddEntry(
    "$kw(%s) FinalizeRead $kw(%s) \"%s\"", 
    this->GetTclName(), clone->GetTclName(), fileName);

  return clone->Finalize(fileName);
}

//-----------------------------------------------------------------------------
int vtkPVWindow::OpenCustom(const char* reader, const char* filename)
{
  if ( !reader || strlen(reader) == 0 || 
       !filename || strlen(filename) == 0 )
    {
    return VTK_ERROR;
    }
  vtkLinkedListIterator<vtkPVReaderModule*>* it = 
    this->ReaderList->NewIterator();
  while(!it->IsDoneWithTraversal())
    {
    vtkPVReaderModule* rm = 0;
    int retVal = it->GetData(rm);
    if (retVal == VTK_OK && 
        rm && rm->GetModuleName() && !strcmp(rm->GetModuleName(), reader) &&
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
  clone->GrabFocus();
  this->UpdateEnableState();
  if (retVal != VTK_OK)
    {
    return retVal;
    }
  int timesteps = clone->GetNumberOfTimeSteps(); 
  if (timesteps > 0)
    {
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
        this->GetApplication(), this, "Error Saving File", 
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
  this->GetTraceHelper()->AddEntry("$kw(%s) WriteVTKFile \"%s\" %d", this->GetTclName(),
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
      this->GetApplication(), this, "Error Saving File", 
      "No data set is selected.", 
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkProcessModule* pm = pvApp->GetProcessModule();

  vtkSMPart *part = this->GetCurrentPVSource()->GetPart();
  vtkPVDataInformation* info = part->GetDataInformation();

  // Instantiator does not work for static builds and VTK objects.
  vtkDataObject* data = pm->GetDataObjectOfType(info->GetDataClassName());

  // Check the number of processes.
  int parallel = (pm->GetNumberOfPartitions() > 1);
  int numParts = this->GetCurrentPVSource()->GetNumberOfParts();
  int writerFound = 0;
  
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
      if(!writerFound)
        {
        writerFound = 1;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();
  
  // Make sure we have at least one writer.
  if(!writerFound)
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
      this->GetApplication(), this, "Error Saving File", 
      msg.str(),
      vtkKWMessageDialog::ErrorIcon);
    msg.rdbuf()->freeze(0);
    return;
    }
  
  typesStr << ends;
  char* types = typesStr.str();
  
  vtkKWLoadSaveDialog* saveDialog = this->GetPVApplication()->NewLoadSaveDialog();
  
  this->RetrieveLastPath(saveDialog, "SaveDataFile");
  saveDialog->SaveDialogOn();
  saveDialog->SetParent(this);
  saveDialog->SetTitle(VTK_PV_SAVE_DATA_MENU_LABEL);
  saveDialog->SetFileTypes(types);
  delete [] types;
  saveDialog->Create(this->GetApplication(), 0);
  // Ask the user for the filename.

  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( saveDialog->Invoke() && saveDialog->GetFileName() &&
       strlen(saveDialog->GetFileName()) > 0)
    {
    const char* filename = saveDialog->GetFileName();
    
    // If the current source is a reader and can provide time steps, ask
    // the user whether to write the whole time series.
    int timeSeries = 0;
    vtkPVReaderModule* reader =
      vtkPVReaderModule::SafeDownCast(this->GetCurrentPVSource());
    if(reader && (reader->GetNumberOfTimeSteps() > 1) &&
       vtkKWMessageDialog::PopupYesNo(
         this->GetApplication(), this, "Timesteps",
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
      dlg->Create(this->GetApplication(), "");
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
  vtkDataSet* data = NULL;
  vtkSMPart *part = this->GetCurrentPVSource()->GetPart();
  vtkPVDataInformation* info = part->GetDataInformation();
  if (info->DataSetTypeIsA("vtkImageData"))
    {
    data = vtkImageData::New();
    }
  else if (info->DataSetTypeIsA("vtkStructuredPoints"))
    {
    data = vtkStructuredPoints::New();
    }
  else if (info->DataSetTypeIsA("vtkStructuredGrid"))
    {
    data = vtkStructuredGrid::New();
    }
  else if (info->DataSetTypeIsA("vtkRectilinearGrid"))
    {
    data = vtkRectilinearGrid::New();
    }
  else if (info->DataSetTypeIsA("vtkPolyData"))
    {
    data = vtkPolyData::New();
    }
  else if (info->DataSetTypeIsA("vtkUnstructuredGrid"))
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
      if(kwsys::SystemTools::StringEndsWith(fileName, wm->GetExtension()))
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
void vtkPVWindow::SaveSMState()
{
  vtkKWLoadSaveDialog* exportDialog = vtkKWLoadSaveDialog::New();
  this->RetrieveLastPath(exportDialog, "SaveSMStatePath");
  exportDialog->SetParent(this);
  exportDialog->Create(this->GetApplication(),0);
  exportDialog->SaveDialogOn();
  exportDialog->SetTitle("Save SM State");
  exportDialog->SetDefaultExtension(".pvsm");
  exportDialog->SetFileTypes("{{ParaView Server Manager State} {.pvsm}} {{All Files} {*}}");
  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( exportDialog->Invoke() && 
       exportDialog->GetFileName() &&
       strlen(exportDialog->GetFileName())>0)
    {
    this->SaveSMState(exportDialog->GetFileName());
    this->SaveLastPath(exportDialog, "SaveSMStatePath");
    }
  this->SetEnabled(enabled);
  exportDialog->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SaveSMState(const char *filename)
{
  vtkSMProxyManager* proxm = vtkSMObject::GetProxyManager();
  proxm->SaveState(filename);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SaveBatchScript()
{
  vtkKWLoadSaveDialog* exportDialog = vtkKWLoadSaveDialog::New();
  this->RetrieveLastPath(exportDialog, "SaveBatchLastPath");
  exportDialog->SetParent(this);
  exportDialog->Create(this->GetApplication(),0);
  exportDialog->SaveDialogOn();
  exportDialog->SetTitle("Save Batch Script");
  exportDialog->SetDefaultExtension(".pvb");
  exportDialog->SetFileTypes("{{ParaView Batch Script} {.pvb}} {{All Files} {*}}");
  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( exportDialog->Invoke() && 
       exportDialog->GetFileName() &&
       strlen(exportDialog->GetFileName())>0)
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

  int pos = strlen(fname)-1;
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
  this->SaveBatchScript(filename, 
    dialog->GetOffScreen(),
    dialog->GetImagesFileName(),
    dialog->GetGeometryFileName());
  dialog->Delete();
  dialog = NULL;
}

void vtkPVWindow::SaveBatchScript(const char *filename, int offScreenFlag, const char* imageFileName, const char* vtkNotUsed(geometryFileName))
{
  vtkPVSource *pvs;
  int animationFlag = 0;
  ofstream* file;

  // We may want different questions if there is no animation.
  const char* extension = 0;
  const char* writerName = 0;
  if (imageFileName && strlen(imageFileName) > 0)
    {
    extension = this->ExtractFileExtension(imageFileName);
    if ( !extension)
      {
      vtkKWMessageDialog::PopupMessage(this->GetApplication(), this,
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
        vtkKWMessageDialog::PopupMessage(this->GetApplication(), this,
                                         "Error",  err.str(),
                                         vtkKWMessageDialog::ErrorIcon);
        err.rdbuf()->freeze(0);
        imageFileName = NULL;
        }
      }
    }
  // Should I continue to save the batch script if it
  // does not save an image or geometry?

  file = new ofstream(filename, ios::out);
  if (file->fail())
    {
    vtkErrorMacro("Could not open file " << filename);
    delete file;
    return;
    }

  *file << "# ParaView Version " 
        << this->GetPVApplication()->GetMajorVersion()
        << "." << this->GetPVApplication()->GetMinorVersion() << "\n\n";


  *file << endl << "#Initialization" << endl;

  *file << endl << "vtkSMObject foo" << endl;
  *file << "set proxyManager [foo GetProxyManager]" << endl;

  *file << endl << "set smApplication [foo GetApplication]" << endl;
  vtkSMApplication* sma =
    this->GetPVApplication()->GetSMApplication();
  unsigned int numConfFiles = sma->GetNumberOfConfigurationFiles();
  for (unsigned int idx=0; idx<numConfFiles; idx++)
    {
    const char* fname;
    const char* dir;
    sma->GetConfigurationFile(idx, fname, dir);
    *file << "$smApplication AddConfigurationFile " << fname << " " << dir 
          << endl;
    }
  *file << "$smApplication ParseConfigurationFiles" << endl;

  *file << "foo Delete" << endl << endl;

  *file << "vtkSMProperty foo" << endl;
  *file << "foo SetCheckDomains 0" << endl;
  *file << "foo Delete" << endl << endl;

  // Save out the VTK data pipeline.
  vtkArrayMapIterator<const char*, vtkPVSourceCollection*>* it =
    this->SourceLists->NewIterator();
  // Mark all sources as not visited.
  while( !it->IsDoneWithTraversal() )
    {    
    vtkPVSourceCollection* col = 0;
    if (it->GetData(col) == VTK_OK && col)
      {
      vtkCollectionIterator *collIt = col->NewIterator();
      collIt->InitTraversal();
      while ( !collIt->IsDoneWithTraversal() )
        {
        pvs = static_cast<vtkPVSource*>(collIt->GetCurrentObject()); 
        pvs->SetVisitedFlag(0);
        collIt->GoToNextItem();
        }
      collIt->Delete();
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
    pvs = static_cast<vtkPVSource*>(cit->GetCurrentObject()); 
    pvs->SaveInBatchScript(file);
    cit->GoToNextItem();
    }
  cit->Delete();
  cit = 0;
  this->CenterAxesProxy->SaveInBatchScript(file);
  // Save the renderer stuff.
  this->GetMainView()->SaveInBatchScript(file);
  if (offScreenFlag)
    {
    *file << "  [$Ren1 GetProperty OffScreenRendering] SetElement 0 1\n";
    }    
  else
    {
    *file << "  [$Ren1 GetProperty OffScreenRendering] SetElement 0 0\n";
    }

  this->AnimationManager->SaveInBatchScript(file);
// TODO replace this
//   if (geometryFileName)
//     {
//     //*file << "if {$numberOfProcs > 1} {\n";
//     //*file << "\tvtkXMLPPolyDataWriter GeometryWriter\n";
//     //*file << "\tGeometryWriter SetNumberOfPieces $numberOfProcs" << endl;
//     //*file << "\tGeometryWriter SetStartPiece $myProcId\n";
//     //*file << "\tGeometryWriter SetEndPiece $myProcId\n";
//     //*file << "} else {\n";
//     //*file << "\tvtkXMLPolyDataWriter GeometryWriter\n";
//     //*file << "}\n";
//     //*file << "GeometryWriter SetDataModeToBinary" << endl;
//     //*file << "GeometryWriter EncodeAppendedDataOff" << endl;
//     *file << "vtkCollectPolyData CollectionFilter\n";
//     *file << "vtkPolyData TempPolyData\n";
//     *file << "vtkXMLPolyDataWriter GeometryWriter\n";
//     *file << "\tGeometryWriter SetInput TempPolyData\n";
//     *file << "[CollectionFilter GetOutput] SetUpdateNumberOfPieces $numberOfProcs\n";
//     *file << "[CollectionFilter GetOutput] SetUpdatePiece $myProcId\n";

//     }

  *file << endl;
  *file << "set saveState 0" << endl;
  *file << "for {set i  0} {$i < [expr $argc - 1]} {incr i} {" << endl;
  *file << "  if {[lindex $argv $i] == \"-XML\"} {" << endl;
  *file << "    set saveState 1" << endl;
  *file << "    set stateName [lindex $argv [expr $i + 1]]" << endl;
  *file << "  }" << endl;
  *file << "}" << endl;

  
  *file << "if { $saveState } {" << endl;
  *file << "   $Ren1 UpdateVTKObjects" << endl;
  *file << "   $proxyManager SaveState $stateName" << endl;
  *file << "} else {" << endl;

  if (animationFlag )
    {
    }
  else
    {
    *file << endl << "$Ren1 UpdateVTKObjects" << endl;
    if (imageFileName && *imageFileName && writerName)
      {
      *file << "set inBatch 0" << endl;
      *file << "for {set i  1} {$i < [expr $argc]} {incr i} {" << endl;
      *file << "  if {[lindex $argv $i] == \"-BT\"} {" << endl;
      *file << "    set inBatch 1" << endl;
      *file << "  }" << endl;
      *file << "}" << endl;
      *file << "if { $inBatch } {" << endl;
//      *file << "  set xsize [[$Ren1 GetProperty Size] GetElement 0]" << endl;
//      *file << "  set ysize [[$Ren1 GetProperty Size] GetElement 1]" << endl;
//      *file << "  $Ren1 TileWindows [expr $xsize+30] [expr $ysize+30] 2" << endl;
      *file << "  [$Ren1 GetProperty RenderWindowSize] "
        " SetElement 0 300" << endl;
      *file << "  [$Ren1 GetProperty RenderWindowSize] "
        " SetElement 1 300" << endl;
      *file << "  $Ren1 UpdateVTKObjects" << endl;
      *file << "}" << endl;
      *file << "$Ren1 StillRender" << endl;
      *file 
        << "$Ren1 WriteImage {" << imageFileName << "} " << writerName
        << "\n";
      }
    else
      {
      *file << "$Ren1 StillRender" << endl;
      }

    }


  *file << "}" << endl;
  *file << endl;

  *file << "$proxyManager UnRegisterProxies" << endl;
  *file << endl;
   
//   else
//     { // Just do one frame.
//     if (imageFileName)
//       {
//       *file << "RenWin1 Render\n";
//       *file << "compManager Composite\n";
//       *file << "if {$myProcId == 0} {\n";
//       *file << "\t" << "ImageWriter SetFileName {" << imageFileName << "}\n";
//       *file << "\t" << "ImageWriter Write\n";
//       *file << "}\n\n";
//       }

// TODO replace this
//     if (geometryFileName)
//       {
//       this->SaveGeometryInBatchFile(file, geometryFileName, -1);
//       }
//     }
//   *file << "vtkCommand DeleteAllObjects\n";

  file->flush();
  if (file->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this, "Write Error",
      "There is insufficient disk space to save this batch script. The file "
      "will be deleted.");
    file->close();
    unlink(filename);
    }

  delete file;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SaveState()
{
  vtkKWLoadSaveDialog* exportDialog = vtkKWLoadSaveDialog::New();
  this->RetrieveLastPath(exportDialog, "SaveStateLastPath");
  exportDialog->SetParent(this);
  exportDialog->Create(this->GetApplication(),0);
  exportDialog->SaveDialogOn();
  exportDialog->SetTitle("Save State");
  exportDialog->SetDefaultExtension(".pvs");
  exportDialog->SetFileTypes("{{ParaView State} {.pvs}} {{All Files} {*}}");
  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( exportDialog->Invoke() && 
       exportDialog->GetFileName() &&
       strlen(exportDialog->GetFileName())>0)
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

  *file << "set kw(" << this->GetTclName() << ") [$Application GetMainWindow]" << endl;
  *file << "set kw(" << this->GetMainView()->GetTclName() 
        << ") [$kw(" << this->GetTclName() << ") GetMainView]" << endl;

  *file << "set kw(" << this->AnimationManager->GetTclName()
    << ") [$kw(" << this->GetTclName() << ") GetAnimationManager]" << endl;

  vtkInteractorObserver *style = this->Interactor->GetInteractorStyle();
  if (style == this->CameraStyle3D)
    {
    *file << "[$kw(" << this->GetTclName()
          << ") GetRotateCameraButton] SetState 1" << endl;
    *file << "$kw(" << this->GetTclName() << ") ChangeInteractorStyle 1"
          << endl;
    }
  else if (style == this->CameraStyle2D)
    {
    *file << "[$kw(" << this->GetTclName()
          << ") GetTranslateCameraButton] SetState 1" << endl;
    *file << "$kw(" << this->GetTclName() << ") ChangeInteractorStyle 2"
          << endl;
    }
  else if (style == this->CenterOfRotationStyle)
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeInteractorStyle 4"
          << endl;
    }

  if (this->PackageFiles->GetNumberOfItems() > 0)
    {
    vtkLinkedListIterator<const char*>* it = this->PackageFiles->NewIterator();
    while (!it->IsDoneWithTraversal())
      {
      const char* name = 0;
      if (it->GetData(name) == VTK_OK && name)
        {
        *file << "$kw(" << this->GetTclName() << ") OpenPackage \""
              << name << "\"" << endl;
        }
      it->GoToNextItem();
      }
    it->Delete();
    *file << endl;
    }

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
        pvs = static_cast<vtkPVSource*>(cit->GetCurrentObject()); 
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
  vtkCollectionIterator* cit;
  cit = modules->NewIterator();
  cit->InitTraversal();
  while ( !cit->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>(cit->GetCurrentObject()); 
    if(this->SaveVisibleSourcesOnlyFlag && pvs->GetVisibility())
      pvs->SaveState(file);
    else if(!this->SaveVisibleSourcesOnlyFlag)
      pvs->SaveState(file);
    cit->GoToNextItem();
    }
  cit->Delete();
  // Visibility has to be done in a second pass.
  cit = modules->NewIterator();
  cit->InitTraversal();
  while ( !cit->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>(cit->GetCurrentObject()); 
    if(this->SaveVisibleSourcesOnlyFlag && pvs->GetVisitedFlag())
      pvs->SaveStateVisibility(file);
    else if(!this->SaveVisibleSourcesOnlyFlag)
      pvs->SaveStateVisibility(file);
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

  // short term check to see if this is a lookmark being generated
  // in which case it doesn't support animation saving yet
  if(this->SaveVisibleSourcesOnlyFlag == 0)
    {
    // Save state of the new animation interface
    this->AnimationManager->SaveState(file);
    }

  //  Save state of the Volume Appearance editor
  this->VolumeAppearanceEditor->SaveState(file);

  // Save the center of rotation
  *file << "$kw(" << this->GetTclName() << ") SetCenterOfRotation "
        << this->CenterXEntry->GetValueAsFloat() << " "
        << this->CenterYEntry->GetValueAsFloat() << " "
        << this->CenterZEntry->GetValueAsFloat() << endl;
  
  file->flush();
  if (file->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this, "Write Error",
      "There is insufficient disk space to save the session state. The file "
      "will be deleted.");
    file->close();
    unlink(filename);
    }
  
  delete file;
  file = NULL;
}


//-----------------------------------------------------------------------------
void vtkPVWindow::UpdateSourceMenu()
{
  if ( (this->AnimationManager && this->AnimationManager->GetInPlay()) 
    || (this->AnimationManager && this->AnimationManager->GetInRecording())
    )
    {
    return;
    }

  // We do not want any buttons active when a source is half finished
  // (accept has not been called yet).
  if (this->CurrentPVSource && ! this->CurrentPVSource->GetInitialized())
    {
    return;
    }

  if (!this->SourceMenu)
    {
    vtkWarningMacro("Source menu does not exist. Can not update.");
    return;
    }

  // Remove all of the entries from the source menu to avoid
  // adding things twice.
  this->SourceMenu->DeleteAllMenuItems();

  // Build an ordered list of sources for the menu.
  vtkstd::map<vtkStdString, vtkStdString> sourceKeys;
  vtkstd::map<vtkStdString, vtkPVSource*> sourceValues;

  // Create all of the menu items for sources with no inputs.
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
      // Check if this is a source (or a toolbar module). We do not want to 
      // add those to the source lists.
      if (proto && proto->GetNumberOfInputProperties() == 0)
        {
        numSources++;
        it->GetKey(key);
        const char* menuName = proto->GetMenuName();
        if (!menuName)
          {
          menuName = key;
          }
        sourceKeys[menuName] = key;
        sourceValues[menuName] = proto;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Add the menu items in sorted order.
  vtkstd::map<vtkStdString, vtkStdString>::iterator ki = sourceKeys.begin();
  vtkstd::map<vtkStdString, vtkPVSource*>::iterator vi = sourceValues.begin();
  vtkstd::string methodAndArgs;
  while(ki != sourceKeys.end())
    {
    methodAndArgs = "CreatePVSource ";
    methodAndArgs += ki->second;
    this->SourceMenu->AddCommand(ki->first.c_str(), this,
                                 methodAndArgs.c_str(),
                                 vi->second->GetShortHelp());
    if (vi->second->GetToolbarModule())
      {
      this->EnableToolbarButton(ki->second.c_str());
      }
    ++ki;
    ++vi;
    }

  // If there are no filters, disable the menu.
  if (numSources > 0)
    {
    this->GetMenu()->SetState(VTK_PV_VTK_SOURCES_MENU_LABEL, 
                         vtkKWMenu::Normal);
    }
  else
    {
    this->GetMenu()->SetState(VTK_PV_VTK_SOURCES_MENU_LABEL, 
                         vtkKWMenu::Disabled);
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::UpdateFilterMenu()
{
  if ((this->AnimationManager && this->AnimationManager->GetInPlay()))
    {
    return;
    }

  if ( this->InDemo )
    {
    return;
    }
  if (!this->FilterMenu)
    {
    vtkWarningMacro("Filter menu does not exist. Can not update.");
    return;
    }

  // Remove all of the entries from the filter menu.
  this->FilterMenu->DeleteAllMenuItems();

  if ( !this->GetEnabled() )
    {
    this->FilterMenu->SetEnabled(0);
    return;
    }

  if (this->CurrentPVSource &&
      !this->CurrentPVSource->GetIsPermanent())
    {
    vtkPVDataInformation *pvdi = this->CurrentPVSource->GetDataInformation();
    if (pvdi->GetNumberOfPoints() <= 0)
      {
      this->FilterMenu->SetEnabled(0);
      return;
      }
    // Build an ordered list of filters for the menu.
    vtkstd::map<vtkStdString, vtkStdString> filterKeys;
    vtkstd::map<vtkStdString, vtkPVSource*> filterValues;

    // Add all the appropriate filters to the filter menu.
    vtkArrayMapIterator<const char*, vtkPVSource*>* it = 
      this->Prototypes->NewIterator();
    vtkPVSource* proto;
    const char* key = 0;
    while ( !it->IsDoneWithTraversal() )
      {
      proto = 0;
      if (it->GetData(proto) == VTK_OK)
        {
        // Check if this is an appropriate filter by comparing
        // it's input type with the current data object's type.
        if (proto && proto->GetInputProperty(0) )
          {
          it->GetKey(key);
          const char* menuName = proto->GetMenuName();
          if (!menuName)
            {
            menuName = key;
            }
          filterKeys[menuName] = key;
          filterValues[menuName] = proto;
          }
        }
      it->GoToNextItem();
      }
    it->Delete();

    // Add the menu items in sorted order.
    vtkstd::map<vtkStdString, vtkStdString>::iterator ki = filterKeys.begin();
    vtkstd::map<vtkStdString, vtkPVSource*>::iterator vi = filterValues.begin();
    vtkstd::string methodAndArgs;
    int numFilters = 0;
    while(ki != filterKeys.end())
      {
      methodAndArgs = "CreatePVSource ";
      methodAndArgs += ki->second;
      if (numFilters % 25 == 0 && numFilters > 0)
        {
        this->FilterMenu->AddGeneric("command", ki->first.c_str(), this,
                                     methodAndArgs.c_str(), "-columnbreak 1",
                                     vi->second->GetShortHelp());
        }
      else
        {
        this->FilterMenu->AddGeneric("command", ki->first.c_str(), this,
                                     methodAndArgs.c_str(), 0,
                                     vi->second->GetShortHelp());
        }
      if (!vi->second->GetInputProperty(0)->GetIsValidInput(
            this->CurrentPVSource, vi->second) ||
          !vi->second->GetNumberOfProcessorsValid())
        {
        this->FilterMenu->SetState(ki->first.c_str(), vtkKWMenu::Disabled);
        }
      else if (vi->second->GetToolbarModule())
        {
        this->EnableToolbarButton(ki->second.c_str());
        }
      ++ki;
      ++vi;
      ++numFilters;
      }

    // If there are no sources, disable the menu.
    if (numFilters > 0)
      {
      this->PropagateEnableState(this->FilterMenu);
      }
    else
      {
      this->FilterMenu->SetEnabled(0);
      }
    }
  else
    {
    // If there is no current data, disable the menu.
    this->FilterMenu->SetEnabled(0);
    }

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
    if (pvs->GetTraceHelper()->Initialize())
      {
      this->GetTraceHelper()->AddEntry(
        "$kw(%s) SetCurrentPVSourceCallback $kw(%s)", 
        this->GetTclName(), pvs->GetTclName());
      }
    }
  else
    {
    this->GetTraceHelper()->AddEntry(
      "$kw(%s) SetCurrentPVSourceCallback {}", this->GetTclName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetCurrentPVSource(vtkPVSource *pvs)
{

  if ( pvs && pvs == this->CurrentPVSource)
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

  // This will update the parameters.  
  // I doubt the conditional is still necessary.
  if (pvs)
    {
    this->ShowCurrentSourceProperties();
    }
    
  // I was having problems with the parts being created too early 
  // (before accept was called).  Group was getting only one part when 
  // it should have two (two inputs selected form GUI).
  // This call seemed to trigger creation of the parts.
  //  This conditional will keep the parts from being created when
  // the half finished source is set to be the current.
  if (!pvs || pvs->GetInitialized())
    {
    this->UpdateEnableState();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::UpdateAnimationInterface()
{
  if (this->AnimationManager)
    {
    this->AnimationManager->Update();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::AddDefaultAnimation(vtkPVSource* pvSource)
{
  if (this->AnimationManager)
    {
    this->AnimationManager->Update();
    this->AnimationManager->AddDefaultAnimation(pvSource);
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
      this->MainView->UpdateNavigationWindow(this->CurrentPVSource, 0);
      this->UpdateSelectMenu();
      this->UpdateAnimationInterface();
      }
    }
}


//-----------------------------------------------------------------------------
vtkPVSourceCollection* vtkPVWindow::GetSourceList(const char* listname)
{
  vtkPVSourceCollection* col=0;
  if (this->SourceLists && this->SourceLists->GetItem(listname, col) == VTK_OK)
    {
    return col;
    }
  return 0;
}


//-----------------------------------------------------------------------------
void vtkPVWindow::ResetCameraCallback()
{

  this->GetTraceHelper()->AddEntry("$kw(%s) ResetCameraCallback", 
                                          this->GetTclName());

  if (this->ResetCameraButton->GetCheckButtonState("ViewAngle"))
    {
    this->MainView->StandardViewCallback(0,0,1);
    }
  if (this->ResetCameraButton->GetCheckButtonState("CenterOfRotation"))
    {
    this->ResetCenterCallback();
    }

  this->MainView->ResetCamera();
  this->MainView->EventuallyRender();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::UpdateSelectMenu()
{
  if ( this->AnimationManager && this->AnimationManager->GetInPlay())
    {
    return;
    }

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
      source = static_cast<vtkPVSource*>(it->GetCurrentObject());
      sprintf(methodAndArg, "SetCurrentPVSourceCallback %s", 
              source->GetTclName());
      char* label = this->GetPVApplication()->GetTextRepresentation(source);
      this->GlyphMenu->AddCommand(label, this, methodAndArg,
                                  source->GetSourceClassName());
      delete[] label;
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
      source = static_cast<vtkPVSource*>(it->GetCurrentObject());
      sprintf(methodAndArg, "SetCurrentPVSourceCallback %s", 
              source->GetTclName());
      char* label = this->GetPVApplication()->GetTextRepresentation(source);
      this->SelectMenu->AddCommand(label, this, methodAndArg,
                                   source->GetSourceClassName());
      delete[] label;
      it->GoToNextItem();
      }
    it->Delete();
    }
  
  if (numGlyphs > 0)
    {
    this->SelectMenu->AddCascade("Glyphs", this->GlyphMenu, 0,
                                 "Select one of the glyph sources.");  
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
  if ( this->InDemo )
    {
    return;
    }
  this->MainView->GetNavigationFrame()->EnabledOn();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DisableToolbarButtons()
{
  if ( this->InDemo )
    {
    return;
    }
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
  if ( this->InDemo )
    {
    return;
    }
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
  if ( this->InDemo )
    {
    return;
    }
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
  if ( this->InDemo )
    {
    return;
    }
  if (this->CurrentPVSource == 0)
    {
    return;
    }
  if (this->CurrentPVSource->GetInitialized() == 0)
    {
    this->DisableToolbarButtons();
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
void vtkPVWindow::ShowCurrentSourcePropertiesCallback()
{
  this->GetTraceHelper()->AddEntry(
    "$kw(%s) ShowCurrentSourcePropertiesCallback", this->GetTclName());

  this->ShowCurrentSourceProperties();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ShowCurrentSourceProperties()
{

  // Bring up the properties panel
  
  this->ShowProperties();

  if ( !this->GetViewMenu() )
    {
    return;
    }
  
  // We need to update the view-menu radio button too!

  this->GetViewMenu()->CheckRadioButton(
    this->GetViewMenu(), "Radio", VTK_PV_SOURCE_MENU_INDEX);

  this->MainView->GetSplitFrame()->UnpackSiblings();

  this->Script("pack %s -side top -fill both -expand t",
               this->MainView->GetSplitFrame()->GetWidgetName());

  if (!this->GetCurrentPVSource())
    {
    return;
    }

  this->GetCurrentPVSource()->ResetCallback();
  this->GetCurrentPVSource()->Pack();
}

//----------------------------------------------------------------------------
void vtkPVWindow::ShowAnimationPanes()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) ShowAnimationPanes",
    this->GetTclName());
  
  //Bring up the properties panel.
  this->ShowProperties();
  this->ShowHorizontalPane();
  
  // We need to update the properties-menu radio button too!
  this->GetViewMenu()->CheckRadioButton(
    this->GetViewMenu(), "Radio", VTK_PV_ANIMATION_MENU_INDEX+1);
  if (this->AnimationManager)
    {
    this->AnimationManager->ShowAnimationInterfaces();
    }
}

//----------------------------------------------------------------------------
int vtkPVWindow::GetHorizontalPaneVisibility()
{
  return (this->LowerFrame && this->LowerFrame->GetFrame2Visibility()) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkPVWindow::SetHorizontalPaneVisibility(int arg)
{
  this->GetTraceHelper()->AddEntry(
    "$kw(%s) SetHorizontalPaneVisibility %d",
    this->GetTclName(), arg);
  if (arg)
    {
    if (!this->GetHorizontalPaneVisibility())
      {
      this->LowerFrame->Frame2VisibilityOn();
      this->Script("%s entryconfigure 1 -label {%s}",
        this->GetWindowMenu()->GetWidgetName(),
        VTK_PV_HIDE_HORZPANE_LABEL);
      }
    }
  else
    {
    if (this->GetHorizontalPaneVisibility())
      {
      this->LowerFrame->Frame2VisibilityOff();
      this->Script("%s entryconfigure 1 -label {%s}",
        this->GetWindowMenu()->GetWidgetName(),
        VTK_PV_SHOW_HORZPANE_LABEL);
      }
    }
  this->GetApplication()->SetRegistryValue(2, "RunTime",
    "HorizontalPaneVisibility", "%d", arg);
}

//----------------------------------------------------------------------------
void vtkPVWindow::ToggleHorizontalPaneVisibilityCallback()
{
  this->SetHorizontalPaneVisibility(!this->GetHorizontalPaneVisibility());
}

//----------------------------------------------------------------------------
void vtkPVWindow::DisplayLookmarkManager()
{
#ifdef PARAVIEW_USE_LOOKMARKS
  if ( ! this->PVLookmarkManager )
    {
    this->PVLookmarkManager = vtkPVLookmarkManager::New();
    this->PVLookmarkManager->SetMasterWindow(this);
    this->PVLookmarkManager->Create(this->GetApplication());
    }
  
  this->PVLookmarkManager->Display();
#endif
}

//-----------------------------------------------------------------------------
vtkPVApplication *vtkPVWindow::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}


//-----------------------------------------------------------------------------
void vtkPVWindow::ShowTimerLog()
{
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
  exportDialog->Create(this->GetApplication(),0);
  exportDialog->SaveDialogOn();
  exportDialog->SetTitle("Save ParaView Trace");
  exportDialog->SetDefaultExtension(".pvs");
  exportDialog->SetFileTypes("{{ParaView Trace} {.pvs}} {{All Files} {*}}");
  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if ( exportDialog->Invoke() && 
       exportDialog->GetFileName() &&
       strlen(exportDialog->GetFileName())>0 &&
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

  if (!filename || strlen(filename) <= 0)
    {
    return 0;
    }

  if (trace && *trace)
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
    trace->open(pvApp->GetTraceFileName(), ios::out | ios::app );
    }
  
  newTrace.flush();
  if (newTrace.fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this, "Write Error",
      "There is insufficient disk space to save this session trace. The file "
      "will be deleted.");
    newTrace.close();
    unlink(filename);
    }
  
  return 1;
}

//-----------------------------------------------------------------------------
// Create a new data object/source by cloning a module prototype.
vtkPVSource *vtkPVWindow::CreatePVSource(const char* moduleName,
                                         const char* sourceList,
                                         int add_trace_entry,
                                         int grabFocus)
{
  vtkPVSource *pvs = 0;
  vtkPVSource* clone = 0;
  int success;

  if ( this->Prototypes->GetItem(moduleName, pvs) == VTK_OK ) 
    {
    pvs->SetSourceList(sourceList);

    // During CloneAndInitialize the CurrentPVSource is changed. Unfortunately
    // we need the OverideAutoAccept from pvs. That's why we do the following trick 
    // first we check if pvs has OverideAutoAccept set it to the CurrentPVSource
    // and then set it back (we need the temp CurrentPVSource since it is changed !)
    int overideautoaccept = 0;
    vtkPVSource *current = NULL;
    if( pvs->GetOverideAutoAccept() )
      {
      current = this->CurrentPVSource;
      overideautoaccept = current->GetOverideAutoAccept();
      current->SetOverideAutoAccept(1);
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
    // Put back the old value
    if( pvs->GetOverideAutoAccept() )
      {
      current->SetOverideAutoAccept(overideautoaccept);
      }

    if (success != VTK_OK)
      {
      this->EnableToolbarButtons();
      this->UpdateEnableState();
      vtkErrorMacro("Cloning operation for " << moduleName
                    << " failed.");
      return 0;
      }

    if (!clone)
      {
      this->EnableToolbarButtons();
      this->UpdateEnableState();
      return 0;
      }

    if (grabFocus)
      {
      clone->GrabFocus();
      }

    if (add_trace_entry)
      {
      if (clone->GetTraceHelper()->GetInitialized() == 0)
        { 
        if (sourceList)
          {
          this->GetTraceHelper()->AddEntry(
            "set kw(%s) [$kw(%s) CreatePVSource %s %s]", 
            clone->GetTclName(), this->GetTclName(),
            moduleName, sourceList);
          }
        else
          {
          this->GetTraceHelper()->AddEntry(
            "set kw(%s) [$kw(%s) CreatePVSource %s]", 
            clone->GetTclName(), this->GetTclName(),
            moduleName);
          }
        clone->GetTraceHelper()->SetInitialized(1);
        }
      }

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
  this->UpdateEnableState();

  return clone;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::LoadScript(const char *name)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());

  this->AddRecentFile(name, this, "LoadScript");

  pvApp->GetGUIClientOptions()->SetParaViewScriptName("tmp");
  this->vtkKWWindow::LoadScript(name);
  pvApp->GetGUIClientOptions()->SetParaViewScriptName(0);
}

//-----------------------------------------------------------------------------
int vtkPVWindow::OpenPackage()
{
  int res = 0;
  vtkKWLoadSaveDialog* loadDialog = vtkKWLoadSaveDialog::New();
  this->RetrieveLastPath(loadDialog, "PackagePath");
  loadDialog->SetParent(this);
  loadDialog->Create(this->GetApplication(),0);
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
  if ( !this->CheckIfFileIsReadable(openFileName) )
    {
    return VTK_ERROR;
    }

  this->ReadSourceInterfacesFromFile(openFileName);

  // Store last path
  if ( openFileName && strlen(openFileName) > 0 )
    {
    char *pth = kwsys::SystemTools::DuplicateString(openFileName);
    int pos = strlen(openFileName);
    // Strip off the file name
    while (pos && pth[pos] != '/' && pth[pos] != '\\')
      {
      pos--;
      }
    pth[pos] = '\0';
    // Store in the registry
    this->GetApplication()->SetRegistryValue(
      2, "RunTime", "PackagePath", pth);
    delete [] pth;
    }

  // Save the package load command in the trace file.
  this->GetTraceHelper()->AddEntry(
    "$kw(%s) OpenPackage \"%s\"", this->GetTclName(), openFileName);

  // Add the package to the list that will be saved to state and batch
  // files.
  this->PackageFiles->AppendItem(openFileName);

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
  this->Toolbar->UpdateWidgets();
  this->UpdateEnableState();
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
  this->Toolbar->UpdateWidgets();
  this->UpdateEnableState();
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
    int extPos = (file ? strlen(file) : 0) - 4;
    
    // Look for the ".xml" extension.
    if((extPos > 0) && !strcmp(file+extPos, ".xml"))
      {
      char* fullPath 
        = new char[strlen(file)+strlen(directory)+2];
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
    length = strlen(this->FileExtensions);
    }
  length += strlen(ext) + 5;
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
    length = strlen(this->FileDescriptions);
    }
  length += strlen(description) + strlen(ext) + 10;
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
  this->FileMenu->SetState(VTK_PV_OPEN_DATA_MENU_LABEL, vtkKWMenu::Normal);
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
  char *wmessage = kwsys::SystemTools::DuplicateString(message);
  this->InvokeEvent(vtkKWEvent::WarningMessageEvent, wmessage);
  delete [] wmessage;
  this->ErrorLogDisplay->AppendError(message);
  this->SetErrorIcon(vtkKWWindow::ERROR_ICON_RED);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ErrorMessage(const char* message)
{  
  cout << "ErrorMessage" << endl;
  this->Script("bell");
  this->CreateErrorLogDisplay();
  char *wmessage = kwsys::SystemTools::DuplicateString(message);
  this->InvokeEvent(vtkKWEvent::ErrorMessageEvent, wmessage);
  delete [] wmessage;
  this->ErrorLogDisplay->AppendError(message);
  this->SetErrorIcon(vtkKWWindow::ERROR_ICON_RED);
  cout << "ErrorMessage end" << endl;
  if ( this->GetPVApplication()->GetGUIClientOptions()->GetCrashOnErrors() )
    {
    vtkPVApplication::Abort();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::ErrorIconCallback()
{
  this->Superclass::ErrorIconCallback();
  this->ShowErrorLog();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SaveAnimation()
{
  if (this->AnimationManager)
    {
    this->AnimationManager->SaveAnimation();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SaveGeometry()
{
  if (this->AnimationManager)
    {
    this->AnimationManager->SaveGeometry();
    }
}

//-----------------------------------------------------------------------------
vtkCollection* vtkPVWindow::GetPVColorMaps()
{
  return this->PVColorMaps;
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
    cm = static_cast<vtkPVColorMap*>(it->GetCurrentObject());
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
  cm->SetNumberOfVectorComponents(numberOfComponents);
  cm->Create(this->GetPVApplication());
  cm->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
  cm->SetArrayName(parameterName);
  cm->SetScalarBarTitle(parameterName);
  cm->ResetScalarRangeInternal();

  this->PVColorMaps->AddItem(cm);
  cm->Delete();

  return cm;
}

//-----------------------------------------------------------------------------
// This is the shared volume appearance editor
vtkPVVolumeAppearanceEditor* vtkPVWindow::GetVolumeAppearanceEditor()
{
  if ( !this->VolumeAppearanceEditor && this->GetMainView() )
    {
    this->VolumeAppearanceEditor = vtkPVVolumeAppearanceEditor::New();
    this->VolumeAppearanceEditor->
      SetParent(this->GetMainView()->GetPropertiesParent());
    this->VolumeAppearanceEditor->
      SetPVRenderView(this->GetMainView());
    this->VolumeAppearanceEditor->Create(this->GetPVApplication());
    this->VolumeAppearanceEditor->GetTraceHelper()->SetReferenceHelper(
      this->GetTraceHelper());
    this->VolumeAppearanceEditor->GetTraceHelper()->SetReferenceCommand(
      "GetVolumeAppearanceEditor");
    }
  
  return this->VolumeAppearanceEditor;
}


//-----------------------------------------------------------------------------
void vtkPVWindow::AddManipulator(const char* rotypes, const char* name, 
                                 vtkPVCameraManipulator* pcm)
{
  if ( !pcm || !this->MainView )
    {
    return;
    }

  char *types = kwsys::SystemTools::DuplicateString(rotypes);
  char t[100];
  int res = 1;

  istrstream str(types);
  str.width(100);
  while(str >> t)
    {
    if ( !strcmp(t, "2D") )
      {
      this->MainView->GetManipulatorControl2D()->AddManipulator(name, pcm);
      }
    else if (!strcmp(t, "3D") )
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

  char *types = kwsys::SystemTools::DuplicateString(rotypes);
  char t[100];
  int res = 1;

  istrstream str(types);
  str.width(100);
  while(str >> t)
    {
    if ( !strcmp(t, "2D") )
      {
      this->MainView->GetManipulatorControl2D()->AddArgument(variable, 
                                                             name, widget);
      }
    else if (!strcmp(t, "3D") )
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
         this->GetApplication(), this, "DeleteAllTheSources",
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
  vtkPVSourceCollection* col = this->GetSourceList("Sources");
  if (col)
    {
    this->GetTraceHelper()->AddEntry("# User selected delete all modules");
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
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DeleteAllKeyframesCallback()
{
  if (!this->AnimationManager || !this->AnimationManager->IsCreated())
    {
    return;
    }

  if ( vtkKWMessageDialog::PopupYesNo(
      this->GetApplication(), this, "DeleteAllTheKeyFrames",
      "Delete All Key Frames", 
      "Are you sure you want to delete all the key frames in the animation?", 
      vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::RememberYes |
      vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault ))
    {
    this->DeleteAllKeyframes();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::DeleteAllKeyframes()
{
  if (this->AnimationManager)
    {
    this->AnimationManager->RemoveAllKeyFrames(); 
    this->GetTraceHelper()->AddEntry("$kw(%s) DeleteAllKeyframes", this->GetTclName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetInteractiveRenderEnabled(int s)
{
  this->InteractiveRenderEnabled = s;
  vtkPVGenericRenderWindowInteractor* rwi = this->Interactor;
  if ( !rwi )
    {
    return;
    }
  rwi->SetInteractiveRenderEnabled(s);
  
  vtkRenderWindow *rw = rwi->GetRenderWindow();
  if ( rw )
    {
    if ( s )
      {
      rw->SetDesiredUpdateRate(5.0);
      }
    else
      {
      rw->SetDesiredUpdateRate(0.002);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::UpdateToolbarState()
{
  this->Superclass::UpdateToolbarState();
  
  //this->DisableToolbarButtons();
  //  this->EnableToolbarButtons();

  // If not enabled, source grabbed or recording, disable toolbar button

  if (!this->GetEnabled() ||
      (this->CurrentPVSource && this->CurrentPVSource->GetSourceGrabbed()) ||
      (this->AnimationManager && this->AnimationManager->GetInRecording()))
    {
    this->DisableToolbarButtons();
    }
  else
    {
    this->EnableToolbarButtons();
    }

  if (!this->LowerToolbars)
    {
    return;
    }
  
  // Decide the properties of the toolbars.
  int flat_frame;
  if (this->GetApplication()->HasRegistryValue(
    2, "RunTime", VTK_KW_TOOLBAR_FLAT_FRAME_REG_KEY))
    {
    flat_frame = this->GetApplication()->GetIntRegistryValue(
      2, "RunTime", VTK_KW_TOOLBAR_FLAT_FRAME_REG_KEY);
    }
  else
    {
    flat_frame = vtkKWToolbar::GetGlobalFlatAspect();
    }

  int flat_buttons;
  if (this->GetApplication()->HasRegistryValue(
    2, "RunTime", VTK_KW_TOOLBAR_FLAT_BUTTONS_REG_KEY))
    {
    flat_buttons = this->GetApplication()->GetIntRegistryValue(
      2, "RunTime", VTK_KW_TOOLBAR_FLAT_BUTTONS_REG_KEY);
    }
  else
    {
    flat_buttons = vtkKWToolbar::GetGlobalWidgetsFlatAspect();
    }

  this->LowerToolbars->SetToolbarsFlatAspect(flat_frame);
  this->LowerToolbars->SetToolbarsWidgetsFlatAspect(flat_buttons);

  // Decide if the lower toolbars frame should be shown
  if (this->LowerToolbars->IsCreated())
    {
    if (this->LowerToolbars->GetNumberOfVisibleToolbars())
      {
      this->Script(
        "pack %s -padx 0 -pady 0 -side bottom -fill x -expand no ",
        this->LowerToolbars->GetWidgetName());
      this->LowerToolbars->PackToolbars();
      }
    else
      {
      this->LowerToolbars->Unpack();
      }
    }
}

//----------------------------------------------------------------------------
int vtkPVWindow::GetEnabled()
{
  // In general, if we are playing the animation, the UI should be entirely
  // disabled. There will be some very specific elements that will remain
  // enabled, and they will be taken care of in UpdateEnableState()

  int enabled = this->Superclass::GetEnabled();
  if (this->AnimationManager && this->AnimationManager->GetInPlay())
    {
    enabled = 0;
    }

  return enabled;
}

//-----------------------------------------------------------------------------
void vtkPVWindow::UpdateEnableState()
{
  if (this->InDemo)
    {
    return;
    }

  this->Superclass::UpdateEnableState();

  // The main view is enabled (or not) even if were are playing the animation

  if (this->MainView)
    {
    this->MainView->SetEnabled(this->Superclass::GetEnabled());
    }

  this->PropagateEnableState(this->Toolbar);

  this->PropagateEnableState(this->LowerFrame);
  this->PropagateEnableState(this->AnimationManager);
  this->PropagateEnableState(this->TimerLogDisplay);
  this->PropagateEnableState(this->ErrorLogDisplay);

  this->PropagateEnableState(this->CenterXEntry);
  this->PropagateEnableState(this->CenterYEntry);
  this->PropagateEnableState(this->CenterZEntry);

  if ( this->SourceLists )
    {
    const char* sourcelists[] = { 
      "Sources",
      "GlyphSources",
      0
    };
    int cc, kk;
    for ( cc = 0; sourcelists[cc]; cc ++ )
      {
      vtkPVSourceCollection* col = 0;
      if ( this->SourceLists->GetItem(sourcelists[cc], col) == VTK_OK && col )
        {
        for ( kk = 0; kk < col->GetNumberOfItems(); kk ++ )
          {
          this->PropagateEnableState(
            vtkPVSource::SafeDownCast(col->GetItemAsObject(kk)));
          }
        }
      }
    }

  vtkCollectionIterator* it = this->PVColorMaps->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    this->PropagateEnableState(
      static_cast<vtkPVColorMap*>(it->GetCurrentObject()));
    it->GoToNextItem();
    }
  it->Delete();

  this->PropagateEnableState(this->CurrentPVSource);
}

//----------------------------------------------------------------------------
void vtkPVWindow::UpdateMenuState()
{
  this->Superclass::UpdateMenuState();

  this->PropagateEnableState(this->SourceMenu);
  this->PropagateEnableState(this->FilterMenu);
  this->PropagateEnableState(this->SelectMenu);
  this->PropagateEnableState(this->GlyphMenu);
  this->PropagateEnableState(this->PreferencesMenu);

  int menu_state = 
    (this->GetEnabled() ? vtkKWMenu::Normal: vtkKWMenu::Disabled);

  int in_recording = 
    this->AnimationManager && this->AnimationManager->GetInRecording();

  int source_grabbed = 
    this->CurrentPVSource && this->CurrentPVSource->GetSourceGrabbed();

  if (this->InDemo)
    {
    return;
    }
  
  // Source grabbed or in recording ? Disable the root menu entries
  // In recording (or not recording anymore) ? Re-enable View, Window, Help

  if (source_grabbed || in_recording)
    {
    if (this->GetMenu())
      {
      this->GetMenu()->SetEnabled(0);
      }
    }

  if (!source_grabbed)
    {
    this->GetMenu()->SetState("View",  menu_state);
    this->GetMenu()->SetState("Window",  menu_state);
    this->GetMenu()->SetState("Help",  menu_state);
    }

  // Disable or enable the select menu. Checks if there are any valid
  // entries in the menu, disables the menu if there none, enables it
  // otherwise. Enable them if recording...
  
  vtkPVSourceCollection* sources = this->GetSourceList("Sources");
  int no_sources = !sources || !sources->GetNumberOfItems();

  this->UpdateSelectMenu();
  if (this->SelectMenu)
    {
    this->SelectMenu->SetEnabled(no_sources ? 0 : this->GetEnabled());
    }
  this->GetMenu()->SetState(
    VTK_PV_SELECT_SOURCE_MENU_LABEL, 
    no_sources || source_grabbed ? vtkKWMenu::Disabled : menu_state);
  if (this->ViewMenu)
    {
    this->ViewMenu->SetState(
      VTK_PV_SOURCE_MENU_LABEL, 
      no_sources || source_grabbed ? vtkKWMenu::Disabled : menu_state);
    }

  // Handle the filter menu

  this->UpdateFilterMenu();
  this->GetMenu()->SetState(
    VTK_PV_VTK_FILTERS_MENU_LABEL,  
    !this->FilterMenu->GetEnabled() || source_grabbed 
    ? vtkKWMenu::Disabled : menu_state);

  // Handle the source menu and toolbar buttons.

  this->UpdateSourceMenu();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetProgress(const char* text, int val)
{
  double lastprog = vtkTimerLog::GetUniversalTime();
  if ( !this->ExpectProgress )
    {
    this->LastProgress = lastprog;
    return;
    }
  if ( lastprog - this->LastProgress < 1 )
    {
    return;
    }
  this->LastProgress = lastprog;
  if ( val == 0 || val > 100 )
    {
    return;
    }
  if ( strlen(text) > 4 && text[0] == 'v' && text[1] == 't' && text[2] == 'k' )
    {
    text += 3;
    }
  this->ModifiedEnableState = 1;
  this->SetEnabled(0);
  this->SetStatusText(text);
  this->GetProgressGauge()->SetValue(val);
  this->Script("update");
}

//-----------------------------------------------------------------------------
void vtkPVWindow::StartProgress()
{
  this->MainView->StartBlockingRender();
  this->ExpectProgress = 1;
  this->ModifiedEnableState = 0;
  this->LastProgress = vtkTimerLog::GetUniversalTime();
}

//-----------------------------------------------------------------------------
void vtkPVWindow::EndProgress(int enabled)
{
  this->ExpectProgress = 0;
  this->GetProgressGauge()->SetValue(0);
  this->LastProgress = vtkTimerLog::GetUniversalTime();
  this->SetStatusText("");

  this->MainView->EndBlockingRender();

  if (!this->ModifiedEnableState)
    {
    return;
    }

  this->ModifiedEnableState = 0;
  if ( !enabled || this->GetEnabled() )
    {
    this->UpdateEnableState();
    }
  else
    {
    this->EnabledOn();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetInteractorEventInformation(int x, int y, int ctrl,
                                                int shift, char keycode,
                                                int repeatcount,
                                                const char *keysym)
{
  this->Interactor->SetEventInformation(x, y, ctrl, shift, keycode,
                                        repeatcount, keysym);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::InvokeInteractorEvent(const char *event)
{
  this->Interactor->InvokeEvent(event);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SaveWindowGeometry()
{
  this->Superclass::SaveWindowGeometry();
  if (this->IsCreated())
    {
    this->GetApplication()->SetRegistryValue(
      2, "Geometry", "WindowHorizontalFrame1Size",
      "%d", this->LowerFrame->GetFrame1Size());

    if (this->AnimationManager)
      {
      this->AnimationManager->SaveWindowGeometry();
      }
    }
  
}

//-----------------------------------------------------------------------------
void vtkPVWindow::RestorePVWindowGeometry()
{
  if (this->GetApplication()->HasRegistryValue(
      2, "Geometry", "WindowHorizontalFrame1Size"))
    {
    int reg_size = this->GetApplication()->GetIntRegistryValue(
      2, "Geometry", "WindowHorizontalFrame1Size");
    if (reg_size >= this->LowerFrame->GetFrame1MinimumSize())
      {
      this->LowerFrame->SetFrame1Size(reg_size);
      }
    }
  if (this->AnimationManager)
    {
    this->AnimationManager->RestoreWindowGeometry();
    }
}

//-----------------------------------------------------------------------------
void vtkPVWindow::AddLowerToolbar(vtkKWToolbar* toolbar, const char* name, 
  int visibility/*=1*/)
{
  if (!this->LowerToolbars->AddToolbar(toolbar))
    {
    return;
    }
  int id = this->LowerToolbars->GetNumberOfToolbars() - 1; 
  ostrstream command;
  command << "ToggleLowerToolbarVisibility " << id << " " << name << ends;
  this->AddToolbarToMenu(toolbar, name, this, command.str());
  command.rdbuf()->freeze(0);

  // Restore state from registry.
  ostrstream reg_key;
  reg_key << name << "_ToolbarVisibility" << ends;
  if (this->GetApplication()->GetRegistryValue(2, "RunTime", reg_key.str(), 0))
    {
    visibility = this->GetApplication()->GetIntRegistryValue(2, "RunTime", reg_key.str());
    }
  this->SetLowerToolbarVisibility(toolbar, name, visibility);
  reg_key.rdbuf()->freeze(0);
}
  
//-----------------------------------------------------------------------------
void vtkPVWindow::ToggleLowerToolbarVisibility(int id, const char* name)
{
  vtkKWToolbar* toolbar = this->LowerToolbars->GetToolbar(id);
  if (!toolbar)
    {
    return;
    }
  int new_visibility = this->LowerToolbars->IsToolbarVisible(toolbar)? 0 : 1;
  this->SetLowerToolbarVisibility(toolbar, name, new_visibility);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::SetLowerToolbarVisibility(vtkKWToolbar* toolbar,
  const char* name, int flag)
{
  this->LowerToolbars->SetToolbarVisibility(toolbar, flag);
  this->SetToolbarVisibilityInternal(toolbar, name, flag);
}

//-----------------------------------------------------------------------------
void vtkPVWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CenterXEntry: " << this->GetCenterXEntry() << endl;
  os << indent << "CenterYEntry: " << this->GetCenterYEntry() << endl;
  os << indent << "CenterZEntry: " << this->GetCenterZEntry() << endl;
  os << indent << "FilterMenu: " << this->GetFilterMenu() << endl;
  os << indent << "InteractorStyleToolbar: " << this->GetInteractorToolbar() 
     << endl;
  os << indent << "MainView: " << this->GetMainView() << endl;
  os << indent << "CameraStyle2D: " << this->CameraStyle2D << endl;
  os << indent << "CameraStyle3D: " << this->CameraStyle3D << endl;
  os << indent << "CenterOfRotationStyle: " << this->CenterOfRotationStyle
     << endl;
  os << indent << "ResetCameraButton: " << this->ResetCameraButton << endl;
  os << indent << "RotateCameraButton: " << this->RotateCameraButton << endl;
  os << indent << "TranslateCameraButton: " << this->TranslateCameraButton
     << endl;
  os << indent << "SelectMenu: " << this->SelectMenu << endl;
  os << indent << "SourceMenu: " << this->SourceMenu << endl;
  os << indent << "Toolbar: " << this->GetToolbar() << endl;
  os << indent << "PickCenterToolbar: " << this->GetPickCenterToolbar() << endl;
  os << indent << "Interactor: " << this->Interactor << endl;
  os << indent << "GlyphMenu: " << this->GlyphMenu << endl;
  os << indent << "InitializeDefaultInterfaces: " 
     << this->InitializeDefaultInterfaces << endl;
  os << indent << "UseMessageDialog: " << this->UseMessageDialog << endl;
  os << indent << "InteractiveRenderEnabled: " 
     << (this->InteractiveRenderEnabled?"on":"off") << endl;
  os << indent << "AnimationManager: " << this->AnimationManager << endl;
  os << indent << "InteractorID: " << this->InteractorID << endl;
  os << indent << "InDemo: " << this->InDemo << endl;
  os << indent << "LowerToolbars: " << this->LowerToolbars << endl;
  os << indent << "SaveVisibleSourcesOnlyFlag: " << this->SaveVisibleSourcesOnlyFlag << endl;
  os << indent << "TraceHelper: " << this->TraceHelper << endl;

  // Lookmarks part:
#ifdef PARAVIEW_USE_LOOKMARKS
  os << indent << "PVLookmarkManager: ";
  if( this->PVLookmarkManager )
    {
    this->PVLookmarkManager->PrintSelf( os << endl, indent.GetNextIndent() );
    }
  else
    {
    os << "(none)" << endl;
    }
#endif //PARAVIEW_USE_LOOKMARKS
}


