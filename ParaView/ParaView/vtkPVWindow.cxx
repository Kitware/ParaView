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
#include "vtkPVApplication.h"
#include "vtkKWToolbar.h"
#include "vtkObjectFactory.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWNotebook.h"
#include "vtkKWPushButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWRadioButton.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWTclInteractor.h"

#include "vtkToolkits.h"
#include "vtkDirectory.h"
#include "vtkMath.h"

#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVReaderModule.h"
#include "vtkPVSourceInterfaceDirectories.h"
#include "vtkPVAnimationInterface.h"
#include "vtkPVRenderView.h"
#include "vtkPVFileEntry.h"
#include "vtkPVWizard.h"
#include "vtkPVXMLPackageParser.h"
#include "vtkPVTimerLogDisplay.h"

#include "vtkPVDemoPaths.h"

#include "vtkPVSourceInterfaceDirectories.h"
#include "vtkPVAnimationInterface.h"
#include "vtkPVRenderView.h"
#include "vtkPVFileEntry.h"
#include "vtkPVWizard.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkGenericRenderWindowInteractor.h"
#include "vtkPVInteractorStyleTranslateCamera.h"
#include "vtkPVInteractorStyleRotateCamera.h"
#include "vtkPVInteractorStyleCenterOfRotation.h"
//#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkPVInteractorStyleFly.h"

#include "vtkAxes.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"

#include "vtkArrayMap.txx"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"

#include <ctype.h>
#include <sys/stat.h>

#ifdef _WIN32
#include "vtkKWRegisteryUtilities.h"
#endif


#ifndef VTK_USE_ANSI_STDLIB
#define PV_NOCREATE | ios::nocreate
#else
#define PV_NOCREATE 
#endif

//----------------------------------------------------------------------------
vtkPVWindow* vtkPVWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVWindow");
  if(ret)
    {
    return (vtkPVWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVWindow;
}

int vtkPVWindowCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVWindow::vtkPVWindow()
{
  this->SetWindowClass("ParaView");

  this->CommandFunction = vtkPVWindowCommand;

  // ParaView specific menus:
  // SelectMenu   -> used to select existing data objects
  // GlyphMenu    -> used to select existing glyph objects (cascaded from
  //                 SelectMenu)
  // AdvancedMenu -> for advanced users, contains SourceMenu and FilterMenu,
  //                 buttons for command prompt, exporting VTK scripts...
  // SourceMenu   -> available source modules
  // FilterMenu   -> available filter modules (depending on the current 
  //                 data object's type)
  this->SourceMenu = vtkKWMenu::New();
  this->FilterMenu = vtkKWMenu::New();
  this->SelectMenu = vtkKWMenu::New();
  this->GlyphMenu = vtkKWMenu::New();
  this->AdvancedMenu = vtkKWMenu::New();

  // This toolbar contains buttons for modifying user interaction
  // mode
  this->InteractorToolbar = vtkKWToolbar::New();

  this->FlyButton = vtkKWRadioButton::New();
  this->RotateCameraButton = vtkKWRadioButton::New();
  this->TranslateCameraButton = vtkKWRadioButton::New();
  //this->TrackballCameraButton = vtkKWRadioButton::New();
  
  this->GenericInteractor = vtkPVGenericRenderWindowInteractor::New();
  
  // This toolbar contains buttons for instantiating new modules
  this->Toolbar = vtkKWToolbar::New();

  
  //this->TrackballCameraStyle = vtkInteractorStyleTrackballCamera::New();
  this->TranslateCameraStyle = vtkPVInteractorStyleTranslateCamera::New();
  this->RotateCameraStyle = vtkPVInteractorStyleRotateCamera::New();
  this->CenterOfRotationStyle = vtkPVInteractorStyleCenterOfRotation::New();
  this->FlyStyle = vtkPVInteractorStyleFly::New();
  
  this->PickCenterToolbar = vtkKWToolbar::New();
  this->PickCenterButton = vtkKWPushButton::New();
  this->ResetCenterButton = vtkKWPushButton::New();
  this->CenterEntryOpenButton = vtkKWPushButton::New();
  this->CenterEntryCloseButton = vtkKWPushButton::New();
  this->CenterEntryFrame = vtkKWWidget::New();
  this->CenterXLabel = vtkKWLabel::New();
  this->CenterXEntry = vtkKWEntry::New();
  this->CenterYLabel = vtkKWLabel::New();
  this->CenterYEntry = vtkKWEntry::New();
  this->CenterZLabel = vtkKWLabel::New();
  this->CenterZEntry = vtkKWEntry::New();

  this->FlySpeedToolbar = vtkKWToolbar::New();
  this->FlySpeedLabel = vtkKWLabel::New();
  this->FlySpeedScale = vtkKWScale::New();
  
  this->CenterSource = vtkAxes::New();
  this->CenterSource->SymmetricOn();
  this->CenterSource->ComputeNormalsOff();
  this->CenterMapper = vtkPolyDataMapper::New();
  this->CenterMapper->SetInput(this->CenterSource->GetOutput());
  this->CenterActor = vtkActor::New();
  this->CenterActor->PickableOff();
  this->CenterActor->SetMapper(this->CenterMapper);
  this->CenterActor->VisibilityOff();
  
  this->CurrentPVData = NULL;
  this->CurrentPVSource = NULL;

  // Allow the user to interactively resize the properties parent.
  this->MiddleFrame->SetSeparatorWidth(6);
  this->MiddleFrame->SetFrame1MinimumWidth(5);
  this->MiddleFrame->SetFrame1Width(360);
  this->MiddleFrame->SetFrame2MinimumWidth(200);

  // Frame used for animations.
  this->AnimationInterface = vtkPVAnimationInterface::New();

  this->TimerLogDisplay = NULL;
  this->TclInteractor = NULL;

  // Set the extension and the type (name) of the script for
  // this application. They are all Tcl scripts but we give
  // them different names to differentiate them.
  this->SetScriptExtension(".pvs");
  this->SetScriptType("ParaView");

  // Set the title of the properties menu (for some reason, the
  // menus are named MenuProperties, MenuFile etc. instead of
  // FileMenu, PropertiesMenu etc. in vtkKWMenu).
  this->SetMenuPropertiesTitle("View");

  // Used to store the extensions and descriptions for supported
  // file formats (in Tk dialog format i.e. {ext1 ext2 ...} 
  // {{desc1} {desc2} ...}
  this->FileExtensions = NULL;
  this->FileDescriptions = NULL;

  // The prototypes for source and filter modules. Instances are 
  // created by calling ClonePrototype() on these.
  this->Prototypes = vtkArrayMap<const char*, vtkPVSource*>::New();
  // The prototypes for reader modules. Instances are 
  // created by calling ReadFile() on these.
  this->ReaderList = vtkLinkedList<vtkPVReaderModule*>::New();

  // Map <name> -> <source collection>
  // These contain the sources and filters which the user manipulate.
  this->SourceLists = vtkArrayMap<const char*, vtkPVSourceCollection*>::New();
  // Add a default collection for user created readers, sources and
  // filters.
  vtkPVSourceCollection* sources = vtkPVSourceCollection::New();
  this->SourceLists->SetItem("Sources", sources);
  sources->Delete();

  // Keep a list of the toolbar buttons so that they can be 
  // disabled/enabled in certain situations.
  this->ToolbarButtons = vtkArrayMap<const char*, vtkKWPushButton*>::New();

  // Keep a list of all loaded packages (Tcl libraries) so that
  // they can be written out when writing Tcl scripts.
  this->PackageNames = vtkLinkedList<const char*>::New();

  // This can be used to disable the pop-up dialogs if necessary
  // (usually used from inside regression scripts)
  this->UseMessageDialog = 1;
  // Whether or not to read the default interfaces.
  this->InitializeDefaultInterfaces = 1;

  this->MainView = 0;
}

//----------------------------------------------------------------------------
vtkPVWindow::~vtkPVWindow()
{
  this->PrepareForDelete();

  this->FlyButton->Delete();
  this->FlyButton = NULL;
  this->RotateCameraButton->Delete();
  this->RotateCameraButton = NULL;
  this->TranslateCameraButton->Delete();
  this->TranslateCameraButton = NULL;
  //this->TrackballCameraButton->Delete();
  //this->TrackballCameraButton = NULL;

  this->CenterSource->Delete();
  this->CenterSource = NULL;
  this->CenterMapper->Delete();
  this->CenterMapper = NULL;
  this->CenterActor->Delete();
  this->CenterActor = NULL;

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

  if (this->TclInteractor)
    {
    this->TclInteractor->Delete();
    this->TclInteractor = NULL;
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
}

//----------------------------------------------------------------------------
void vtkPVWindow::CloseNoPrompt()
{
  if (this->TimerLogDisplay )
    {
    this->TimerLogDisplay->SetMasterWindow(NULL);
    this->TimerLogDisplay->Delete();
    this->TimerLogDisplay = NULL;
    }

  if (this->TclInteractor )
    {
    this->TclInteractor->SetMasterWindow(NULL);
    this->TclInteractor->Delete();
    this->TclInteractor = NULL;
    }

  this->vtkKWWindow::CloseNoPrompt();
}

//----------------------------------------------------------------------------
void vtkPVWindow::PrepareForDelete()
{
  if (this->CurrentPVData)
    {
    this->CurrentPVData->UnRegister(this);
    this->CurrentPVData = NULL;
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

  if (this->FlyStyle)
    {
    this->FlyStyle->Delete();
    this->FlyStyle = NULL;
    }

  if (this->RotateCameraStyle)
    {
    this->RotateCameraStyle->Delete();
    this->RotateCameraStyle = NULL;
    }
  if (this->TranslateCameraStyle)
    {
    this->TranslateCameraStyle->Delete();
    this->TranslateCameraStyle = NULL;
    }
  if (this->CenterOfRotationStyle)
    {
    this->CenterOfRotationStyle->Delete();
    this->CenterOfRotationStyle = NULL;
    }
  
  if (this->GenericInteractor)
    {
    this->GenericInteractor->Delete();
    this->GenericInteractor = NULL;
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
  
  if (this->CenterEntryOpenButton)
    {
    this->CenterEntryOpenButton->Delete();
    this->CenterEntryOpenButton = NULL;
    }
  
  if (this->CenterEntryCloseButton)
    {
    this->CenterEntryCloseButton->Delete();
    this->CenterEntryCloseButton = NULL;
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

  if (this->FlySpeedLabel)
    {
    this->FlySpeedLabel->Delete();
    this->FlySpeedLabel = NULL;
    }
  
  if (this->FlySpeedScale)
    {
    this->FlySpeedScale->Delete();
    this->FlySpeedScale = NULL;
    }
  
  if (this->FlySpeedToolbar)
    {
    this->FlySpeedToolbar->Delete();
    this->FlySpeedToolbar = NULL;
    }

  if (this->MainView)
    {
    // At exit, save the background colour in the registery.
    this->SaveColor(2, "RenderViewBG", 
		    this->MainView->GetBackgroundColor());
    
    this->MainView->Delete();
    this->MainView = NULL;
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
  
  if (this->AdvancedMenu)
    {
    this->AdvancedMenu->Delete();
    this->AdvancedMenu = NULL;
    }

  if (this->AnimationInterface)
    {
    this->AnimationInterface->Delete();
    this->AnimationInterface = NULL;
    }
  
}


//----------------------------------------------------------------------------
void vtkPVWindow::InitializeMenus(vtkKWApplication *app)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);
  vtkKWWidget *pushButton;
  
  // Add view options.

  // Shows the notebook for the current source and data object.
  char *rbv = 
    this->GetMenuProperties()->CreateRadioButtonVariable(
      this->GetMenuProperties(),"Radio");
  this->GetMenuProperties()->AddRadioButton(
    2, " Source", rbv, this, "ShowCurrentSourcePropertiesCallback", 1,
    "Display the properties of the current data source or filter");
  delete [] rbv;

  // Shows the animation tool.
  rbv = this->GetMenuProperties()->CreateRadioButtonVariable(
           this->GetMenuProperties(),"Radio");
  this->GetMenuProperties()->AddRadioButton(
    3, " Animation", rbv, this, "ShowAnimationProperties", 1,
    "Display the interface for creating animations by varying variables "
    "in a loop");
  delete [] rbv;

  // Add/Remove to/from File menu.

  // We do not need Close in the file menu since we don't
  // support multiple windows (exit is enough)
  this->MenuFile->DeleteMenuItem("Close");
  // Open a data file. Can support multiple file formats (see Open()).
  this->MenuFile->InsertCommand(0, "Open Data File", this, "OpenCallback",0);
  // Save current data in VTK format.
  this->MenuFile->InsertCommand(1, "Save Data", this, "WriteData",0);
  // Copies the current trace file to another file.
  this->MenuFile->InsertCommand(3, "Save Trace File", this, "SaveTrace");

  // ParaView specific menus.

  // Create the select menu (for selecting user created and default
  // (i.e. glyphs) data objects/sources)
  this->SelectMenu->SetParent(this->GetMenu());
  this->SelectMenu->Create(this->Application, "-tearoff 0");
  this->Menu->InsertCascade(2, "Select", this->SelectMenu, 0);
  
  // Create the menu for selecting the glyphs.  
  this->GlyphMenu->SetParent(this->SelectMenu);
  this->GlyphMenu->Create(this->Application, "-tearoff 0");
  this->SelectMenu->AddCascade("Glyphs", this->GlyphMenu, 0,
				 "Select one of the glyph sources.");  

  // Advanced stuff like saving VTK scripts, loading packages etc.
  this->AdvancedMenu->SetParent(this->GetMenu());
  this->AdvancedMenu->Create(this->Application, "-tearoff 0");
  this->Menu->InsertCascade(3, "Advanced", this->AdvancedMenu, 0);

  this->AdvancedMenu->InsertCommand(4, "Command Prompt", this,
				    "DisplayCommandPrompt",8,
				    "Display a prompt to interact "
    "with the ParaView engine");
  this->AdvancedMenu->AddCommand("Load ParaView Script", this, "LoadScript", 0,
				 "Load ParaView Script (.pvs)");
  this->AdvancedMenu->InsertCommand(2, "Export VTK Script", this,
				    "ExportVTKScript", 7,
				    "Write a script which can be "
				    "parsed by the vtk executable");
  
  // Log stuff (not traced)
  this->AdvancedMenu->InsertCommand(5, "Show Log", this, "ShowLog", 2,
				    "Show log of render events and timing");
              

  this->AdvancedMenu->InsertCommand(7, "Open Package", this, "OpenPackage", 2,
				    "Open a ParaView package and load the "
				    "contents");
  
  // Create the menu for creating data sources.  
  this->SourceMenu->SetParent(this->AdvancedMenu);
  this->SourceMenu->Create(this->Application, "-tearoff 0");
  this->AdvancedMenu->AddCascade("VTK Sources", this->SourceMenu, 4,
				 "Choose a source from a list of "
				 "VTK sources");  
  
  // Create the menu for creating data sources (filters).  
  this->FilterMenu->SetParent(this->AdvancedMenu);
  this->FilterMenu->Create(this->Application, "-tearoff 0");
  this->AdvancedMenu->AddCascade("VTK Filters", this->FilterMenu, 4,
				 "Choose a filter from a list of "
				 "VTK filters");  
  this->Script("%s entryconfigure \"VTK Filters\" -state disabled",
	       this->AdvancedMenu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVWindow::InitializeToolbars(vtkKWApplication *app)
{

  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);

  this->InteractorToolbar->SetParent(this->GetToolbarFrame());
  this->InteractorToolbar->Create(app);

  this->Toolbar->SetParent(this->GetToolbarFrame());
  this->Toolbar->Create(app);

  this->Script("pack %s -side left -pady 0 -anchor n -fill none -expand no",
               this->InteractorToolbar->GetWidgetName());
  this->Script("pack  %s -side left -pady 0 -fill both -expand yes",
	       this->Toolbar->GetWidgetName());
  
}

//----------------------------------------------------------------------------
void vtkPVWindow::InitializeInteractorInterfaces(vtkKWApplication *app)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);

  vtkKWRadioButton *button;

  // Set up the button to reset the camera.
  vtkKWWidget* pushButton = vtkKWWidget::New();
  pushButton->SetParent(this->InteractorToolbar->GetFrame());
  pushButton->Create(app, "button", "-image KWResetViewButton -bd 0");
  pushButton->SetCommand(this, "ResetCameraCallback");
  this->Script( "pack %s -side left -fill none -expand no",
                pushButton->GetWidgetName());
  pushButton->SetBalloonHelpString(
    "Reset the view to show all the visible parts.");
  pushButton->Delete();
  pushButton = NULL;

  // set up the interactor styles
  // The interactor styles (selection and events) add no trace entries.
  
  // fly interactor style
  this->FlyButton->SetParent(this->InteractorToolbar);
  this->FlyButton->Create(app, "-indicatoron 0 -image KWFlyButton -selectimage KWActiveFlyButton -bd 0");
  this->FlyButton->SetBalloonHelpString(
    "Fly View Mode\n   Left Button: Fly toward mouse position.\n   Right Button: Fly backward");
  this->Script("%s configure -command {%s ChangeInteractorStyle 0}",
               this->FlyButton->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill none -expand no -padx 2 -pady 2", 
               this->FlyButton->GetWidgetName());

  // rotate camera interactor style
  this->RotateCameraButton->SetParent(this->InteractorToolbar);
  this->RotateCameraButton->Create(app, "-indicatoron 0 -image KWRotateViewButton -selectimage KWActiveRotateViewButton -bd 0");
  this->RotateCameraButton->SetState(1);
  this->RotateCameraButton->SetBalloonHelpString(
    "Rotate View Mode\n   Left Button: Rotate.\n  Shift + LeftButton: Z roll.\n   Right Button: Behaves like translate view mode.");
  this->Script("%s configure -command {%s ChangeInteractorStyle 1}",
               this->RotateCameraButton->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill none -expand no -padx 2 -pady 2", 
               this->RotateCameraButton->GetWidgetName());

  // translate camera interactor style
  this->TranslateCameraButton->SetParent(this->InteractorToolbar);
  this->TranslateCameraButton->Create(app, "-indicatoron 0 -image KWTranslateViewButton -selectimage KWActiveTranslateViewButton -bd 0");
  this->TranslateCameraButton->SetBalloonHelpString(
    "Translate View Mode\n   Left Button: Translate.\n   Right Button: Zoom.");
  this->Script("%s configure -command {%s ChangeInteractorStyle 2}", 
               this->TranslateCameraButton->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill none -expand no -padx 2 -pady 2", 
               this->TranslateCameraButton->GetWidgetName());

  // trackball camera interactor style
  //this->TrackballCameraButton->SetParent(this->InteractorStyleToolbar);
  //this->TrackballCameraButton->Create(app, "-text Trackball");
  //this->TrackballCameraButton->SetBalloonHelpString(
  //  "Trackball Camera Mode\n   Left Button: Rotate.\n   Shift + Left Button: Pan.\n   Right Button: Zoom.  (Zoom direction is reversed from Translate View Mode)");
  //this->Script("%s configure -command {%s ChangeInteractorStyle 3}",
  //             this->TrackballCameraButton->GetWidgetName(), this->GetTclName());
  //this->Script("pack %s -side left -fill none -expand no -padx 2 -pady 2",
  //             this->TrackballCameraButton->GetWidgetName());
  //this->TrackballCameraButton->SetState(1);
  
  this->RotateCameraStyle->SetCenter(0.0, 0.0, 0.0);
  this->MainView->ResetCamera();
}

//----------------------------------------------------------------------------
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
  this->WidgetName = new char [strlen(".paraview")+1];
  strcpy(this->WidgetName,".paraview");
  // Invoke super method first.
  this->vtkKWWindow::Create(pvApp,"");

  this->Script("wm geometry %s 900x700+0+0", this->GetWidgetName());
  
  // Hide the main window until after all user interface is initialized.
  this->Script( "wm withdraw %s", this->GetWidgetName());

  // Put the version in the status bar.
  char version[128];
  sprintf(version,"Version %d.%d", this->GetPVApplication()->GetMajorVersion(),
	  this->GetPVApplication()->GetMinorVersion());
  this->SetStatusText(version);

  this->InitializeMenus(app);
  this->InitializeToolbars(app);
  this->CreateDefaultPropertiesParent();

  // Create the main properties notebook.
  // The notebook does not create trace entries.
  this->Notebook->SetParent(this->GetPropertiesParent());
  this->Notebook->Create(this->Application,"");

  // Create the main view.
  this->CreateMainView(pvApp);

  this->InitializeInteractorInterfaces(app);

  // Initialize a couple of variables in the trace file.
  pvApp->AddTraceEntry("set kw(%s) [Application GetMainWindow]",
                       this->GetTclName());
  this->SetTraceInitialized(1);
  // We have to set this variable after the window variable is set,
  // so it has to be done here.
  pvApp->AddTraceEntry("set kw(%s) [$kw(%s) GetMainView]",
                       this->GetMainView()->GetTclName(), this->GetTclName());
  this->GetMainView()->SetTraceInitialized(1);


  this->PickCenterToolbar->SetParent(this->GetToolbarFrame());
  this->PickCenterToolbar->Create(app);
  
  this->PickCenterButton->SetParent(this->PickCenterToolbar);
  this->PickCenterButton->Create(app, "-image KWPickCenterButton -bd 1");
  this->PickCenterButton->SetCommand(this, "ChangeInteractorStyle 4");
  
  this->ResetCenterButton->SetParent(this->PickCenterToolbar);
  this->ResetCenterButton->Create(app, "-bd 1");
  this->ResetCenterButton->SetLabel("Reset");
  this->ResetCenterButton->SetCommand(this, "ResetCenterCallback");
  this->ResetCenterButton->SetBalloonHelpString("Reset the center of rotation to the center of the current data set.");
  
  this->CenterEntryOpenButton->SetParent(this->PickCenterToolbar);
  this->CenterEntryOpenButton->Create(app, "-bd 1");
  this->CenterEntryOpenButton->SetLabel(">");
  this->CenterEntryOpenButton->SetCommand(this, "CenterEntryOpenCallback");
  
  this->Script("pack %s %s %s -side left -expand no -fill none -pady 2",
               this->PickCenterButton->GetWidgetName(),
               this->ResetCenterButton->GetWidgetName(),
               this->CenterEntryOpenButton->GetWidgetName());
  
  this->CenterEntryFrame->SetParent(this->PickCenterToolbar);
  this->CenterEntryFrame->Create(app, "frame", "");
  
  this->CenterEntryCloseButton->SetParent(this->CenterEntryFrame);
  this->CenterEntryCloseButton->Create(app, "-bd 1");
  this->CenterEntryCloseButton->SetLabel("<");
  this->CenterEntryCloseButton->SetCommand(this, "CenterEntryCloseCallback");
  
  this->CenterXLabel->SetParent(this->CenterEntryFrame);
  this->CenterXLabel->Create(app, "");
  this->CenterXLabel->SetLabel("X");
  
  this->CenterXEntry->SetParent(this->CenterEntryFrame);
  this->CenterXEntry->Create(app, "-width 7");
  this->Script("bind %s <KeyPress-Return> {%s CenterEntryCallback}",
               this->CenterXEntry->GetWidgetName(), this->GetTclName());
  this->CenterXEntry->SetValue(this->RotateCameraStyle->GetCenter()[0], 3);
  
  this->CenterYLabel->SetParent(this->CenterEntryFrame);
  this->CenterYLabel->Create(app, "");
  this->CenterYLabel->SetLabel("Y");
  
  this->CenterYEntry->SetParent(this->CenterEntryFrame);
  this->CenterYEntry->Create(app, "-width 7");
  this->Script("bind %s <KeyPress-Return> {%s CenterEntryCallback}",
               this->CenterYEntry->GetWidgetName(), this->GetTclName());
  this->CenterYEntry->SetValue(this->RotateCameraStyle->GetCenter()[1], 3);

  this->CenterZLabel->SetParent(this->CenterEntryFrame);
  this->CenterZLabel->Create(app, "");
  this->CenterZLabel->SetLabel("Z");
  
  this->CenterZEntry->SetParent(this->CenterEntryFrame);
  this->CenterZEntry->Create(app, "-width 7");
  this->Script("bind %s <KeyPress-Return> {%s CenterEntryCallback}",
               this->CenterZEntry->GetWidgetName(), this->GetTclName());
  this->CenterZEntry->SetValue(this->RotateCameraStyle->GetCenter()[2], 3);

  this->Script("pack %s %s %s %s %s %s %s -side left",
               this->CenterXLabel->GetWidgetName(),
               this->CenterXEntry->GetWidgetName(),
               this->CenterYLabel->GetWidgetName(),
               this->CenterYEntry->GetWidgetName(),
               this->CenterZLabel->GetWidgetName(),
               this->CenterZEntry->GetWidgetName(),
               this->CenterEntryCloseButton->GetWidgetName());

  this->MainView->GetRenderer()->AddActor(this->CenterActor);
  
  this->FlySpeedToolbar->SetParent(this->GetToolbarFrame());
  this->FlySpeedToolbar->Create(app);
  
  this->FlySpeedLabel->SetParent(this->FlySpeedToolbar);
  this->FlySpeedLabel->Create(app, "");
  this->FlySpeedLabel->SetLabel("Fly Speed");
  
  this->FlySpeedScale->SetParent(this->FlySpeedToolbar);
  this->FlySpeedScale->Create(app, "");
  this->FlySpeedScale->SetRange(0.0, 50.0);
  this->FlySpeedScale->SetValue(20.0);
  this->FlySpeedScale->SetCommand(this, "FlySpeedScaleCallback");
  this->Script("pack %s %s -side left", 
               this->FlySpeedLabel->GetWidgetName(),
               this->FlySpeedScale->GetWidgetName());
  
  this->GenericInteractor->SetPVRenderView(this->MainView);
  this->ChangeInteractorStyle(1);
 
  // set up bindings for the interactor  
  const char *wname = this->MainView->GetVTKWidget()->GetWidgetName();
  const char *tname = this->GetTclName();
  this->Script("bind %s <B1-Motion> {}", wname);
  this->Script("bind %s <B3-Motion> {}", wname);
  this->Script("bind %s <Shift-B1-Motion> {}", wname);
  this->Script("bind %s <Shift-B3-Motion> {}", wname);
  
  this->Script("bind %s <Any-ButtonPress> {%s AButtonPress %%b %%x %%y}",
               wname, tname);
  this->Script("bind %s <Shift-Any-ButtonPress> {%s AShiftButtonPress %%b %%x %%y}",
               wname, tname);
  this->Script("bind %s <Any-ButtonRelease> {%s AButtonRelease %%b %%x %%y}",
               wname, tname);
  this->Script("bind %s <Shift-Any-ButtonRelease> {%s AShiftButtonRelease %%b %%x %%y}",
               wname, tname);
  this->Script("bind %s <Motion> {%s MouseMotion %%x %%y}",
               wname, tname);
  this->Script("bind %s <Configure> {%s Configure %%w %%h}",
               wname, tname);
  
  // Interface for the animation tool.
  this->AnimationInterface->SetWindow(this);
  this->AnimationInterface->SetView(this->GetMainView());
  this->AnimationInterface->SetParent(this->MainView->GetPropertiesParent());
  this->AnimationInterface->Create(app, "-bd 2 -relief raised");

  this->AddRecentFilesToMenu("Exit",this);

  // File->Open Data File is disabled unless reader modules are loaded.
  // AddFileType() enables this entry.
  this->Script("%s entryconfigure \"Open Data File\" -state disabled",
	       this->MenuFile->GetWidgetName());

  if (this->InitializeDefaultInterfaces)
    {
    vtkPVSourceCollection* sources = vtkPVSourceCollection::New();
    this->SourceLists->SetItem("GlyphSources", sources);
    sources->Delete();

    // We need an application before we can read the interface.
    this->ReadSourceInterfaces();
    
    // Create the extract grid button
    this->AddToolbarButton("ExtractGrid", "PVExtractGridButton", 
			   "ExtractGridCallback",
			   "Extract a sub grid from a structured data set.");

    vtkPVSource *pvs=0;
    
    // Create the sources that can be used for glyphing.
    // ===== Arrow
    pvs = this->CreatePVSource("ArrowSource", "GlyphSources", 0);
    pvs->IsDeletableOff();
    pvs->Accept(1);
    pvs->SetTraceReferenceObject(this);
    {
    ostrstream s;
    s << "GetPVSource GlyphSources " << pvs->GetName() << ends;
    pvs->SetTraceReferenceCommand(s.str());
    s.rdbuf()->freeze(0);
    }
    
    // ===== Cone
    pvs = this->CreatePVSource("ConeSource", "GlyphSources", 0);
    pvs->IsDeletableOff();
    pvs->Accept(1);
    pvs->SetTraceReferenceObject(this);
    {
    ostrstream s;
    s << "GetPVSource GlyphSources " << pvs->GetName() << ends;
    pvs->SetTraceReferenceCommand(s.str());
    s.rdbuf()->freeze(0);
    }
    
    // ===== Sphere
    pvs = this->CreatePVSource("SphereSource", "GlyphSources", 0);
    pvs->IsDeletableOff();
    pvs->Accept(1);
    pvs->SetTraceReferenceObject(this);
    {
    ostrstream s;
    s << "GetSource GlyphSources " << pvs->GetName() << ends;
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

  // Make the 3D View Settings the current one.
  this->Script("%s invoke \" 3D View Settings\"", 
	       this->MenuProperties->GetWidgetName());
  this->Script( "wm deiconify %s", this->GetWidgetName());

  this->Script("wm protocol %s WM_DELETE_WINDOW { %s Exit }",
	       this->GetWidgetName(), this->GetTclName());

}

//----------------------------------------------------------------------------
// The prototypes for source and filter modules. Instances are 
// created by calling ClonePrototype() on these.
void vtkPVWindow::AddPrototype(const char* name, vtkPVSource* proto)
{
  this->Prototypes->SetItem(name, proto);
}

//----------------------------------------------------------------------------
// Keep a list of all loaded packages (Tcl libraries) so that
// they can be written out when writing Tcl scripts.
void vtkPVWindow::AddPackageName(const char* name)
{
  this->PackageNames->AppendItem(name);
}

//----------------------------------------------------------------------------
// Keep a list of the toolbar buttons so that they can be 
// disabled/enabled in certain situations.
void vtkPVWindow::AddToolbarButton(const char* buttonName, 
				   const char* imageName, 
				   const char* command,
				   const char* balloonHelp)
{
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

void vtkPVWindow::CenterEntryOpenCallback()
{
  this->Script("catch {eval pack forget %s}",
               this->CenterEntryOpenButton->GetWidgetName());
  this->Script("pack %s -side left -expand no -fill none -pady 2",
               this->CenterEntryFrame->GetWidgetName());
}

void vtkPVWindow::CenterEntryCloseCallback()
{
  this->Script("catch {eval pack forget %s}",
               this->CenterEntryFrame->GetWidgetName());
  this->Script("pack %s -side left -expand no -fill none -pady 2",
               this->CenterEntryOpenButton->GetWidgetName());
}

void vtkPVWindow::CenterEntryCallback()
{
  float x = this->CenterXEntry->GetValueAsFloat();
  float y = this->CenterYEntry->GetValueAsFloat();
  float z = this->CenterZEntry->GetValueAsFloat();
  this->RotateCameraStyle->SetCenter(x, y, z);
  this->CenterActor->SetPosition(x, y, z);
  this->MainView->EventuallyRender();
}

void vtkPVWindow::ResetCenterCallback()
{
  if ( ! this->CurrentPVData)
    {
    return;
    }
  
  float center[3];
  this->CurrentPVData->GetVTKData()->GetCenter(center);
  this->RotateCameraStyle->SetCenter(center);
  this->CenterXEntry->SetValue(center[0], 3);
  this->CenterYEntry->SetValue(center[1], 3);
  this->CenterZEntry->SetValue(center[2], 3);
  this->CenterActor->SetPosition(center);
  this->ResizeCenterActor();
  this->MainView->EventuallyRender();
}

void vtkPVWindow::FlySpeedScaleCallback()
{
  this->FlyStyle->SetSpeed(this->FlySpeedScale->GetValue());
}

void vtkPVWindow::ResizeCenterActor()
{
  float bounds[6];
  
  int vis = this->CenterActor->GetVisibility();
  this->CenterActor->VisibilityOff();
  this->MainView->ComputeVisiblePropBounds(bounds);
  if ((bounds[0] < bounds[1]) && (bounds[2] < bounds[3]) &&
      (bounds[4] < bounds[5]))
    {
    this->CenterActor->SetScale(0.25 * (bounds[1]-bounds[0]),
                                0.25 * (bounds[3]-bounds[2]),
                                0.25 * (bounds[5]-bounds[4]));
    }
  else
    {
    this->CenterActor->SetScale(1, 1, 1);
    this->CenterActor->VisibilityOn();
    this->ResetCameraCallback();
    this->CenterActor->VisibilityOff();
    }
    
  this->CenterActor->SetVisibility(vis);
}

void vtkPVWindow::ChangeInteractorStyle(int index)
{
  this->Script("catch {eval pack forget %s}",
               this->PickCenterToolbar->GetWidgetName());
  this->Script("catch {eval pack forget %s}",
               this->FlySpeedToolbar->GetWidgetName());
  
  switch (index)
    {
    case 0:
      this->RotateCameraButton->SetState(0);
      this->TranslateCameraButton->SetState(0);
      //this->TrackballCameraButton->SetState(0);
      this->CenterActor->VisibilityOff();
      this->GenericInteractor->SetInteractorStyle(this->FlyStyle);
      this->Script("pack %s -side left",
                   this->FlySpeedToolbar->GetWidgetName());
      break;
    case 1:
      this->FlyButton->SetState(0);
      this->TranslateCameraButton->SetState(0);
      //this->TrackballCameraButton->SetState(0);
      this->GenericInteractor->SetInteractorStyle(this->RotateCameraStyle);
      this->Script("pack %s -side left",
                   this->PickCenterToolbar->GetWidgetName());
      this->ResizeCenterActor();
      this->CenterActor->VisibilityOn();
      break;
    case 2:
      this->FlyButton->SetState(0);
      this->RotateCameraButton->SetState(0);
      //this->TrackballCameraButton->SetState(0);
      this->GenericInteractor->SetInteractorStyle(this->TranslateCameraStyle);
      this->CenterActor->VisibilityOff();
      break;
    case 3:
      vtkErrorMacro("Trackball no longer suported.");
      //this->FlyButton->SetState(0);
      //this->RotateCameraButton->SetState(0);
      //this->TranslateCameraButton->SetState(0);
      //this->GenericInteractor->SetInteractorStyle(this->TrackballCameraStyle);
      //this->CenterActor->VisibilityOff();
      break;
    case 4:
      this->GenericInteractor->SetInteractorStyle(this->CenterOfRotationStyle);
      this->CenterActor->VisibilityOff();
      break;
    }
  this->MainView->EventuallyRender();
}

void vtkPVWindow::AButtonPress(int button, int x, int y)
{
  // not binding middle button
  if (button == 1)
    {
    this->GenericInteractor->SetEventInformationFlipY(x, y);
    this->GenericInteractor->LeftButtonPressEvent();
    }
  else if (button == 3)
    {
    this->GenericInteractor->SetEventInformationFlipY(x, y);
    this->GenericInteractor->RightButtonPressEvent();
    }
}

void vtkPVWindow::AShiftButtonPress(int button, int x, int y)
{
  // not binding middle button
  if (button == 1)
    {
    this->GenericInteractor->SetEventInformationFlipY(x, y, 0, 1);
    this->GenericInteractor->LeftButtonPressEvent();
    }
  else if (button == 3)
    {
    this->GenericInteractor->SetEventInformationFlipY(x, y, 0, 1);
    this->GenericInteractor->RightButtonPressEvent();
    }
}

void vtkPVWindow::AButtonRelease(int button, int x, int y)
{
  // not binding middle button
  if (button == 1)
    {
    this->GenericInteractor->SetEventInformationFlipY(x, y);
    this->GenericInteractor->LeftButtonReleaseEvent();
    }
  else if (button == 3)
    {
    this->GenericInteractor->SetEventInformationFlipY(x, y);
    this->GenericInteractor->RightButtonReleaseEvent();
    }
}

void vtkPVWindow::AShiftButtonRelease(int button, int x, int y)
{
  // not binding middle button
  if (button == 1)
    {
    this->GenericInteractor->SetEventInformationFlipY(x, y, 0, 1);
    this->GenericInteractor->LeftButtonReleaseEvent();
    }
  else if (button == 3)
    {
    this->GenericInteractor->SetEventInformationFlipY(x, y, 0, 1);
    this->GenericInteractor->RightButtonReleaseEvent();
    }
}

void vtkPVWindow::MouseMotion(int x, int y)
{
  this->GenericInteractor->SetEventInformationFlipY(x, y);
  this->GenericInteractor->MouseMoveEvent();
}

void vtkPVWindow::Configure(int width, int height)
{
  this->GenericInteractor->UpdateSize(width, height);
  this->GenericInteractor->ConfigureEvent();
}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::GetPVSource(const char* listname, char* sourcename)
{
  vtkPVSourceCollection* col = this->GetSourceList(listname);
  if (col)
    {
    vtkPVSource *pvs;
    col->InitTraversal();
    while ( (pvs = col->GetNextPVSource()) )
      {
      if (strcmp(sourcename, pvs->GetName()) == 0)
	{
	return pvs;
	}
      }
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVWindow::CreateMainView(vtkPVApplication *pvApp)
{
  vtkPVRenderView *view;
  
  view = vtkPVRenderView::New();
  view->CreateRenderObjects(pvApp);
  
  this->MainView = view;
  this->MainView->SetParent(this->ViewFrame);
  this->AddView(this->MainView);
  this->MainView->Create(this->Application,"-width 200 -height 200");
  this->MainView->MakeSelected();
  this->MainView->ShowViewProperties();
  this->MainView->SetupBindings();
  this->MainView->AddBindings(); // additional bindings in PV not in KW
  
  float rgb[3];
  this->RetrieveColor(2, "RenderViewBG", rgb); 
  if (rgb[0] == -1)
    {
    rgb[0] = rgb[1] = rgb[2] = 0;
    }
  this->MainView->SetBackgroundColor(rgb);
  this->Script( "pack %s -expand yes -fill both", 
                this->MainView->GetWidgetName());  

  this->MenuHelp->AddCommand("Play Demo", this, "PlayDemo", 0);
}


//----------------------------------------------------------------------------
void vtkPVWindow::PlayDemo()
{
  int found=0;
  int foundData=0;

  char temp1[1024];
  char temp2[1024];

  struct stat fs;

#ifdef _WIN32  

  // First look in the registery
  char loc[1024];
  char temp[1024];
  
  vtkKWRegisteryUtilities *reg = this->GetApplication()->GetRegistery();
  sprintf(temp, "%i", this->GetApplication()->GetApplicationKey());
  reg->SetTopLevel(temp);
  if (reg->ReadValue("Inst", loc, "Loc"))
    {
    sprintf(temp1,"%s/Demos/Demo1.pvs",loc);
    sprintf(temp2,"%s/Data/blow.vtk",loc);
    }

  // first make sure the file exists, this prevents an empty file from
  // being created on older compilers
  if (stat(temp2, &fs) == 0) 
    {
    foundData=1;
    this->Application->Script("set tmpPvDataDir [string map {\\\\ /} {%s/Data}]", loc);
    }

  if (stat(temp1, &fs) == 0) 
    {
    this->LoadScript(temp1);
    found=1;
    }

#endif // _WIN32  

  // Look in binary and installation directories

  const char** dir;
  for(dir=VTK_PV_DEMO_PATHS; !foundData && *dir; ++dir)
    {
    if (!foundData)
      {
      sprintf(temp2, "%s/Data/blow.vtk", *dir);
      if (stat(temp2, &fs) == 0) 
	{
	foundData=1;
	this->Application->Script("set tmpPvDataDir %s/Data", *dir);
	}
      }
    }

  for(dir=VTK_PV_DEMO_PATHS; !found && *dir; ++dir)
    {
    sprintf(temp1, "%s/Demos/Demo1.pvs", *dir);
    if (stat(temp1, &fs) == 0) 
      {
      this->LoadScript(temp1);
      found=1;
      }
    }

  if (!found)
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

//----------------------------------------------------------------------------
// Try to open a file for reading, return error on failure.
int vtkPVWindow::CheckIfFileIsReadable(const char* fileName)
{
  ifstream input(fileName, ios::in PV_NOCREATE);
  if (input.fail())
    {
    return VTK_ERROR;
    }
  else
    {
    return VTK_OK;
    }
}


//----------------------------------------------------------------------------
// Prompts the user for a filename and calls Open().
void vtkPVWindow::OpenCallback()
{
  char buffer[1024];
  // Retrieve old path from the registery
  if ( !this->GetApplication()->GetRegisteryValue(
	 2, "RunTime", "OpenPath", buffer) )
    {
    sprintf(buffer, ".");
    }
  
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

  this->Script("set openFileName [tk_getOpenFile -initialdir {%s} "
	       "-filetypes {{{ParaView Files} {%s}} %s}]", 
               buffer, this->FileExtensions, this->FileDescriptions);

  openFileName = new char[
    strlen(this->GetPVApplication()->GetMainInterp()->result) + 1];
  strcpy(openFileName, this->GetPVApplication()->GetMainInterp()->result);

  if (strcmp(openFileName, "") == 0)
    {
    return;
    }

  if  (this->Open(openFileName) != VTK_OK)
    {
    return;
    }

  // Store last path
  if ( openFileName && strlen(openFileName) > 0 )
    {
    char *pth = new char [strlen(openFileName)+1];
    sprintf(pth,"%s",openFileName);
    int pos = strlen(openFileName);
    // Strip off the file name
    while (pos && pth[pos] != '/' && pth[pos] != '\\')
      {
      pos--;
      }
    pth[pos] = '\0';
    // Store in the registery
    this->GetApplication()->SetRegisteryValue(
      2, "RunTime", "OpenPath", pth);
    delete [] pth;
    }
  
  delete [] openFileName;
}

//----------------------------------------------------------------------------
int vtkPVWindow::Open(char *openFileName)
{
  if (this->CheckIfFileIsReadable(openFileName) != VTK_OK)
    {
    char* error = new char[strlen("Can not open file ")
			  + strlen(openFileName) + strlen(" for reading.") 
			  + 2];
    sprintf(error,"Can not open file %s for reading.", openFileName);
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
    delete [] error;
    return VTK_ERROR;
    }


// These should be added manually to regression scripts so that
// they don't hang with a dialog up.
//  this->GetPVApplication()->AddTraceEntry("$kw(%s) UseMessageDialogOff", 
//					  this->GetTclName());
  this->GetPVApplication()->AddTraceEntry("$kw(%s) Open \"%s\"", 
					  this->GetTclName(), openFileName);
//  this->GetPVApplication()->AddTraceEntry("$kw(%s) UseMessageDialogOn", 
//					  this->GetTclName());

  // Ask each reader module if it can read the file. This first
  // one which says OK gets to read the file.
  vtkLinkedListIterator<vtkPVReaderModule*>* it = 
    this->ReaderList->NewIterator();
  while(it->IsDoneWithTraversal() != VTK_OK)
    {
    vtkPVReaderModule* rm = 0;
    int retVal = it->GetData(rm);
    if (retVal == VTK_OK && rm->CanReadFile(openFileName))
      {
      vtkPVReaderModule* clone = 0;
      // Read the file. On success this will return a new source.
      // Add that source to the list of sources.
      if (rm->ReadFile(openFileName, clone) == VTK_OK && clone)
	{
	this->GetSourceList("Sources")->AddItem(clone);
	if (clone->GetAcceptAfterRead())
	  {
	  clone->Accept(0);
	  }
	clone->Delete();
	}
      it->Delete();
      this->AddRecentFile(NULL, openFileName, this, "OpenFile");
      return VTK_OK;
      }
    it->GoToNextItem();
    }
  it->Delete();

  
  ostrstream error;
  error << "Could not find an appropriate reader for file "
	<< openFileName << ends;
  if (this->UseMessageDialog)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this, "Open Error", error.str(), 
      vtkKWMessageDialog::ErrorIcon);
    }
  else
    {
    vtkErrorMacro(<<error.str());
    }
  error.rdbuf()->freeze(0);     

  return VTK_ERROR;
}

//----------------------------------------------------------------------------
void vtkPVWindow::WriteVTKFile(char *filename)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (!this->CurrentPVData)
    {
    return;
    }

  pvApp->AddTraceEntry("$kw(%s) WriteVTKFile %s", this->GetTclName(),
                       filename);
 
  pvApp->BroadcastScript("vtkDataSetWriter writer");
  pvApp->BroadcastScript("writer SetFileName %s", filename);
  pvApp->BroadcastScript("writer SetInput %s",
                         this->GetCurrentPVData()->GetVTKDataTclName());
  pvApp->BroadcastScript("writer SetFileTypeToBinary");
  pvApp->BroadcastScript("writer Write");
  pvApp->BroadcastScript("writer Delete");
}

//----------------------------------------------------------------------------
void vtkPVWindow::WritePVTKFile(char *filename, int ghostLevel)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numProcs;
  int idx;

  if (!this->CurrentPVData)
    {
    return;
    }

  numProcs = 1;
  if (pvApp->GetController())
    {
    numProcs = pvApp->GetController()->GetNumberOfProcesses();
    }

  pvApp->AddTraceEntry("$kw(%s) WritePVTKFile %s", this->GetTclName(),
                       filename, ghostLevel);

  pvApp->BroadcastScript("vtkPDataSetWriter writer");
  pvApp->BroadcastScript("writer SetFileName %s", filename);
  pvApp->BroadcastScript("writer SetInput %s",
                         this->GetCurrentPVData()->GetVTKDataTclName());
  pvApp->BroadcastScript("writer SetFileTypeToBinary");
  pvApp->BroadcastScript("writer SetNumberOfPieces %d", numProcs);
  pvApp->BroadcastScript("writer SetGhostLevel %d", ghostLevel);
  this->Script("writer SetStartPiece 0");
  this->Script("writer SetEndPiece 0");
  for (idx = 1; idx < numProcs; ++idx)
    {
    pvApp->RemoteScript(idx, "writer SetStartPiece %d", idx);
    pvApp->RemoteScript(idx, "writer SetEndPiece %d", idx);
    }
  pvApp->BroadcastScript("writer Write");
  pvApp->BroadcastScript("writer Delete");
}

//----------------------------------------------------------------------------
void vtkPVWindow::WriteData()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  char *filename;
  int numProcs;

  if (!this->CurrentPVData)
    {
    return;
    }

  numProcs = 1;
  if (pvApp->GetController())
    {
    numProcs = pvApp->GetController()->GetNumberOfProcesses();
    }

  if (numProcs == 1)
    {
    this->Script("tk_getSaveFile -filetypes {{{VTK files} {.vtk}}} -defaultextension .vtk -initialfile data.vtk");
    
    filename = new char[strlen(this->Application->GetMainInterp()->result)+1];
    sprintf(filename, "%s", this->Application->GetMainInterp()->result);
  
    if (strcmp(filename, "") == 0)
      {
      delete [] filename;
      return;
      }
    this->WriteVTKFile(filename);
    }
  else
    {
    int ghostLevel;

    this->Script("tk_getSaveFile -filetypes {{{PVTK files} {.pvtk}}} -defaultextension .pvtk -initialfile data.pvtk");
    filename = new char[strlen(this->Application->GetMainInterp()->result)+1];
    sprintf(filename, "%s", this->Application->GetMainInterp()->result);
    if (strcmp(filename, "") == 0)
      {
      delete [] filename;
      return;
      }
  
    // See if the user wants to save any ghost levels.
    this->Script("tk_dialog .ghostLevelDialog {Ghost Level Selection} {How many ghost levels would you like to save?} {} 0 0 1 2");
    ghostLevel = this->GetIntegerResult(pvApp);
    if (ghostLevel == -1)
      {
      delete [] filename;
      return;
      }

    this->WritePVTKFile(filename, ghostLevel);
    }
  
  delete [] filename;
}

//----------------------------------------------------------------------------
void vtkPVWindow::ExportVTKScript()
{
  char *filename;
  
  this->Script("tk_getSaveFile -filetypes {{{Tcl Scripts} {.tcl}} {{All Files} {.*}}} -defaultextension .tcl");
  filename = this->Application->GetMainInterp()->result;
  
  if (strcmp(filename, "") == 0)
    {
    return;
    }

  this->SaveInTclScript(filename, 1);
}

//----------------------------------------------------------------------------
void vtkPVWindow::SaveInTclScript(const char* filename, int vtkFlag)
{
  ofstream *file;
  vtkPVSource *pvs;
  int num, idx;
  int imageFlag = 0;
  int animationFlag = 0;
  int offScreenFlag = 0;
  char *path = NULL;
      
  file = new ofstream(filename, ios::out);
  if (file->fail())
    {
    vtkErrorMacro("Could not open file " << filename);
    delete file;
    file = NULL;
    return;
    }

  *file << "# ParaView Version " << this->GetPVApplication()->GetMajorVersion()
	   << "." << this->GetPVApplication()->GetMinorVersion() << "\n\n";

  if (this->PackageNames->GetNumberOfItems() > 0)
    {
    *file << vtkPVApplication::LoadComponentProc << endl;
    vtkLinkedListIterator<const char*>* it = this->PackageNames->NewIterator();
    while (it->IsDoneWithTraversal() != VTK_OK)
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

  if (vtkFlag)
    {
    *file << "package require vtk\n"
          << "package require vtkinteraction\n"
          << "# create a rendering window and renderer\n";
    }
  else
    {
    *file << "# Script generated for regression test within ParaView.\n";
    }


  // Descide what this script should do.
  // Save an image or series of images, or run interactively.
  const char *script = this->AnimationInterface->GetScript();
  if (script && strlen(script) > 0)
    {
    if (vtkKWMessageDialog::PopupYesNo(
	  this->Application, this, "Animation", 
	  "Do you want your script to generate an animation?", 
	  vtkKWMessageDialog::QuestionIcon))
      {
      animationFlag = 1;
      }
    }
  
  if (animationFlag == 0)
    {
    if (vtkKWMessageDialog::PopupYesNo(
	  this->Application, this, "Image", 
	  "Do you want your script to save an image?", 
	  vtkKWMessageDialog::QuestionIcon))
      {
      imageFlag = 1;
      }
    }

  if (animationFlag || imageFlag)
    {
    this->Script("tk_getSaveFile -title {Save Image} -defaultextension {.jpg} -filetypes {{{JPEG Images} {.jpg}} {{PNG Images} {.png}} {{Binary PPM} {.ppm}} {{TIFF Images} {.tif}}}");
    path = strcpy(
      new char[strlen(this->Application->GetMainInterp()->result)+1], 
      this->Application->GetMainInterp()->result);
    }

  if (path && strlen(path) > 0)
    {
    if (vtkKWMessageDialog::PopupYesNo(this->Application, this, "Offscreen", 
				       "Do you want offscreen rendering?", 
				       vtkKWMessageDialog::QuestionIcon))
      {
      offScreenFlag = 1;
      }
    }

  this->GetMainView()->SaveInTclScript(file, vtkFlag, offScreenFlag);

  vtkArrayMapIterator<const char*, vtkPVSourceCollection*>* it =
    this->SourceLists->NewIterator();

  // Mark all sources as not visited.
  while( it->IsDoneWithTraversal() != VTK_OK )
    {
    vtkPVSourceCollection* col = 0;
    if (it->GetData(col) == VTK_OK && col)
      {
      col->InitTraversal();
      while ( (pvs = col->GetNextPVSource()) ) 
	{
	pvs->SetVisitedFlag(0);
	}
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Loop through sources saving the visible sources.
  vtkPVSourceCollection* modules = this->GetSourceList("Sources");
  modules->InitTraversal();
  while ( (pvs = modules->GetNextPVSource()) ) 
    {
    pvs->SaveInTclScript(file);
    }

  *file << "vtkCompositeManager compManager\n\t";
  *file << "compManager SetRenderWindow RenWin1 \n\t";
  *file << "compManager InitializePieces\n\n";

  if (path && strlen(path) > 0)
    {
    *file << "compManager ManualOn\n\t";
    *file << "if {[catch {set myProcId [[compManager GetController] GetLocalProcessId]}]} {set myProcId 0 } \n\n";
    *file << "vtkWindowToImageFilter WinToImage\n\t";
    *file << "WinToImage SetInput RenWin1\n";
    *file << "vtkJPEGWriter Writer\n\t";
    *file << "Writer SetInput [WinToImage GetOutput]\n\n";
    if (offScreenFlag)
      {
      *file << "RenWin1 SetOffScreenRendering 1\n\n";
      }    
   
    if (imageFlag)
      {
      *file << "if {$myProcId != 0} {compManager RenderRMI} else {\n\t";
      *file << "RenWin1 Render\n\t";
      *file << "Writer SetFileName {" << path << "}\n\t";
      *file << "Writer Write\n";
      *file << "}\n\n";
      }
    if (animationFlag)
      {
      int length = strlen(path);
      if (strcmp(path+length-4, ".jpg") == 0)
        {
        path[length-4] = '\0';
        }
      this->AnimationInterface->SaveInTclScript(file, path);
      }
    delete [] path;
    *file << "vtkCommand DeleteAllObjects\n";
    *file << "exit";
    }
  else
    {
    if (vtkFlag)
      {
      *file << "# enable user interface interactor\n"
            << "iren SetUserMethod {wm deiconify .vtkInteract}\n"
            << "iren Initialize\n\n"
            << "# prevent the tk window from showing up then start the event loop\n"
            << "wm withdraw .\n";
      }
    }

  if (file)
    {
    file->close();
    delete file;
    file = NULL;
    }
}

//----------------------------------------------------------------------------
// Not implemented yet.
void vtkPVWindow::SaveWorkspace()
{
}

//----------------------------------------------------------------------------
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
  const char* key;
  int numFilters = 0;
  while ( it->IsDoneWithTraversal() != VTK_OK )
    {
    proto = 0;
    if (it->GetData(proto) == VTK_OK)
      {
      // Check if this is a source. We do not want to add filters
      // to the source lists.
      if (proto && !proto->GetInputClassName())
	{
	numFilters++;
	char methodAndArgs[150];
	it->GetKey(key);
	sprintf(methodAndArgs, "CreatePVSource %s", key);
	this->SourceMenu->AddCommand(key, this, methodAndArgs);
	}
      }
    it->GoToNextItem();
    }
  it->Delete();

  // If there are no filters, disable the menu.
  if (numFilters > 0)
    {
    this->Script("%s entryconfigure \"VTK Sources\" -state normal",
		 this->AdvancedMenu->GetWidgetName());
    }
  else
    {
    this->Script("%s entryconfigure \"VTK Sources\" -state disabled",
		 this->AdvancedMenu->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkPVWindow::UpdateFilterMenu()
{
  if (!this->FilterMenu)
    {
    vtkWarningMacro("Filter menu does not exist. Can not update.");
    return;
    }

  // Remove all of the entries from the filter menu.
  this->FilterMenu->DeleteAllMenuItems();

  if (this->CurrentPVData && this->CurrentPVSource &&
      this->CurrentPVSource->GetIsDeletable())
    {
    // Add all the appropriate filters to the filter menu.
    vtkArrayMapIterator<const char*, vtkPVSource*>* it = 
      this->Prototypes->NewIterator();
    vtkPVSource* proto;
    const char* key;
    int numSources = 0;
    while ( it->IsDoneWithTraversal() != VTK_OK )
      {
      proto = 0;
      if (it->GetData(proto) == VTK_OK)
	{
	// Check if this is an appropriate filter by comparing
	// it's input type with the current data object's type.
	if (proto && proto->GetIsValidInput(this->CurrentPVData))
	  {
	  numSources++;
	  char methodAndArgs[150];
	  it->GetKey(key);
	  sprintf(methodAndArgs, "CreatePVSource %s", key);
	  // Remove "vtk" from the class name to get the menu item name.
	  this->FilterMenu->AddCommand(key, this, methodAndArgs);
	  }
	}
      it->GoToNextItem();
      }
    it->Delete();
    
    // If there are no sources, disable the menu.
    if (numSources > 0)
      {
      this->Script("%s entryconfigure \"VTK Filters\" -state normal",
		   this->AdvancedMenu->GetWidgetName());
      }
    else
      {
      this->Script("%s entryconfigure \"VTK Filters\" -state disabled",
		   this->AdvancedMenu->GetWidgetName());
      }
    this->EnableToolbarButtons();
    }
  else
    {
    // If there is no current data, disable the menu.
    this->DisableToolbarButtons();
    this->Script("%s entryconfigure \"VTK Filters\" -state disabled",
		 this->AdvancedMenu->GetWidgetName());
    }
  
}

//----------------------------------------------------------------------------
void vtkPVWindow::SetCurrentPVData(vtkPVData *pvd)
{
  if (this->CurrentPVData)
    {
    this->CurrentPVData->UnRegister(this);
    this->CurrentPVData = NULL;
    }
  if (pvd)
    {
    pvd->Register(this);
    }
  this->CurrentPVData = pvd;

  this->UpdateFilterMenu();
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkPVWindow::SetCurrentPVSourceCallback(vtkPVSource *pvs)
{
  this->SetCurrentPVSource(pvs);

  if (pvs)
    {
    pvs->SetAcceptButtonColorToWhite();
    if (pvs->InitializeTrace())
      {
      this->GetPVApplication()->AddTraceEntry(
	"$kw(%s) SetCurrentPVSourceCallback $kw(%s)", 
	this->GetTclName(), pvs->GetTclName());
      }
    }
  else
    {
    this->GetPVApplication()->AddTraceEntry(
      "$kw(%s) SetCurrentPVSourceCallback NULL", this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkPVWindow::SetCurrentPVSource(vtkPVSource *pvs)
{

  // Handle selection.
  if (this->CurrentPVSource)
    {
    this->CurrentPVSource->Deselect();
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
  if (pvs)
    {
    this->SetCurrentPVData(pvs->GetPVOutput());
    }
  else
    {
    this->SetCurrentPVData(NULL);
    }

  // This will update the parameters.  
  // I doubt the conditional is still necessary.
  if (pvs)
    {
    this->ShowCurrentSourceProperties();
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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


//----------------------------------------------------------------------------
vtkPVSourceCollection* vtkPVWindow::GetSourceList(const char* listname)
{
  vtkPVSourceCollection* col=0;
  if (this->SourceLists->GetItem(listname, col) == VTK_OK)
    {
    return col;
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVWindow::ResetCameraCallback()
{
  this->GetPVApplication()->AddTraceEntry("$kw(%s) ResetCameraCallback", 
                                          this->GetTclName());

  this->MainView->ResetCamera();
  this->MainView->EventuallyRender();
}

//----------------------------------------------------------------------------
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
    glyphSources->InitTraversal();
    while ( (source = glyphSources->GetNextPVSource()) )
      {
      sprintf(methodAndArg, "SetCurrentPVSourceCallback %s", 
	      source->GetTclName());
      this->GlyphMenu->AddCommand(source->GetName(), this, methodAndArg,
				  source->GetVTKSource() ?
				  source->GetVTKSource()->GetClassName()+3
				  : 0);
      numGlyphs++;
      }
    }


  vtkPVSourceCollection* sources = this->GetSourceList("Sources");
  sources->InitTraversal();
  int numSources = 0;
  while ( (source = sources->GetNextPVSource()) )
    {
    sprintf(methodAndArg, "SetCurrentPVSourceCallback %s", 
	    source->GetTclName());
    this->SelectMenu->AddCommand(source->GetName(), this, methodAndArg,
				 source->GetVTKSource() ?
				 source->GetVTKSource()->GetClassName()+3
				 : 0);
    numSources++;
    }

  if (numGlyphs > 0)
    {
    this->SelectMenu->AddCascade("Glyphs", this->GlyphMenu, 0,
				 "Select one of the glyph sources.");  
    }

  // Disable or enable the menu.
  this->EnableSelectMenu();
}

//----------------------------------------------------------------------------
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
  
  if (numSources == 0 && numGlyphs == 0)
    {
    this->Script("%s entryconfigure Select -state disabled",
                 this->Menu->GetWidgetName());
    this->Script("%s entryconfigure \" Source\" -state disabled",
                 this->MenuProperties->GetWidgetName());
    }
  else
    {
    this->Script("%s entryconfigure Select -state normal",
                 this->Menu->GetWidgetName());
    this->Script("%s entryconfigure \" Source\" -state normal",
                 this->MenuProperties->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkPVWindow::DisableMenus()
{
  int numMenus;
  int i;

  this->Script("%s index end", this->Menu->GetWidgetName());
  numMenus = atoi(this->GetPVApplication()->GetMainInterp()->result);
  
  // deactivating menus and toolbar buttons (except the interactors)
  for (i = 0; i <= numMenus; i++)
    {
    this->Script("%s entryconfigure %d -state disabled",
                 this->Menu->GetWidgetName(), i);
    }
}

//----------------------------------------------------------------------------
void vtkPVWindow::EnableMenus()
{
  int numMenus;
  int i;

  this->Script("%s index end", this->Menu->GetWidgetName());
  numMenus = atoi(this->GetPVApplication()->GetMainInterp()->result);
  
  // deactivating menus and toolbar buttons (except the interactors)
  for (i = 0; i <= numMenus; i++)
    {
    this->Script("%s entryconfigure %d -state normal",
                 this->Menu->GetWidgetName(), i);
    }

  // Disable or enable the menu.
  this->EnableSelectMenu();
}

//----------------------------------------------------------------------------
void vtkPVWindow::DisableToolbarButtons()
{
  vtkArrayMapIterator<const char*, vtkKWPushButton*>* it = 
    this->ToolbarButtons->NewIterator();
  while ( it->IsDoneWithTraversal() != VTK_OK )
    {
    vtkKWPushButton* button = 0;
    if (it->GetData(button) == VTK_OK && button)
      {
      this->Script("%s configure -state disabled", button->GetWidgetName());
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVWindow::EnableToolbarButtons()
{
  if (this->CurrentPVData == NULL)
    {
    return;
    }

  vtkArrayMapIterator<const char*, vtkKWPushButton*>* it = 
    this->ToolbarButtons->NewIterator();
  while ( it->IsDoneWithTraversal() != VTK_OK )
    {
    vtkKWPushButton* button = 0;
    if (it->GetData(button) == VTK_OK && button)
      {
      this->Script("%s configure -state normal", button->GetWidgetName());
      }
    it->GoToNextItem();
    }
  it->Delete();

  // The ExtractGrid button (if there is one) is context dependent.
  // It is enabled only of the current data is image data, structured
  // points, rectilinear grid or structured grid.
  int type = this->CurrentPVData->GetVTKData()->GetDataObjectType();
  if (type != VTK_IMAGE_DATA && type != VTK_STRUCTURED_POINTS &&
      type != VTK_RECTILINEAR_GRID && type != VTK_STRUCTURED_GRID)
    {
    vtkKWPushButton* button = 0;
    if (this->ToolbarButtons->GetItem("ExtractGrid", button) == VTK_OK && 
	button)
      {
      this->Script("%s configure -state disabled",
		   button->GetWidgetName());
      }
    }


}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::ExtractGridCallback()
{
  if (this->CurrentPVData == NULL)
    { // This should not be able to happen but ...
    return NULL;
    }

  int type = this->CurrentPVData->GetVTKData()->GetDataObjectType();
  if (type == VTK_IMAGE_DATA || type == VTK_STRUCTURED_POINTS)
    {
    return this->CreatePVSource("ImageClip"); 
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

//----------------------------------------------------------------------------
void vtkPVWindow::ShowCurrentSourcePropertiesCallback()
{
  this->GetPVApplication()->AddTraceEntry(
    "$kw(%s) ShowCurrentSourcePropertiesCallback", this->GetTclName());

  this->ShowCurrentSourceProperties();
}

//----------------------------------------------------------------------------
void vtkPVWindow::ShowCurrentSourceProperties()
{
  this->ShowProperties();
  
  // We need to update the properties-menu radio button too!
  this->GetMenuProperties()->CheckRadioButton(
    this->GetMenuProperties(), "Radio", 2);

  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->MainView->GetPropertiesParent()->GetWidgetName());
  this->Script("pack %s -side top -fill x -expand t",
               this->MainView->GetNavigationFrame()->GetWidgetName());
  
  if (!this->GetCurrentPVSource())
    {
    return;
    }
  
  this->Script("pack %s -side top -fill x -expand t",
               this->GetCurrentPVSource()->GetNotebook()->GetWidgetName());
  this->GetCurrentPVSource()->GetNotebook()->Raise("Source");
}
//----------------------------------------------------------------------------
void vtkPVWindow::ShowAnimationProperties()
{
  this->GetPVApplication()->AddTraceEntry("$kw(%s) ShowAnimationProperties",
                                          this->GetTclName());

  this->AnimationInterface->UpdateSourceMenu();

  // Try to find a good default value for the source.
  if (this->AnimationInterface->GetPVSource() == NULL)
    {
    vtkPVSource *pvs = this->GetCurrentPVSource();
    if (pvs == NULL && this->GetSourceList("Sources")->GetNumberOfItems() > 0)
      {
      pvs = (vtkPVSource*)this->GetSourceList("Sources")->GetItemAsObject(0);
      }
    this->AnimationInterface->SetPVSource(pvs);
    }

  // What does this do?
  this->ShowProperties();
  
  // We need to update the properties-menu radio button too!
  this->GetMenuProperties()->CheckRadioButton(
    this->GetMenuProperties(), "Radio", 3);

  // Get rid of the page already packed.
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->MainView->GetPropertiesParent()->GetWidgetName());
  // Put our page in.
  this->Script("pack %s -side top -expand t -fill x -ipadx 3 -ipady 3",
               this->AnimationInterface->GetWidgetName());
}

//----------------------------------------------------------------------------
vtkPVApplication *vtkPVWindow::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->Application);
}


//----------------------------------------------------------------------------
void vtkPVWindow::ShowLog()
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

//----------------------------------------------------------------------------
void vtkPVWindow::SaveTrace()
{
  ofstream *trace = this->GetPVApplication()->GetTraceFile();
  if ( ! trace)
    {
    return;
    }
  
  char *filename;
  
  this->Script("tk_getSaveFile -filetypes {{{ParaView Script} {.pvs}}} -defaultextension .pvs");
  filename = new char[strlen(this->Application->GetMainInterp()->result)+1];
  sprintf(filename, "%s", this->Application->GetMainInterp()->result);
  
  if (strcmp(filename, "") == 0)
    {
    delete [] filename;
    return;
    }
  
  trace->close();
  
  const int bufferSize = 4096;
  char buffer[bufferSize];

  ofstream newTrace("ParaViewTrace.pvs");
  ifstream oldTrace(filename);
  
  while(oldTrace)
    {
    oldTrace.read(buffer, bufferSize);
    if(oldTrace.gcount())
      {
      newTrace.write(buffer, oldTrace.gcount());
      }
    }

  trace->open("ParaViewTrace.pvs", ios::in | ios::app );
}

//----------------------------------------------------------------------------
// Create a new data object/source by cloning a module prototype.
vtkPVSource *vtkPVWindow::CreatePVSource(const char* className,
                                         const char* sourceList,
					 int addTraceEntry)
{
  vtkPVSource *pvs = 0;
  vtkPVSource* clone = 0;
  int success;

  if ( this->Prototypes->GetItem(className, pvs) == VTK_OK ) 
    {
    // Make the cloned source current only if it is going into
    // the Sources list.
    if (sourceList && strcmp(sourceList, "Sources") != 0)
      {
      success = pvs->ClonePrototype(0, clone);
      }
    else
      {
      success = pvs->ClonePrototype(1, clone);
      }

    if (success != VTK_OK)
      {
      vtkErrorMacro("Cloning operation for " << className
		    << " failed.");
      return 0;
      }

    if (!clone)
      {
      return 0;
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
	    className, sourceList);
	  }
	else
	  {
	  this->GetPVApplication()->AddTraceEntry(
	    "set kw(%s) [$kw(%s) CreatePVSource %s]", 
	    clone->GetTclName(), this->GetTclName(),
	    className);
	  }
	clone->SetTraceInitialized(1);
	}
      }

    clone->UpdateParameterWidgets();

    vtkPVSourceCollection* col = 0;
    if(sourceList)
      {
      col = this->GetSourceList(sourceList);
      }
    else
      {
      col = this->GetSourceList("Sources");
      }
    
    if (col)
      {
      col->AddItem(clone);
      }
    else
      {
      vtkWarningMacro("The specified source list (" 
		      << (sourceList ? sourceList : "Sources") 
		      << ") could not be found.")
      }
    clone->Delete();
    }

  return clone;
}

//----------------------------------------------------------------------------
void vtkPVWindow::DisplayCommandPrompt()
{
  if ( ! this->TclInteractor )
    {
    this->TclInteractor = vtkKWTclInteractor::New();
    this->TclInteractor->SetTitle("Command Prompt");
    this->TclInteractor->SetMasterWindow(this);
    this->TclInteractor->Create(this->GetPVApplication());
    }
  
  this->TclInteractor->Display();
}

//----------------------------------------------------------------------------
int vtkPVWindow::OpenPackage()
{

  char buffer[1024];
  // Retrieve old path from the registery
  if ( !this->GetApplication()->GetRegisteryValue(
	 2, "RunTime", "PackagePath", buffer) )
    {
    sprintf(buffer, ".");
    }

  this->Script(
    "set openFileName [tk_getOpenFile -initialdir {%s} "
    "-filetypes {{{ParaView Package Files} {*.xml}} {{All Files} {*.*}}}]", 
    buffer);

  char* openFileName = new char[
    strlen(this->GetPVApplication()->GetMainInterp()->result) + 1];
  strcpy(openFileName, this->GetPVApplication()->GetMainInterp()->result);
  
  if (strcmp(openFileName, "") == 0)
    {
    return VTK_ERROR;
    }

  return this->OpenPackage(openFileName);

}

//----------------------------------------------------------------------------
int vtkPVWindow::OpenPackage(const char* openFileName)
{
  if ( this->CheckIfFileIsReadable(openFileName) != VTK_OK )
    {
    return VTK_ERROR;
    }

  this->ReadSourceInterfacesFromFile(openFileName);

  // Store last path
  if ( openFileName && strlen(openFileName) > 0 )
    {
    char *pth = new char [strlen(openFileName)+1];
    sprintf(pth,"%s",openFileName);
    int pos = strlen(openFileName);
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

//----------------------------------------------------------------------------
void vtkPVWindow::ReadSourceInterfaces()
{
  // Add special sources.
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Setup our built in source interfaces.
  this->ReadSourceInterfacesFromString(vtkPVWindow::StandardSourceInterfaces);
  this->ReadSourceInterfacesFromString(vtkPVWindow::StandardFilterInterfaces);
  this->ReadSourceInterfacesFromString(vtkPVWindow::StandardReaderInterfaces);
  
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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
    int extPos = strlen(file)-4;
    
    // Look for the ".xml" extension.
    if((extPos > 0) && (strcmp(file+extPos, ".xml") == 0))
      {
      char* fullPath = new char[strlen(file)+strlen(directory)+2];
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

//----------------------------------------------------------------------------
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


//----------------------------------------------------------------------------
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
    sprintf(newStr, "%s {{%s} {%s}}", this->FileDescriptions, description, ext);
    }
  if (this->FileDescriptions)
    {
    delete [] this->FileDescriptions;
    }
  this->FileDescriptions = newStr;
  newStr = NULL;

  this->ReaderList->AppendItem(prototype);
  this->Script("%s entryconfigure \"Open Data File\" -state normal",
	       this->MenuFile->GetWidgetName());

}


//----------------------------------------------------------------------------
void vtkPVWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CenterXEntry: " << this->GetCenterXEntry() << endl;
  os << indent << "CenterYEntry: " << this->GetCenterYEntry() << endl;
  os << indent << "CenterZEntry: " << this->GetCenterZEntry() << endl;
  os << indent << "CurrentPVData: " << this->GetCurrentPVData() << endl;
  os << indent << "FilterMenu: " << this->GetFilterMenu() << endl;
  os << indent << "FlyStyle: " << this->GetFlyStyle() << endl;
  os << indent << "InteractorStyleToolbar: " << this->GetInteractorToolbar() 
     << endl;
  os << indent << "MainView: " << this->GetMainView() << endl;
  os << indent << "RotateCameraStyle: " << this->GetRotateCameraStyle() << endl;
  os << indent << "SelectMenu: " << this->GetSelectMenu() << endl;
  os << indent << "SourceMenu: " << this->GetSourceMenu() << endl;
  os << indent << "Toolbar: " << this->GetToolbar() << endl;
  os << indent << "TranslateCameraStyle: " << this->GetTranslateCameraStyle() << endl;
}

// Define the standard reader interfaces.
const char* vtkPVWindow::StandardReaderInterfaces =
"<ModuleInterfaces>\n"
"  <Module name=\"ensight\" class=\"vtkPVEnSightReaderModule\" root_name=\"ensight\" \n"
"          output=\"vtkDataSet\" module_type=\"Reader\"\n"
"          extensions=\".case\" file_description=\"EnSight Files\">\n"
"    <Source type=\"vtkGenericEnSightReader\"/>\n"
"  </Module>\n"
"  <Module name=\"pvtkfile\" class=\"vtkPVDataSetReaderModule\" root_name=\"pvtkfile\"\n"
"          output=\"vtkDataSet\" module_type=\"Reader\"\n"
"          extensions=\".pvtk\" file_description=\"Parallel VTK Files\">\n"
"    <Source type=\"vtkPDataSetReader\"/>\n"
"  </Module>\n"
"  <Module name=\"vtkfile\" class=\"vtkPVDataSetReaderModule\" root_name=\"vtkfile\" \n"
"          output=\"vtkDataSet\" module_type=\"Reader\"\n"
"          extensions=\".vtk\" file_description=\"VTK Files\">\n"
"    <Source type=\"vtkPDataSetReader\"/>\n"
"  </Module>\n"
"  <Module name=\"stlreader\" class=\"vtkPVReaderModule\" root_name=\"stl\" \n"
"          output=\"vtkPolyData\" module_type=\"Reader\"\n"
"          extensions=\".stl\" file_description=\"Stereo Lithography\">\n"
"    <Source type=\"vtkSTLReader\"/>\n"
"  </Module>\n"
"  <Module name=\"popreader\" class=\"vtkPVAdvancedReaderModule\" \n"
"          root_name=\"pop\" output=\"vtkStructuredGrid\"\n"
"          module_type=\"Reader\"\n"
"          extensions=\".pop\" file_description=\"POP Ocean Files\">\n"
"    <Source type=\"vtkPOPReader\"/>\n"
"    <VectorEntry variable=\"Radius\" type=\"float\" \n"
"                 help=\"Set the radius of the data set\"/>\n"
"    <VectorEntry variable=\"ClipExtent\" type=\"int\" length=\"6\" \n"
"                 help=\"For reading a smaller extent.\"/>\n"
"    <VectorEntry variable=\"NumberOfGhostLevels\" type=\"int\" \n"
"                 help=\"Set the number of ghost levels to read.\"/>\n"
"  </Module>\n"
"</ModuleInterfaces>\n";

// Define the standard source interfaces.
const char* vtkPVWindow::StandardSourceInterfaces =
"<ModuleInterfaces>\n"
"\n"
"<Module name=\"ArrowSource\" root_name=\"Arrow\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkArrowSource\"/>\n"
"  <VectorEntry variable=\"TipResolution\" type=\"int\" help=\"Set the number of faces on the tip.\"/>\n"
"  <VectorEntry variable=\"TipRadius\" type=\"float\" help=\"Set the radius of the widest part of the tip.\"/>\n"
"  <VectorEntry variable=\"TipLength\" type=\"float\" help=\"Set the length of the tip (the whole arrow is length 1)\"/>\n"
"  <VectorEntry variable=\"ShaftResolution\" type=\"int\" help=\"Set the number of faces on shaft\"/>\n"
"  <VectorEntry variable=\"ShaftRadius\" type=\"float\" help=\"Set the radius of the shaft\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"Axes\" root_name=\"Axes\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkAxes\"/>\n"
"  <VectorEntry label=\"Scale\" variable=\"ScaleFactor\" type=\"float\" help=\"Set the size of the axes\"/>\n"
"  <VectorEntry variable=\"Origin\" type=\"float\" length=\"3\" help=\"Set the x, y, z coordinates of the origin of the axes\"/>\n"
"  <LabeledToggle variable=\"Symmetric\" help=\"Select whether to display the negative axes\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ConeSource\" root_name=\"Cone\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkConeSource\"/>\n"
"  <VectorEntry variable=\"Resolution\" type=\"int\" help=\"Set the number of faces on this cone\"/>\n"
"  <VectorEntry variable=\"Radius\" type=\"float\" help=\"Set the radius of the widest part of the cone\"/>\n"
"  <VectorEntry variable=\"Height\" type=\"float\" help=\"Set the height of the cone\"/>\n"
"  <LabeledToggle variable=\"Capping\" help=\"Set whether to draw the base of the cone\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"CubeSource\" root_name=\"Cube\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkCubeSource\"/>\n"
"  <VectorEntry variable=\"XLength\" type=\"int\" help=\"The length of the cube in the x direction.\"/>\n"
"  <VectorEntry variable=\"YLength\" type=\"int\" help=\"The length of the cube in the y direction.\"/>\n"
"  <VectorEntry variable=\"ZLength\" type=\"int\" help=\"The length of the cube in the z direction.\"/>\n"
"  <VectorEntry variable=\"Center\" type=\"float\" length=\"3\" help=\"Set the center of the cube.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"CylinderSource\" root_name=\"Cyl\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkCylinderSource\"/>\n"
"  <VectorEntry variable=\"Resolution\" type=\"int\" help=\"The number of facets used to define the cylinder.\"/>\n"
"  <VectorEntry variable=\"Height\" type=\"float\" help=\"The height of the cylinder (along the y axis).\"/>\n"
"  <VectorEntry variable=\"Radius\" type=\"float\" help=\"The radius of the cylinder.\"/>\n"
"  <VectorEntry variable=\"Center\" type=\"float\" length=\"3\" help=\"Set the center of the cylinder.\"/>\n"
"  <LabeledToggle variable=\"Capping\" help=\"Set whether to draw the ends of the cylinder\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ImageMandelbrotSource\" root_name=\"Fractal\" output=\"vtkImageData\" module_type=\"Source\">\n"
"  <Source type=\"vtkImageMandelbrotSource\"/>\n"
"  <VectorEntry label=\"Extent\" variable=\"WholeExtent\" type=\"int\" length=\"6\" help=\"Set the min and max values of the data in each dimension\"/>\n"
"  <VectorEntry label=\"SubSpace\" variable=\"ProjectionAxes\" type=\"int\" length=\"3\" help=\"Choose which axes of the data set to display\"/>\n"
"  <VectorEntry label=\"Origin\" variable=\"OriginCX\" type=\"float\" length=\"4\" help=\"Set the imaginary and real values for C (constant) and X (initial value)\"/>\n"
"  <VectorEntry label=\"Size\" variable=\"SizeCX\" type=\"float\" length=\"4\" help=\"Set the inaginary and real values for the world size of the volume for C (constant) and X (initial value)\"/>\n"
"  <VectorEntry label=\"MaxIterations\" variable=\"MaximumNumberOfIterations\" type=\"int\" help=\"The limit on computational iterations. Values less than 2.0 after this many iterations are considered in the set.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ImageReader\" root_name=\"ImageRead\" output=\"vtkImageData\" module_type=\"Source\">\n"
"  <Source type=\"vtkImageReader\"/>\n"
"  <FileEntry variable=\"FilePrefix\" extension=\"\" help=\"Set the file root. Used with file pattern to format a file name.\"/>  \n"
"  <StringEntry variable=\"FilePattern\" help=\"Set the format string.\"/>\n"
"  <SelectionList variable=\"DataScalarType\" help=\"Select the pixel type in the file: short, int, float ...\">\n"
"    <Item name=\"char\" value=\"1\"/>\n"
"    <Item name=\"unsigned char\" value=\"2\"/>\n"
"    <Item name=\"short\" value=\"3\"/>\n"
"    <Item name=\"unsigned short\" value=\"4\"/>\n"
"    <Item name=\"int\" value=\"5\"/>\n"
"    <Item name=\"unsigned int\" value=\"6\"/>\n"
"    <Item name=\"long\" value=\"7\"/>\n"
"    <Item name=\"unsigned short\" value=\"8\"/>\n"
"    <Item name=\"unsigned long\" value=\"9\"/>\n"
"    <Item name=\"float\" value=\"10\"/>\n"
"    <Item name=\"double\" value=\"11\"/>\n"
"  </SelectionList>\n"
"  <SelectionList variable=\"DataByteOrder\" help=\"Select the pixel type in the file: short, int, float ...\">\n"
"    <Item name=\"big endian\" value=\"0\"/>\n"
"    <Item name=\"little endian\" value=\"1\"/>\n"
"  </SelectionList>\n"
"  <VectorEntry label=\"Extent\" variable=\"DataExtent\" type=\"int\" length=\"6\" help=\"Set the min and max values of the data in each dimension\"/>\n"
"  <VectorEntry variable=\"NumberOfScalarComponents\" type=\"int\" help=\"How many componenets does each pixel have. (i.e. RGB would be 3.)\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"OutlineCornerSource\" root_name=\"Corners\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkOutlineCornerSource\"/>\n"
"  <VectorEntry variable=\"Bounds\" type=\"float\" length=\"6\" help=\"Bounds of the outline.\"/>\n"
"  <VectorEntry variable=\"CornerFactor\" type=\"float\" help=\"The relative size of the corners to the length of the corresponding bounds. (0.001 -> 0.5)\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ParticleReader\" root_name=\"Particle\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkParticleReader\"/>\n"
"  <FileEntry variable=\"FileName\" extension=\"*\" help=\"Set the file to read.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"PLOT3DReader\" root_name=\"Plot3D\" output=\"vtkStructuredGrid\" module_type=\"Source\">\n"
"  <Source type=\"vtkPLOT3DReader\"/>\n"
"  <FileEntry variable=\"XYZFileName\" extension=\"bin\" help=\"Set the geometry file to read.\"/>  \n"
"  <FileEntry variable=\"QFileName\" extension=\"bin\" help=\"Set the data file to read.\"/>\n"
"  <VectorEntry variable=\"ScalarFunctionNumber\" type=\"int\" help=\"PLOT3D number for scalars\"/>\n"
"  <VectorEntry variable=\"VectorFunctionNumber\" type=\"int\" help=\"PLOT3D number for vectors\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"POPReader\" root_name=\"POPReader\" output=\"vtkStructuredGrid\" module_type=\"Source\">\n"
"  <Source type=\"vtkPOPReader\"/>\n"
"  <FileEntry variable=\"FileName\" extension=\"pop\" help=\"Select the file for the data set\"/>\n"
"  <VectorEntry variable=\"Radius\" type=\"float\" help=\"Set the radius of the data set\"/>\n"
"  <VectorEntry variable=\"ClipExtent\" type=\"int\" length=\"6\" help=\"For reading a smaller extent.\"/>\n"
"  <VectorEntry variable=\"NumberOfGhostLevels\" type=\"int\" help=\"Set the number of ghost levels to read.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"RTAnalyticSource\" root_name=\"RTSource\" output=\"vtkStructuredPoints\" module_type=\"Source\">\n"
"  <Source type=\"vtkRTAnalyticSource\"/>\n"
"  <VectorEntry variable=\"WholeExtent\" type=\"int\" length=\"6\" help=\"Extent (dimensions) of the function.\"/>\n"
"  <VectorEntry variable=\"Center\" type=\"int\" length=\"3\" help=\"Center of the function.\"/>\n"
"  <VectorEntry variable=\"Maximum\" type=\"float\" help=\"Maximum value of the function\"/>\n"
"  <VectorEntry variable=\"XFreq\" type=\"float\" help=\"Frequency in X\"/>\n"
"  <VectorEntry variable=\"YFreq\" type=\"float\" help=\"Frequency in Y\"/>\n"
"  <VectorEntry variable=\"ZFreq\" type=\"float\" help=\"Frequency in Z\"/>\n"
"  <VectorEntry variable=\"XMag\" type=\"float\" help=\"Wave amplitude in X\"/>\n"
"  <VectorEntry variable=\"YMag\" type=\"float\" help=\"Wave amplitude in Y\"/>\n"
"  <VectorEntry variable=\"ZMag\" type=\"float\" help=\"Wave amplitude in Z\"/>\n"
"  <VectorEntry variable=\"StandardDeviation\" type=\"float\" help=\"Standard deviation of the gaussian\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"SphereSource\" root_name=\"Sphere\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkSphereSource\"/>\n"
"  <VectorEntry variable=\"Center\" type=\"float\" length=\"3\" help=\"Set the coordinates for the center of the sphere.\"/>\n"
"  <VectorEntry variable=\"Radius\" type=\"float\" help=\"Set the radius of the sphere\"/>\n"
"  <VectorEntry label=\"Theta Resolution\" variable=\"ThetaResolution\" type=\"int\" help=\"Set the number of points in the longitude direction (ranging from Start Theta to End Theta)\"/>\n"
"  <VectorEntry label=\"Start Theta\" variable=\"StartTheta\" type=\"float\" help=\"Set the starting angle in the longitude direction\"/>\n"
"  <VectorEntry label=\"End Theta\" variable=\"EndTheta\" type=\"float\" help=\"Set the ending angle in the longitude direction\"/>\n"
"  <VectorEntry label=\"Phi Resolution\" variable=\"PhiResolution\" type=\"int\" help=\"Set the number of points in the latitude direction (ranging from Start Phi to End Phi)\"/>\n"
"  <VectorEntry label=\"Start Phi\" variable=\"StartPhi\" type=\"float\" help=\"Set the starting angle in the latitude direction\"/>\n"
"  <VectorEntry label=\"End Phi\" variable=\"EndPhi\" type=\"float\" help=\"Set the ending angle in the latitude direction\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"STLReader\" root_name=\"STLReader\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkSTLReader\"/>\n"
"  <FileEntry variable=\"FileName\" extension=\"stl\" help=\"Select the data file for the STL data set\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"SuperquadricSource\" root_name=\"SQuad\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkSuperquadricSource\"/>\n"
"  <VectorEntry variable=\"Center\" type=\"float\" length=\"3\" help=\"Set the center of the superquadric\"/>\n"
"  <VectorEntry variable=\"Scale\" type=\"float\" length=\"3\" help=\"Set the scale of the superquadric\"/>\n"
"  <VectorEntry variable=\"ThetaResolution\" type=\"int\" help=\"The number of points in the longitude direction.\"/>\n"
"  <VectorEntry variable=\"PhiResolution\" type=\"int\" help=\"The number of points in the latitude direction.\"/>\n"
"  <VectorEntry variable=\"Thickness\" type=\"float\" help=\"Changing thickness maintains the outside diameter of the toroid.\"/>\n"
"  <VectorEntry variable=\"ThetaRoundness\" type=\"float\" help=\"Values range from 0 (rectangular) to 1 (circular) to higher order.\"/>\n"
"  <VectorEntry variable=\"PhiRoundness\" type=\"float\" help=\"Values range from 0 (rectangular) to 1 (circular) to higher order.\"/>\n"
"  <VectorEntry variable=\"Size\" type=\"float\" help=\"Isotropic size\"/>\n"
"  <LabeledToggle variable=\"Toroidal\" help=\"Whether or not the superquadric is toroidal or ellipsoidal\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"VectorText\" root_name=\"VectorText\" output=\"vtkPolyData\" module_type=\"Source\">\n"
"  <Source type=\"vtkVectorText\"/>\n"
"  <StringEntry variable=\"Text\" help=\"Enter the text to display\"/>\n"
"</Module>\n"
"\n"
"</ModuleInterfaces>\n";

// Define the standard filter interfaces.
const char* vtkPVWindow::StandardFilterInterfaces=
"<ModuleInterfaces>\n"
"<Module name=\"BrownianPoints\" root_name=\"BPts\" input=\"vtkDataSet\" output=\"vtkDataSet\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkBrownianPoints\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"  <VectorEntry variable=\"MinimumSpeed\" type=\"float\" help=\"The minimum size of the random point vectors generated.\"/>\n"
"  <VectorEntry variable=\"MaximumSpeed\" type=\"float\" help=\"The maximum size of the random point vectors generated.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"CellCenters\" root_name=\"Centers\" input=\"vtkDataSet\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkCellCenters\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"  <LabeledToggle variable=\"VertexCells\" help=\"Generate vertex as geometry of just points.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"PCellDataToPointData\" root_name=\"CellToPoint\" input=\"vtkDataSet\" output=\"vtkDataSet\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkPCellDataToPointData\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"  <LabeledToggle variable=\"PassCellData\" help=\" Control whether the input cell data is to be passed to the output. If on, then the input cell data is passed through to the output; otherwise, only generated point data is placed into the output.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"CleanPolyData\" root_name=\"CleanPD\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkCleanPolyData\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <LabeledToggle variable=\"PieceInvariant\" help=\"Turn this off if you do not want pieces or do not mind seams.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"DataSetSurfaceFilter\" root_name=\"Surface\" input=\"vtkDataSet\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkDataSetSurfaceFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"DataSetTriangleFilter\" root_name=\"Tetra\" input=\"vtkDataSet\" output=\"vtkUnstructuredGrid\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkDataSetTriangleFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"DecimatePro\" root_name=\"Deci\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkDecimatePro\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry variable=\"TargetReduction\" type=\"float\" help=\"Value between 0 and 1. Desired reduction of the total number of triangles.\"/>\n"
"  <LabeledToggle variable=\"PreserveTopology\" help=\"If off, better reduction can occur, but model may break up.\"/>\n"
"  <VectorEntry variable=\"FeatureAngle\" type=\"float\" help=\"Topology can be split along features.\"/>\n"
"  <LabeledToggle variable=\"BoundaryVertexDeletion\" help=\"If off, decimate will not remove points on the boundary.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ElevationFilter\" root_name=\"Elevation\" input=\"vtkDataSet\" output=\"vtkDataSet\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkElevationFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"  <VectorEntry variable=\"LowPoint\" type=\"float\" length=\"3\" help=\"Set the minimum point for the elevation\"/>\n"
"  <VectorEntry variable=\"HighPoint\" type=\"float\" length=\"3\" help=\"Set the maximum point for the elevation\"/>\n"
"  <VectorEntry variable=\"ScalarRange\" type=\"float\" length=\"2\" help=\"Set the range of scalar values to generate\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ExtractEdges\" root_name=\"ExtractEdges\" input=\"vtkDataSet\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkExtractEdges\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ExtractGrid\" root_name=\"ExtractGrid\" input=\"vtkStructuredGrid\" output=\"vtkStructuredGrid\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkExtractGrid\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkStructuredGrid\"/>\n"
"  <ExtentEntry variable=\"VOI\" help=\"Set the min/max values of the volume of interest (VOI)\"/>\n"
"  <VectorEntry variable=\"SampleRate\" type=\"int\" length=\"3\" help=\"Set the sampling rate for each dimension\"/>\n"
"  <LabeledToggle variable=\"IncludeBoundary\" help=\"Select whether to always include the boundary of the grid in the output\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ExtractRectilinearGrid\" root_name=\"ExtractRGrid\" input=\"vtkRectilinearGrid\" output=\"vtkRectilinearGrid\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkExtractRectilinearGrid\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkRectilinearGrid\"/>\n"
"  <ExtentEntry variable=\"VOI\" help=\"Set the min/max values of the volume of interest (VOI)\"/>\n"
"  <VectorEntry variable=\"SampleRate\" type=\"int\" length=\"3\" help=\"Set the sampling rate for each dimension\"/>\n"
"  <LabeledToggle variable=\"IncludeBoundary\" help=\"Select whether to always include the boundary of the grid in the output\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ImageGaussianSmooth\" root_name=\"GaussSmooth\" input=\"vtkImageData\" output=\"vtkImageData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkImageGaussianSmooth\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkImageData\"/>\n"
"  <VectorEntry variable=\"StandardDeviations\" type=\"float\" length=\"3\" help=\"STD for x, y and z axes.\"/>\n"
"  <VectorEntry variable=\"RadiusFactors\" type=\"float\" length=\"3\" help=\"Kernel size is this factor times STD.\"/>\n"
"  <SelectionList variable=\"Dimensionality\" help=\"Select whether to perform a 2d or 3d gradient\">\n"
"    <Item name=\"2\" value=\"0\"/>\n"
"    <Item name=\"3\" value=\"1\"/>\n"
"  </SelectionList>\n"
"</Module>\n"
"\n"
"<Module name=\"ImageClip\" root_name=\"ImageClip\" input=\"vtkImageData\" output=\"vtkImageData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkImageClip\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkImageData\"/>\n"
"  <ExtentEntry variable=\"OutputWholeExtent\" help=\"Set the min/max extents in each dimension of the output\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"PLinearExtrusionFilter\" root_name=\"LinExtrude\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkPLinearExtrusionFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry variable=\"ScaleFactor\" type=\"float\" help=\"Set the extrusion scale factor\"/>\n"
"  <VectorEntry variable=\"Vector\" type=\"float\" length=\"3\" help=\"Set the direction for the extrusion\"/>\n"
"  <LabeledToggle variable=\"Capping\" help=\"Select whether to draw endcaps\"/>\n"
"  <LabeledToggle variable=\"PieceInvariant\" help=\"Turn this off if you do want to process ghost levels and do not mind seams.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"LoopSubdivisionFilter\" root_name=\"LoopSubDiv\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkLoopSubdivisionFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry variable=\"NumberOfSubdivisions\" type=\"int\" help=\"Each subdivision changes single triangles into four triangles.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"MaskPoints\" root_name=\"MaskPts\" input=\"vtkDataSet\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkMaskPoints\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"  <VectorEntry variable=\"OnRatio\" type=\"int\" help=\"The ratio of points retained.\"/>\n"
"  <VectorEntry label=\"MaxPoints\" variable=\"MaximumNumberOfPoints\" type=\"int\" help=\"Limit the number of points.\"/>\n"
"  <VectorEntry variable=\"Offset\" type=\"int\" help=\"Start with this point.\"/>\n"
"  <LabeledToggle label=\"Random\" variable=\"RandomMode\" help=\"Select whether to randomly select points, or subsample regularly.\"/>\n"
"  <LabeledToggle variable=\"GenerateVertices\" help=\"Convienience feature to display points.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"OutlineCornerFilter\" root_name=\"COutline\" input=\"vtkDataSet\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkOutlineCornerFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"  <VectorEntry variable=\"CornerFactor\" type=\"float\" help=\"The relative size of the corners to the length of the corresponding bounds. (0.001 -> 0.5)\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"PieceScalars\" root_name=\"ColorPieces\" input=\"vtkDataSet\" output=\"vtkDataSet\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkPieceScalars\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"  <LabeledToggle label=\"Random\" variable=\"RandomMode\" help=\"Select whether to use random colors for the various pieces\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"PPolyDataNormals\" root_name=\"PDNormals\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkPPolyDataNormals\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry variable=\"FeatureAngle\" type=\"float\" help=\"Points are duplicated along features over this angle (0->180)\"/>\n"
"  <LabeledToggle variable=\"Splitting\" help=\"Turn on/off the splitting of sharp edges.\"/>\n"
"  <LabeledToggle variable=\"Consistency\" help=\"Turn on/off the enforcement of consistent polygon ordering.\"/>\n"
"  <LabeledToggle label=\"CellNormals\" variable=\"ComputeCellNormals\" help=\"Turn on/off the computation of cell normals.\"/>\n"
"  <LabeledToggle variable=\"FlipNormals\" help=\"Flipping reverves the meaning of front and back.\"/>\n"
"  <LabeledToggle label=\"NonManifold\" variable=\"NonManifoldTraversal\" help=\"Turn on/off traversal across non-manifold edges. This will prevent problems where the consistency of polygonal ordering is corrupted due to topological loops.\"/>\n"
"  <LabeledToggle variable=\"PieceInvariant\" help=\"Turn this off if you do not want to process ghost levels and do not mind seams.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"PVConnectivityFilter\" root_name=\"Connect\" input=\"vtkDataSet\" output=\"vtkUnstructuredGrid\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkPVConnectivityFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"QuadricClustering\" root_name=\"QC\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkQuadricClustering\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry label=\"Spacing\" variable=\"DivisionSpacing\" type=\"float\" length=\"3\" help=\"Set the spacing of the bins in each dimension\"/>\n"
"  <LabeledToggle variable=\"UseInputPoints\" help=\"Select whether to use points from the input in the output or to calculate optimum representative points for each bin\"/>\n"
"  <LabeledToggle variable=\"UseFeatureEdges\" help=\"Select whether to use feature edge quadrics to match up the boundaries between pieces\"/>\n"
"  <LabeledToggle variable=\"UseFeaturePoints\" help=\"Select whether to use feature point quadrics to align piece boundaries\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"RectilinearGridGeometryFilter\" root_name=\"RectGridGeom\" input=\"vtkRectilinearGrid\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkRectilinearGridGeometryFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkRectilinearGrid\"/>\n"
"  <ExtentEntry variable=\"Extent\" help=\"Set the min/max extents of the grid\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ReflectionFilter\" root_name=\"Reflect\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkReflectionFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <SelectionList variable=\"Plane\" help=\"Select which plane of the bounding box to reflect across\">\n"
"    <Item name=\"X Min\" value=\"0\"/>\n"
"    <Item name=\"Y Min\" value=\"1\"/>\n"
"    <Item name=\"Z Min\" value=\"2\"/>\n"
"    <Item name=\"X Max\" value=\"3\"/>\n"
"    <Item name=\"Y Max\" value=\"4\"/>\n"
"    <Item name=\"Z Max\" value=\"5\"/>\n"
"  </SelectionList>\n"
"</Module>\n"
"\n"
"<Module name=\"RibbonFilter\" root_name=\"Ribbon\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkRibbonFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry variable=\"Width\" type=\"float\" help=\"The half width of the ribbon (or minimum).\"/>\n"
"  <VectorEntry variable=\"Angle\" type=\"float\" help=\"The offset angle of the ribbon from the line normal (0->360).\"/>\n"
"  <VectorEntry variable=\"DefaultNormal\" type=\"float\" length=\"3\" help=\"If no normals are supplied\"/>\n"
"  <LabeledToggle variable=\"VaryWidth\" help=\"Turn on/off the variation of ribbon width with scalar value.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"RotationalExtrusionFilter\" root_name=\"RotExtrude\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkRotationalExtrusionFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry variable=\"Resolution\" type=\"int\"/>\n"
"  <LabeledToggle variable=\"Capping\"/>\n"
"  <VectorEntry variable=\"Angle\" type=\"float\" help=\"Set the angle of rotation.\"/>\n"
"  <VectorEntry variable=\"Translation\" type=\"float\" help=\"The total amount of translation along the z-axis.\"/>\n"
"  <VectorEntry variable=\"DeltaRadius\" type=\"float\" help=\"The change in radius during sweep process.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ShrinkFilter\" root_name=\"Shrink\" input=\"vtkDataSet\" output=\"vtkUnstructuredGrid\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkShrinkFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"  <VectorEntry variable=\"ShrinkFactor\" type=\"float\" help=\"Set the amount to shrink by\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"ShrinkPolyData\" root_name=\"ShrinkPD\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkShrinkPolyData\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry variable=\"ShrinkFactor\" type=\"float\" help=\"Set the amount to shrink by\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"SmoothPolyDataFilter\" root_name=\"Smooth\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkSmoothPolyDataFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry variable=\"NumberOfIterations\" type=\"int\" help=\"More iterations produces better smoothing.\"/>\n"
"  <VectorEntry variable=\"Convergence\" type=\"float\" help=\"Smooting factor (0->1).\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"Stripper\" root_name=\"Strip\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkStripper\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry variable=\"MaximumLength\" type=\"int\" help=\"Stop stripping after this many triangles.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"StructuredGridGeometryFilter\" root_name=\"StruGridGeom\" input=\"vtkStructuredGrid\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkStructuredGridGeometryFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkStructuredGrid\"/>\n"
"  <VectorEntry variable=\"Extent\" type=\"int\" length=\"6\" help=\"Set the min/max extents of the grid\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"StructuredGridOutlineFilter\" root_name=\"GOutline\" input=\"vtkStructuredGrid\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkStructuredGridOutlineFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkStructuredGrid\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"TriangleFilter\" root_name=\"Tri\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkTriangleFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"TubeFilter\" root_name=\"Tuber\" input=\"vtkPolyData\" output=\"vtkPolyData\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkTubeFilter\"/>\n"
"  <InputMenu label=\"Input\" help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPolyData\"/>\n"
"  <VectorEntry variable=\"NumberOfSides\" type=\"int\" help=\"Set the number of sides for the tube\"/>\n"
"  <LabeledToggle variable=\"Capping\" help=\"Select whether to draw endcaps on the tube\"/>\n"
"  <VectorEntry variable=\"Radius\" type=\"float\" help=\"Set the radius of the tube\"/>\n"
"  <SelectionList variable=\"VaryRadius\" help=\"Select whether/how to vary the radius of the tube\">\n"
"    <Item name=\"Off\" value=\"0\"/>\n"
"    <Item name=\"ByScalar\" value=\"1\"/>\n"
"    <Item name=\"ByVector\" value=\"2\"/>\n"
"  </SelectionList>\n"
"  <VectorEntry variable=\"RadiusFactor\" type=\"float\" help=\"Set the maximum tube radius in terms of a multiple of the minimum radius\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"WarpScalar\" root_name=\"WarpS\" input=\"vtkPointSet\" output=\"vtkPointSet\" default=\"scalars\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkWarpScalar\"/>\n"
"  <InputMenu id=\"im\" label=\"Input\"\n"
"             help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPointSet\"/>\n"
"  <ArrayMenu input_name=\"Input\" attribute_type=\"Scalars\"\n"
"             label=\"Scalars\" input_menu=\"im\"\n"
"             help=\"Select the input scalars to process.\"/>\n"
"  <VectorEntry variable=\"ScaleFactor\" type=\"float\" help=\"Displacement is vector times scale.\"/>\n"
"  <VectorEntry variable=\"Normal\" type=\"float\" length=\"3\" help=\"Warp direction.\"/>\n"
"  <LabeledToggle variable=\"UseNormal\" help=\"Use instance model normals rather than instance normal.\"/>\n"
"  <LabeledToggle variable=\"XYPlane\" help=\"Z value is used to warp the surface, scalars to color surface.\"/>\n"
"</Module>\n"
"\n"
"<Module name=\"WarpVector\" root_name=\"WarpV\" input=\"vtkPointSet\" output=\"vtkPointSet\" default=\"vectors\" module_type=\"Filter\">\n"
"  <Filter type=\"vtkWarpVector\"/>\n"
"  <InputMenu id=\"im\" label=\"Input\"\n"
"             help=\"Set the input to this filter.\"\n"
"             input_name=\"PVInput\" input_type=\"vtkPointSet\"/>\n"
"  <ArrayMenu input_name=\"Input\" attribute_type=\"Vectors\"\n"
"             label=\"Vectors\" input_menu=\"im\" number_of_components=\"3\"\n"
"             help=\"Select the input vectors to process.\"/>\n"
"  <VectorEntry variable=\"ScaleFactor\" type=\"float\" help=\"Displacement is vector times scale.\"/>\n"
"</Module>\n"
"\n"
"  <Module name=\"ExtractVoids\" class=\"vtkPVExtractGeometryByScalar\" \n"
"          root_name=\"ExtractVoids\"\n"
"          output=\"vtkUnstructuredGrid\" input=\"vtkUnstructuredGrid\" module_type=\"Filter\">\n"
"    <Filter type=\"vtkKWExtractGeometryByScalar\"/>\n"
"  </Module>\n"
"  <Module name=\"Calculator\" class=\"vtkPVArrayCalculator\"\n"
"          root_name=\"Calc\" button_image=\"PVCalculatorButton\"\n"
"          button_help=\"Create new arrays using mathematical "
"operations and existing arrays.\"\n"
"          output=\"vtkDataSet\" input=\"vtkDataSet\"\n"
"           module_type=\"Filter\">\n"
"    <Filter type=\"vtkArrayCalculator\"/>\n"
"  </Module>\n"
"  <Module name=\"Cut\" root_name=\"Cut\" replace_input=\"0\"\n" 
"          output=\"vtkPolyData\" input=\"vtkDataSet\"\n"
"          module_type=\"Filter\" button_image=\"PVCutButton\"\n"
"          button_help=\"Cut with an implicit plane or sphere. "
"Cutting typically reduces the dimensionality of the data.\">\n"
"    <Filter type=\"vtkCutter\"/>\n"
"    <InputMenu id=\"im1\" label=\"Input\" help=\"Set the input to this "
"filter.\"\n"
"               input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"    <BoundsDisplay show_hide_frame=\"1\" label=\"Input Bounds\"\n"
"                   input_menu=\"im1\"/>\n"
"    <SelectWidget use_widget_command=\"1\" label=\"Cut Function\"\n"
"                  variable=\"CutFunction\">\n"
"      <Item label=\"Plane\" value=\"GetPlaneTclName\">\n"
"        <PlaneWidget/>\n"
"      </Item>\n"
"      <Item label=\"Sphere\" value=\"GetSphereTclName\">\n"
"        <SphereWidget/>\n"
"      </Item>\n"
"    </SelectWidget>\n"
"    <ContourEntry label=\"Cut Values\"/>\n"
"  </Module>\n"
"  <Module name=\"Clip\" root_name=\"Clip\"\n"
"          output=\"vtkUnstructuredGrid\" input=\"vtkDataSet\"\n"
"           module_type=\"Filter\" button_image=\"PVClipButton\"\n"
"           button_help=\"Clip with an implicit plane, sphere or with scalars."
" Clipping does not reduce the dimensionality of the data set.\">\n"
"    <Filter type=\"vtkClipDataSet\"/>\n"
"    <InputMenu id=\"im1\" label=\"Input\" \n"
"               help=\"Set the input to this filter.\"\n"
"               input_name=\"PVInput\" input_type=\"vtkDataSet\"/>\n"
"    <BoundsDisplay show_hide_frame=\"1\" label=\"Input Bounds\"\n"
"                   input_menu=\"im1\"/>\n"
"    <SelectWidget use_widget_command=\"1\" label=\"Clip Function\"\n"
"                  variable=\"ClipFunction\">\n"
"      <Item label=\"Plane\" value=\"GetPlaneTclName\">\n"
"        <PlaneWidget/>\n"
"      </Item>\n"
"      <Item label=\"Sphere\" value=\"GetSphereTclName\">\n"
"        <SphereWidget/>\n"
"      </Item>\n"
"      <Item label=\"Scalars\">\n"
"        <ContainerWidget>\n"
"         <Item>\n"
"           <ScalarRangeLabel array_menu=\"it3.am1\"/>\n"
"         </Item>\n"
"         <Item id=\"it3\">\n"
"           <ArrayMenu id=\"am1\" input_name=\"Input\"\n"
"                      attribute_type=\"Scalars\"\n"
"                      label=\"Scalars\"\n"
"                      help=\"Choose the clipping scalar array.\"\n"
"                      input_menu=\"im1\"/>\n"
"         </Item>\n"
"        </ContainerWidget>\n"
"      </Item>\n"
"    </SelectWidget>\n"
"    <VectorEntry label=\"Offset\" type=\"float\" variable=\"Value\"/>\n"
"    <LabeledToggle label=\"Inside Out\" variable=\"InsideOut\"\n"
"                   help=\"Switches which part to keep.\"/>\n"
"  </Module>\n"
"  <Module name=\"Threshold\" class=\"vtkPVThreshold\" \n"
"          root_name=\"Threshold\" button_image=\"PVThresholdButton\"\n"
"          button_help=\"Extract cells that satisfy a threshold criterion.\"\n"
"          output=\"vtkUnstructuredGrid\" input=\"vtkDataSet\"\n"
"          module_type=\"Filter\">\n"
"    <Filter type=\"vtkThreshold\"/>\n"
"  </Module>\n"
"  <Module name=\"Contour\" class=\"vtkPVContour\" \n"
"          root_name=\"Contour\" button_image=\"PVContourButton\"\n"
"          button_help=\"Generate isolines or isosurfaces.\"\n"
"          output=\"vtkPolyData\" input=\"vtkDataSet\"\n"
"          module_type=\"Filter\">\n"
#ifdef VTK_USE_PATENTED
"    <Filter type=\"vtkKitwareContourFilter\"/>\n"
#else
"    <Filter type=\"vtkContourFilter\"/>\n"
#endif
"  </Module>\n"
"  <Module name=\"Glyph\" class=\"vtkPVGlyph3D\" root_name=\"Glyph\" \n"
"          input=\"vtkDataSet\" output=\"vtkPolyData\"\n"
"          module_type=\"Filter\" button_image=\"PVGlyphButton\"\n"
"          button_help=\"Generate a glyph (symbol) at each point of "
"the dataset. These can be colored, scaled and oriented\">\n"
"    <Filter type=\"vtkGlyph3D\"/>\n"
"  </Module>\n"
"  <Module name=\"Probe\" class=\"vtkPVProbe\" \n"
"          root_name=\"Probe\" button_image=\"PVProbeButton\"\n"
"          button_help=\"Sample data values at specified points or along a"
"line\"\n"
"          output=\"vtkPolyData\" input=\"vtkDataSet\"\n"
"          module_type=\"Filter\">\n"
"    <Filter type=\"vtkProbeFilter\"/>\n"
"  </Module>\n"
"</ModuleInterfaces>\n";
