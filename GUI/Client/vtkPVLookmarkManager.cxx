/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLookmarkManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkPVLookmarkManager.h"

#include "vtkPVWidgetCollection.h"
#include "vtkSMDisplayProxy.h"
#include "vtkPVInteractorStyleCenterOfRotation.h"
#include "vtkPVSelectTimeSet.h"
#include "vtkPVStringEntry.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVFileEntry.h"
#include "vtkPVLookmark.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVSelectionList.h"
#include "vtkPVScale.h"
#include "vtkPVReaderModule.h"
#include "vtkPVInputMenu.h"
#include "vtkPVArraySelection.h"
#include "vtkPVSelectWidget.h"
#include "vtkPVCameraIcon.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVMinMax.h"
#ifdef PARAVIEW_USE_EXODUS
#include "vtkPVBasicDSPFilterWidget.h"
#endif

#include "vtkKWOptionMenu.h"
#include "vtkKWLookmark.h"
#include "vtkKWLookmarkFolder.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWIcon.h"
#include "vtkKWText.h"
#include "vtkKWTclInteractor.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWidget.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWWindow.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWDragAndDropTargetSet.h"

#include "vtkXMLUtilities.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLLookmarkElement.h"

#include "vtkBase64Utilities.h"
#include "vtkRenderWindow.h"
#include "vtkCollectionIterator.h"
#include "vtkImageReader2.h"
#include "vtkJPEGWriter.h"
#include "vtkWindowToImageFilter.h"
#include "vtkJPEGReader.h"
#include "vtkImageResample.h"
#include "vtkIndent.h"
#include "vtkCamera.h"
#include "vtkVector.txx"
#include "vtkCollection.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkVectorIterator.txx"
#include "vtkImageClip.h"
#include "vtkStdString.h"
#include "vtkPVTraceHelper.h"

#include "vtkCamera.h"
#include "vtkPVProcessModule.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMDoubleVectorProperty.h"

#include "vtkSMDisplayProxy.h"

#ifndef _WIN32
  #include <sys/wait.h>
  #include <unistd.h>
#endif
#include <vtkstd/vector>
#include <vtkstd/string>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLookmarkManager);
vtkCxxRevisionMacro(vtkPVLookmarkManager, "1.24");
int vtkPVLookmarkManagerCommand(ClientData cd, Tcl_Interp *interp, int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVLookmarkManager::vtkPVLookmarkManager()
{
  this->CommandFunction = vtkPVLookmarkManagerCommand;

  this->PVLookmarks = vtkVector<vtkPVLookmark*>::New();
  this->KWLookmarks = vtkVector<vtkKWLookmark*>::New();
  this->LmkFolderWidgets = vtkVector<vtkKWLookmarkFolder*>::New();

  this->LmkPanelFrame = vtkKWFrame::New();
  this->LmkScrollFrame = vtkKWFrameWithScrollbar::New();
  this->SeparatorFrame = vtkKWFrame::New();
  this->TopDragAndDropTarget = vtkKWFrame::New();
  this->BottomDragAndDropTarget= vtkKWFrame::New();
  this->CreateLmkButton = vtkKWPushButton::New();

  this->Menu = vtkKWMenu::New();
  this->MenuFile = vtkKWMenu::New();
  this->MenuEdit = vtkKWMenu::New();
  this->MenuImport = vtkKWMenu::New();
  this->MenuHelp = vtkKWMenu::New();

  this->QuickStartGuideDialog = 0;
  this->QuickStartGuideTxt = 0;
  this->UsersTutorialDialog = 0;
  this->UsersTutorialTxt = 0;

  this->MasterWindow = 0;
  this->Title = NULL;
  this->SetTitle("Lookmark Manager");
}

//----------------------------------------------------------------------------
vtkPVLookmarkManager::~vtkPVLookmarkManager()
{
  this->Checkpoint();

  this->CreateLmkButton->Delete();
  this->CreateLmkButton= NULL;
  this->SeparatorFrame->Delete();
  this->SeparatorFrame= NULL;
  this->TopDragAndDropTarget->Delete();
  this->TopDragAndDropTarget= NULL;
  this->BottomDragAndDropTarget->Delete();
  this->BottomDragAndDropTarget= NULL;

  this->Menu->Delete();
  this->MenuEdit->Delete();
  this->MenuImport->Delete();
  this->MenuFile->Delete();
  this->MenuHelp->Delete();

  if (this->QuickStartGuideTxt)
    {
    this->QuickStartGuideTxt->Delete();
    this->QuickStartGuideTxt = NULL;
    }

  if (this->QuickStartGuideDialog)
    {
    this->QuickStartGuideDialog->Delete();
    this->QuickStartGuideDialog = NULL;
    }

  if (this->UsersTutorialTxt)
    {
    this->UsersTutorialTxt->Delete();
    this->UsersTutorialTxt = NULL;
    }

  if (this->UsersTutorialDialog)
    {
    this->UsersTutorialDialog->Delete();
    this->UsersTutorialDialog = NULL;
    }

  if(this->KWLookmarks)
    {
    vtkVectorIterator<vtkKWLookmark *> *it = this->KWLookmarks->NewIterator();
    while (!it->IsDoneWithTraversal())
      {
      vtkKWLookmark *lmkWidget = 0;
      if (it->GetData(lmkWidget) == VTK_OK && lmkWidget)
        {
//        lmkWidget->GetLookmark()->Delete();
        lmkWidget->Delete();
        }
      it->GoToNextItem();
      }
    it->Delete();
    this->KWLookmarks->Delete();
    }

  if(this->LmkFolderWidgets)
    {
    vtkVectorIterator<vtkKWLookmarkFolder *> *it = this->LmkFolderWidgets->NewIterator();
    while (!it->IsDoneWithTraversal())
      {
      vtkKWLookmarkFolder *lmkFolder = 0;
      if (it->GetData(lmkFolder) == VTK_OK && lmkFolder)
        {
        lmkFolder->Delete();
        }
      it->GoToNextItem();
      }
    it->Delete();
    this->LmkFolderWidgets->Delete();
    }

  if(this->PVLookmarks)
    {
    vtkPVLookmark *lmk = 0;
    vtkVectorIterator<vtkPVLookmark *> *it = this->PVLookmarks->NewIterator();
    while (!it->IsDoneWithTraversal())
      {
      if (it->GetData(lmk) == VTK_OK)
        {
        lmk->Delete();
        }
      it->GoToNextItem();
      }
    it->Delete();
    this->PVLookmarks->Delete();
    this->PVLookmarks = 0;
    }

  this->LmkScrollFrame->Delete();
  this->LmkScrollFrame = NULL;
  this->LmkPanelFrame->Delete();
  this->LmkPanelFrame= NULL;
  
  this->SetTitle(NULL);
  this->SetMasterWindow(0);

}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVLookmarkManager::GetPVRenderView()
{
  return this->GetPVApplication()->GetMainView();
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SetMasterWindow(vtkKWWindow* win)
{

  if (this->MasterWindow != win) 
    { 
    if (this->MasterWindow) 
      { 
      this->MasterWindow->UnRegister(this); 
      }
    this->MasterWindow = win; 
    if (this->MasterWindow) 
      { 
      this->MasterWindow->Register(this); 
      if (this->IsCreated())
        {
        this->Script("wm transient %s %s", this->GetWidgetName(), 
                     this->MasterWindow->GetWidgetName());
        }
      } 
    this->Modified(); 
    }
}



//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Create(vtkKWApplication *app)
{

  if (!this->Superclass::Create(app, NULL, NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->PVApplication = vtkPVApplication::SafeDownCast(this->vtkKWObject::GetApplication());

  const char *wname = this->GetWidgetName();
  if (this->MasterWindow)
    {
    this->Script("toplevel %s -class %s",
                 wname,
                 this->MasterWindow->GetWindowClass());
    }
  else
    {
    this->Script("toplevel %s" ,wname);
    }

  this->Script("wm title %s \"%s\"", wname, this->Title);
  this->Script("wm iconname %s \"vtk\"", wname);
  this->Script("wm geometry %s 380x700+0+0", wname);
  this->Script("wm protocol %s WM_DELETE_WINDOW {wm withdraw %s}",
               wname, wname);

  if (this->MasterWindow)
    {
    this->Script("wm transient %s %s", wname, 
                 this->MasterWindow->GetWidgetName());
    }

  // Set up standard menus

  this->Menu->SetParent(this);
  this->Menu->SetTearOff(0);
  this->Menu->Create(app, "");
  this->Script("%s configure -menu %s", this->GetWidgetName(),
               this->Menu->GetWidgetName());

  // Menu : File

  this->MenuFile->SetParent(this->Menu);
  this->MenuFile->SetTearOff(0);
  this->MenuFile->Create(app, "");

  this->MenuImport->SetParent(this->MenuFile);
  this->MenuImport->SetTearOff(0);
  this->MenuImport->Create(app, "");
  char* rbv = 
    this->MenuImport->CreateRadioButtonVariable(this, "Import");
  this->Script( "set %s 0", rbv );
  this->MenuImport->AddRadioButton(0, "Append", rbv, this, "ImportLookmarksCallback", 0);
  this->MenuImport->AddRadioButton(1, "Replace", rbv, this, "ImportLookmarksCallback", 1);
  delete [] rbv;

  this->Menu->AddCascade("File", this->MenuFile, 0);
  this->MenuFile->AddCascade("Import", this->MenuImport,0);
  this->MenuFile->AddCommand("Save As", this, "SaveLookmarksCallback");
  this->MenuFile->AddCommand("Export Folder", this, "SaveFolderCallback");
  this->MenuFile->AddCommand("Close", this, "CloseCallback");

  // Menu : Edit

  this->MenuEdit->SetParent(this->Menu);
  this->MenuEdit->SetTearOff(0);
  this->MenuEdit->Create(app, "");
  this->Menu->AddCascade("Edit", this->MenuEdit, 0);
  this->MenuEdit->AddCommand("Undo", this, "UndoCallback");
  this->MenuEdit->AddSeparator();
  this->MenuEdit->AddCommand("Create Lookmark", this, "CreateLookmarkCallback");
  this->MenuEdit->AddCommand("Update Lookmark", this, "UpdateLookmarkCallback");
  this->MenuEdit->AddCommand("Rename Lookmark", this, "RenameLookmarkCallback");
  this->MenuEdit->AddSeparator();
  this->MenuEdit->AddCommand("Create Folder", this, "NewFolderCallback");
  this->MenuEdit->AddCommand("Rename Folder", this, "RenameFolderCallback");
  this->MenuEdit->AddSeparator();
  this->MenuEdit->AddCommand("Remove Item(s)", this, "RemoveCallback");
  this->MenuEdit->AddSeparator();
  this->MenuEdit->AddCommand("Select All", this, "AllOnOffCallback 1");
  this->MenuEdit->AddCommand("Clear All", this, "AllOnOffCallback 0");
  this->MenuEdit->SetState("Undo",2);

  // Menu : Help

  this->MenuHelp->SetParent(this->Menu);
  this->MenuHelp->SetTearOff(0);
  this->MenuHelp->Create(app, "");
  this->Menu->AddCascade("Help", this->MenuHelp, 0);
  this->MenuHelp->AddCommand("Quick Start Guide", this, "DisplayQuickStartGuide");
  this->MenuHelp->AddCommand("User's Tutorial", this, "DisplayUsersTutorial");

  this->LmkPanelFrame->SetParent(this);
  this->LmkPanelFrame->Create(this->GetPVApplication(),0);

  this->LmkScrollFrame->SetParent(this->LmkPanelFrame);
  this->LmkScrollFrame->Create(this->GetPVApplication(),0);

  this->SeparatorFrame->SetParent(this->LmkPanelFrame);
  this->SeparatorFrame->Create(this->GetPVApplication(),"-bd 2 -relief groove");

  this->CreateLmkButton->SetParent(this->LmkPanelFrame);
  this->CreateLmkButton->Create(this->GetPVApplication(), "");
  this->CreateLmkButton->SetText("Create Lookmark");
  this->CreateLmkButton->SetCommand(this,"CreateLookmarkCallback");

  this->TopDragAndDropTarget->SetParent(this->LmkScrollFrame->GetFrame());
  this->TopDragAndDropTarget->Create(this->GetPVApplication(),0);

  this->BottomDragAndDropTarget->SetParent(this->LmkScrollFrame->GetFrame());
  this->BottomDragAndDropTarget->Create(this->GetPVApplication(),0);

  this->Script("pack %s -padx 2 -pady 4 -expand t", 
                this->CreateLmkButton->GetWidgetName());
  this->Script("pack %s -ipady 1 -pady 2 -anchor nw -expand t -fill x",
                 this->SeparatorFrame->GetWidgetName());

  this->Script("pack %s -anchor w -fill both -side top",
                 this->TopDragAndDropTarget->GetWidgetName());
  this->Script("%s configure -height 12",
                 this->TopDragAndDropTarget->GetWidgetName());
  this->Script("pack %s -anchor w -fill x -pady 12 -side top",
                 this->LmkScrollFrame->GetWidgetName());
  this->Script("pack %s -anchor n -side top -fill x -expand t",
                this->LmkPanelFrame->GetWidgetName());

  this->Script("set commandList \"\"");

  this->UndoCallback();

  this->Script("wm withdraw %s", wname);
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::CloseCallback()
{
  this->Script("wm withdraw %s", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Display()
{
  this->Script("wm deiconify %s", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DisplayQuickStartGuide()
{
  if (!this->QuickStartGuideDialog)
    {
    this->QuickStartGuideDialog = vtkKWMessageDialog::New();
    }

  if (!this->QuickStartGuideDialog->IsCreated())
    {
    this->QuickStartGuideDialog->SetMasterWindow(this->MasterWindow);
    this->QuickStartGuideDialog->Create(this->GetPVApplication(), "-bd 1 -relief solid");
    }

  this->ConfigureQuickStartGuide();

  this->QuickStartGuideDialog->Invoke();

  this->Script("focus %s",this->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ConfigureQuickStartGuide()
{
  vtkPVApplication *app = this->GetPVApplication();

  if (!this->QuickStartGuideTxt)
    {
    this->QuickStartGuideTxt = vtkKWText::New();
    }

  if (!this->QuickStartGuideTxt->IsCreated())
    {
    this->QuickStartGuideTxt->SetParent(this->QuickStartGuideDialog->GetBottomFrame());
    this->QuickStartGuideTxt->Create(app, "-setgrid true");
    this->QuickStartGuideTxt->SetWidth(60);
    this->QuickStartGuideTxt->SetHeight(20);
    this->QuickStartGuideTxt->SetWrapToWord();
    this->QuickStartGuideTxt->EditableTextOff();
    this->QuickStartGuideTxt->UseVerticalScrollbarOn();
    double r, g, b;
    this->QuickStartGuideTxt->GetTextWidget()->GetParent()
      ->GetBackgroundColor(&r, &g, &b);
    this->QuickStartGuideTxt->GetTextWidget()->SetBackgroundColor(r, g, b);
    }

  this->Script("pack %s -side left -padx 2 -expand 1 -fill both",
                this->QuickStartGuideTxt->GetWidgetName());
  this->Script("pack %s -side bottom",  // -expand 1 -fill both
                this->QuickStartGuideDialog->GetMessageDialogFrame()->GetWidgetName());

  this->QuickStartGuideDialog->SetTitle("Lookmarks Quick-Start Guide");

  ostrstream str;

  str << "A Quick Start Guide for Lookmarks in ParaView" << endl << endl;
  str << "Step 1:" << endl << endl;
  str << "Open your dataset." << endl << endl;
  str << "Step 2:" << endl << endl;
  str << "Visit some feature of interest, set the view parameters as desired." << endl << endl;
  str << "Step 3:" << endl << endl;
  str << "In the Lookmark Manager, press \"Create Lookmark\". Note that a lookmark entry has appeared." << endl << endl;
  str << "Step 4:" << endl << endl;
  str << "Visit some other feature of interest, set the view parameters as desired." << endl << endl;
  str << "Step 5:" << endl << endl;
  str << "In the Lookmark Manager, press \"Create Lookmark\". Note that another lookmark entry has appeared." << endl << endl;
  str << "Step 6:" << endl << endl;
  str << "Click the thumbnail of the first lookmark. Note that ParaView returns to those view parameters and then hands control over to you." << endl << endl;
  str << "Step 7:" << endl << endl;
  str << "Click the thumbnail of the second lookmark. Note the same behavior." << endl << endl;
  str << "Step 8:" << endl << endl;
  str << "Read the User's Tutorial also available in the Help menu and explore the Lookmark Manager interface, to learn how to:" << endl << endl;
  str << "- Organize and edit lookmarks." << endl << endl;
  str << "- Save and import lookmarks to and from disk." << endl << endl;
  str << "- Apply a lookmark to a dataset different from the one with which it was created." << endl << endl;
  str << ends;
  this->QuickStartGuideTxt->SetValue( str.str() );
  str.rdbuf()->freeze(0);
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DisplayUsersTutorial()
{
  if (!this->UsersTutorialDialog)
    {
    this->UsersTutorialDialog = vtkKWMessageDialog::New();
    }

  if (!this->UsersTutorialDialog->IsCreated())
    {
    this->UsersTutorialDialog->SetMasterWindow(this->MasterWindow);
    this->UsersTutorialDialog->Create(this->GetPVApplication(), "-bd 1 -relief solid");
    }

  this->ConfigureUsersTutorial();

  this->UsersTutorialDialog->Invoke();

  this->Script("focus %s",this->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ConfigureUsersTutorial()
{
  vtkPVApplication *app = this->GetPVApplication();

  if (!this->UsersTutorialTxt)
    {
    this->UsersTutorialTxt = vtkKWText::New();
    }
  if (!this->UsersTutorialTxt->IsCreated())
    {
    this->UsersTutorialTxt->SetParent(this->UsersTutorialDialog->GetBottomFrame());
    this->UsersTutorialTxt->Create(app, "-setgrid true");
    this->UsersTutorialTxt->SetWidth(60);
    this->UsersTutorialTxt->SetHeight(20);
    this->UsersTutorialTxt->SetWrapToWord();
    this->UsersTutorialTxt->EditableTextOff();
    this->UsersTutorialTxt->UseVerticalScrollbarOn();
    double r, g, b;
    this->UsersTutorialTxt->GetTextWidget()->GetParent()
      ->GetBackgroundColor(&r, &g, &b);
    this->UsersTutorialTxt->GetTextWidget()->SetBackgroundColor(r, g, b);
    }

  this->Script("pack %s -side left -padx 2 -expand 1 -fill both",
                this->UsersTutorialTxt->GetWidgetName());
  this->Script("pack %s -side bottom",  // -expand 1 -fill both
                this->UsersTutorialDialog->GetMessageDialogFrame()->GetWidgetName());

  this->UsersTutorialDialog->SetTitle("Lookmarks' User's Manual");

  ostrstream str;

  str << "A User's Manual for Lookmarks in ParaView" << endl << endl;
  str << "Introduction:" << endl << endl;
  str << "Lookmarks provide a way to save and manage views of what you consider to be the important regions of your dataset in a fashion analogous to how bookmarks are used by a web browser, all within ParaView. They automate the mundane task of recreating complex filter trees, making it possible to easily toggle back and forth between views. They enable more effective data comparison because they can be applied to different datasets with similar geometry. They can be saved to a single file and then imported in a later ParaView session or shared with co-workers for collaboration purposes. A lookmark is a time-saving tool that automates the recreation of a complex view of data." << endl << endl;
  str << "Feedback:" << endl << endl;
  str << "Lookmarks in ParaView are still in an early stage of development. Any feedback you have would be of great value. To report errors, suggest features or changes, or comment on the functionality, please send an email to sdm-vs@ca.sandia.gov or call Eric Stanton at 505-284-4422." << endl << endl;
  str << "Terminology:" << endl << endl;
  str << "Lookmark Manager - It is here that you import lookmarks into ParaView, toggle from one lookmark to another, save and remove lookmarks as you deem appropriate, and create new lookmarks of the data. In addition, the Lookmark Manager is hierarchical so that you can organize the lookmarks into nested folders." << endl << endl;
  str << "Lookmark Widget - You interact with lookmarks through the lookmark widgets displayed in the Lookmark Manager. Each widget contains a thumbnail preview of the lookmark, a collapsible comments area, its default dataset name, and the name of the lookmark itself. A single-click of the thumbnail generates that lookmark in the ParaView window. A checkbox placed in front of each widget enables single or multiple lookmark selections for removing, renaming, and updating lookmarks. A lookmark widget can also be dragged and dropped to other parts of the Lookmark Manager by grabbing its label." << endl << endl;
  str << "Lookmark File - The contents of the Lookmark Manager or a sub-folder can be saved to a lookmark file, a text-based, XML-structured file that stores the state information of all lookmarks in the Lookmark Manager. This file can be loaded into any ParaView session and is not tied to a particular dataset. It is this file that can easily be shared with co-workers." << endl << endl;
  str << "How To:" << endl << endl;
  str << "Display the Lookmark Manager window - Select \"Window\" >> \"Lookmark Manager\" in the top ParaView menu. The window that appears is detached from the main ParaView window. Note that you can interact with the main ParaView window and the Lookmark Manager window remains in the foreground. The Lookmark Manager window can be closed and reopened without affecting the contents of the Lookmark Manager." << endl << endl;
  str << "Create a new lookmark - Press the \"Create Lookmark\" button or select it in the \"Edit\" menu. Note that the Lookmark Manager will momentarily be moved behind the main ParaView window. This is normal and necessary to generate the thumbnail of the current view. The state of the applicable filters is saved with the lookmark. It is assigned an initial name of the form Lookmark#. A lookmark widget is then appended to the bottom of the Lookmark Manager." << endl << endl;
  str << "View a lookmark - Click on the thumbnail of the lookmark you wish to view. You will then witness the appropriate filters being created in the main ParaView window. Note the lookmark name has been appended to the filter name of each filter belonging to this lookmark.  Clicking this same lookmark again will cause these filters to be deleted if possible (i.e. if they have not been set as inputs to other filters) and the saved filters will be regenerated. See also How to change the dataset to which lookmarks are applied. " << endl << endl;
  str << "Update an existing lookmark - Select the lookmark to be updated and then press \"Edit\" >> \"Update Lookmark\". This stores the state of all filters that contribute to the current view in that lookmark. The lookmark's thumbnail is also replaced to reflect the current view. All other attributes of the lookmark widget (name, comments, etc.) remain unchanged." << endl << endl;
  str << "Save the contents of the Lookmark Manager - Press \"File\" >> \"Save As\". You will be asked to select a new or pre-existing lookmark file (with a .lmk extension) to which to save. All information needed to recreate the current state of the Lookmark Manager is written to this file. This file can be opened and edited in a text editor." << endl << endl;
  str << "Export the contents of a folder - Press \"File\" >> \"Export Folder\". You will be asked to select a new or pre-existing lookmark file. All information needed to recreate the lookmarks and/or folders nested within the selected folder is written to this file." << endl << endl;
  str << "Import a lookmark file - Press \"File\" >> \"Import\" >> and either \"Append\" or \"Replace\" in its cascaded menu. The first will append the contents of the imported lookmark file to the existing contents of the Lookmark Manager. The latter will first remove the contents of the Lookmark Manager and then import the new lookmark file." << endl << endl;
  str << "Automatic saving and loading of the contents of the Lookmark Manager - Any time you modify the Lookmark Manager in some way (create or update a lookmark, move, rename, or remove items, import or export lookmarks), after the modification takes place a lookmark file by the name of \"ParaViewlmk\" is written to the user's home directory containing the state of the Lookmark Manager at that point in time. This file is automatically imported into ParaView at the start of the session. It can be used to recover your lookmarks in the event of a ParaView crash." << endl << endl;
  str << "Using a lookmark on a different dataset - When you create a new lookmark, the current dataset is recorded as that lookmark's default dataset. This means that when you revisit this lookmark (by clicking on its thumbnail) with the \"Lock to Dataset\" option ON, it is this dataset that will be used with the lookmark. However, if the \"Lock to Dataset\" option is turned OFF, the dataset currently being viewed in the render window will be used." << endl << endl;
  str << "Create a folder - Press \"Edit\" >> \"Create Folder\". This appends to the end of the Lookmark Manager an empty folder named \"New Folder\". You can now drag lookmarks into this folder (see How to move lookmarks and/or folders)." << endl << endl;
  str << "Move lookmarks and/or folders - A lookmark or folder can be moved in between any other lookmark or folder in the Lookmark Manager. Simply move your mouse over the name of the item you wish to relocate and hold the left mouse button down. Then drag the widget to the desired location (a box will appear under your mouse if the location is a valid drop point) then release the left mouse button. Releasing over the label of a folder will drop the item in the first nested entry of that folder." << endl << endl;
  str << "Remove lookmarks and/or folders - Select any combination of lookmarks and/or folders and then press \"Edit\" >> \"Remove Item(s)\" button. You will be asked to verify that you wish to delete these. This prompt may be turned off. " << endl << endl;
  str << "Rename a lookmark or folder - Select the lookmark or folder you wish to rename and press \"Edit\" >> \"Rename Lookmark\" or \"Edit\" >> \"Rename Folder\".  This will replace the name with an editable text field containing the old name. Modify the name and press the Enter/Return key. This will remove the text field and replace it with a label containing the new name." << endl << endl;
  str << "Comment on a lookmark - When the contents of the Lookmark Manager are saved to a lookmark file, any text you have typed in the comments area of a lookmark widget will also be saved. By default the comments frame is initially collapsed (see How to expand/collapse lookmarks, folders, and the comments field)." << endl << endl;
  str << "Select lookmarks and/or folders - To select a lookmark or folder to be removed, renamed, or updated (lookmarks only), checkboxes have been placed in front of each item in the Lookmark Manager. Checking a folder will by default check all nested lookmarks and folders as well." << endl << endl;
  str << "Expand/collapse lookmarks, folders, and the comments field - The frames that encapsulate lookmarks, folders, and the comments field can be expanded or collapsed by clicking on the \"x\" or the \"v\", respectively, in the upper right hand corner of the frame. " << endl << endl;
  str << "Undo a change to the Lookmark Manager - Press \"Edit\" >> \"Undo\". This will return the Lookmark Manager's contents to its state before the previous action was performed." << endl << endl;
  str << ends;
  this->UsersTutorialTxt->SetValue( str.str() );
  str.rdbuf()->freeze(0);
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SetButtonFrameState(int state)
{
  this->CreateLmkButton->SetEnabled(state);
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::Checkpoint()
{
  ostrstream str;

  #ifdef _WIN32

  if ( !getenv("HOMEPATH") )
    {
    return;
    }
  str << "C:" << getenv("HOMEPATH") << "\\#ParaViewlmk#" << ends;
  this->SaveLookmarksInternal(str.str());

  #else

  if ( !getenv("HOME") )
    {
    return;
    }
  str << getenv("HOME") << "/.ParaViewlmk" << ends;
  this->SaveLookmarksInternal(str.str());

  #endif
   
  this->MenuEdit->SetState("Undo",0);

}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::UndoCallback()
{
  ostrstream str;

  // Get the path to the checkpointed file

  #ifndef _WIN32

  if ( getenv("HOME") )
    {
    return;
    }
  str << getenv("HOME") << "/.ParaViewlmk" << ends;

  #else

  if ( !getenv("HOMEPATH") )
    {
    return;
    }
  str << "C:" << getenv("HOMEPATH") << "\\#ParaViewlmk#" << ends;

  #endif

  ifstream infile(str.str());

  if ( !infile.fail())
    {
    vtkXMLDataParser *parser;
    vtkXMLDataElement *root;

    //parse the .lmk xml file and get the root node for traversing
    parser = vtkXMLDataParser::New();
    parser->SetStream(&infile);
    parser->Parse();
    root = parser->GetRootElement();

    // Check and then remove all lookmarks and folders currently in display
    this->RemoveCheckedChildren(this->LmkScrollFrame->GetFrame(),1);

    this->ImportLookmarksInternal(0,root,this->LmkScrollFrame->GetFrame());
    
    // after all the widgets are generated, go back thru and add d&d targets for each of them
    this->ResetDragAndDropTargetSetAndCallbacks();

    // this is needed to enable the scrollbar in the lmk mgr
    vtkKWLookmark *lookmarkWidget;
    this->KWLookmarks->GetItem(0,lookmarkWidget);
    if(lookmarkWidget)
      {
      this->GetPVApplication()->GetMainView()->ForceRender();
      lookmarkWidget->GetLmkMainFrame()->PerformShowHideFrame();
      this->GetPVApplication()->GetMainView()->ForceRender();
      lookmarkWidget->GetLmkMainFrame()->PerformShowHideFrame();
      this->GetPVApplication()->GetMainView()->ForceRender();
      }

    infile.close();
    parser->Delete();
    }
  this->MenuEdit->SetState("Undo",2);

}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::AllOnOffCallback(int state)
{
  vtkIdType i,numberOfLookmarkWidgets, numberOfLookmarkFolders;
  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;

  numberOfLookmarkWidgets = this->KWLookmarks->GetNumberOfItems();
  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->KWLookmarks->GetItem(i,lookmarkWidget);
    lookmarkWidget->SetSelectionState(state);
    }
  numberOfLookmarkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numberOfLookmarkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    lmkFolderWidget->SetSelectionState(state);
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ImportLookmarksCallback()
{
  char *filename;
  vtkXMLDataParser *parser;
  vtkXMLDataElement *root;
  vtkKWLookmark *lookmarkWidget;

  this->SetButtonFrameState(0);

  if(!(filename = this->PromptForLookmarkFile(0)))
    {
    this->SetButtonFrameState(1);
    this->Script("pack %s -anchor w -fill both -side top",
                  this->LmkScrollFrame->GetWidgetName());
    this->SetButtonFrameState(1);

    return;
    }

  this->SetButtonFrameState(1);
  
  ifstream infile(filename);

  if ( infile.fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  this->Script("[winfo toplevel %s] config -cursor watch", 
                this->GetWidgetName());

  //parse the .lmk xml file and get the root node for traversing
  parser = vtkXMLDataParser::New();
  parser->SetStream(&infile);
  parser->Parse();
  root = parser->GetRootElement();

  // If "Replace" is selected we remove all preexisting lmks and containers first
  if(this->MenuImport->GetRadioButtonValue(this,"Import") && (this->KWLookmarks->GetNumberOfItems()>0 || this->LmkFolderWidgets->GetNumberOfItems()>0))
    {
    this->RemoveCheckedChildren(this->LmkScrollFrame->GetFrame(),1);

    }

  this->Checkpoint();

  this->ImportLookmarksInternal(this->GetNumberOfChildLmkItems(this->LmkScrollFrame->GetFrame()),root,this->LmkScrollFrame->GetFrame());
  
  // after all the widgets are generated, go back thru and add d&d targets for each of them
  this->ResetDragAndDropTargetSetAndCallbacks();

  // this is needed to enable the scrollbar in the lmk mgr
  this->KWLookmarks->GetItem(0,lookmarkWidget);
  if(lookmarkWidget)
    {
    this->GetPVApplication()->GetMainView()->ForceRender();
    lookmarkWidget->GetLmkMainFrame()->PerformShowHideFrame();
    this->GetPVApplication()->GetMainView()->ForceRender();
    lookmarkWidget->GetLmkMainFrame()->PerformShowHideFrame();
    this->GetPVApplication()->GetMainView()->ForceRender();
    }

  this->Script("[winfo toplevel %s] config -cursor {}", 
                this->GetWidgetName());

  this->Script("%s yview moveto 0",
               this->LmkScrollFrame->GetFrame()->GetParent()->GetWidgetName());

  infile.close();
  parser->Delete();

  return;
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DragAndDropPerformCommand(int x, int y, vtkKWWidget *vtkNotUsed(widget), vtkKWWidget *vtkNotUsed(anchor))
{
  if (  vtkKWTkUtilities::ContainsCoordinates(
        this->GetApplication()->GetMainInterp(),
        this->TopDragAndDropTarget->GetWidgetName(),
        x, y))
    {
    this->Script("%s configure -bd 2 -relief groove", this->TopDragAndDropTarget->GetWidgetName());
    }
  else
    {
    this->Script("%s configure -bd 0 -relief flat", this->TopDragAndDropTarget->GetWidgetName());
    }
}

// attempt at making the after widget lookmark the target instead of the pane its packed in
void vtkPVLookmarkManager::ResetDragAndDropTargetSetAndCallbacks()
{
  vtkIdType numberOfLookmarkWidgets = this->KWLookmarks->GetNumberOfItems();
  vtkIdType numberOfLookmarkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  vtkKWLookmarkFolder *targetLmkFolder;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmark *targetLmkWidget;
  vtkIdType i=0;
  vtkIdType j=0;

  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->KWLookmarks->GetItem(i,lookmarkWidget);
    lookmarkWidget->GetDragAndDropTargetSet()->SetEnable(1);
    // for each lmk container
    for(j=numberOfLookmarkFolders-1;j>=0;j--)
      {
      this->LmkFolderWidgets->GetItem(j,lmkFolderWidget);
      // in this case, we'll have to get the label's grandfather to get the after widget which is the lookmarkFolder widget itself, but now the target area is limited to the label
      if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(lmkFolderWidget->GetSeparatorFrame()))
        {
        lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(lmkFolderWidget->GetSeparatorFrame());
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(lmkFolderWidget->GetSeparatorFrame(), this, "DragAndDropEndCommand");
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(lmkFolderWidget->GetSeparatorFrame(), lmkFolderWidget, "DragAndDropPerformCommand");
//        lookmarkWidget->SetDragAndDropStartCommand(lmkFolderWidget->GetSeparatorFrame(), lmkFolderWidget, "DragAndDropStartCallback");
//        lookmarkWidget->SetDragAndDropStartCommand(lmkFolderWidget->GetSeparatorFrame(), this, "DragAndDropStartCallback");
        }
      if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(lmkFolderWidget->GetNestedSeparatorFrame()))
        {
        lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(lmkFolderWidget->GetNestedSeparatorFrame());
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(lmkFolderWidget->GetNestedSeparatorFrame(), this, "DragAndDropEndCommand");
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(lmkFolderWidget->GetNestedSeparatorFrame(), lmkFolderWidget, "DragAndDropPerformCommand");
//        lookmarkWidget->SetDragAndDropStartCommand(lmkFolderWidget->GetNestedSeparatorFrame(), this, "DragAndDropStartCallback");
        }
      if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(lmkFolderWidget->GetLabelFrame()->GetLabel()))
        {
        lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(lmkFolderWidget->GetLabelFrame()->GetLabel());
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(lmkFolderWidget->GetLabelFrame()->GetLabel(), this, "DragAndDropEndCommand");
        lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(lmkFolderWidget->GetLabelFrame()->GetLabel(), lmkFolderWidget, "DragAndDropPerformCommand");
//        lookmarkWidget->SetDragAndDropStartCommand(lmkFolderWidget->GetLabelFrame()->GetLabel(), lmkFolderWidget, "DragAndDropStartCallback");
        }
      }
    // add lmk widgets as targets to this lookmark as well, they will become the "after widgets"
    for(j=numberOfLookmarkWidgets-1;j>=0;j--)
      {
      this->KWLookmarks->GetItem(j,targetLmkWidget);
      if(targetLmkWidget != lookmarkWidget)
        {
        if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkWidget->GetSeparatorFrame()))
          {
          lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkWidget->GetSeparatorFrame());
          lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkWidget->GetSeparatorFrame(), this, "DragAndDropEndCommand");
          lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkWidget->GetSeparatorFrame(), targetLmkWidget, "DragAndDropPerformCommand");
//          lookmarkWidget->SetDragAndDropStartCommand(targetLmkWidget->GetSeparatorFrame()->GetFrame(), this, "DragAndDropStartCallback");
          }
        }
      }

    // add top frame as target for the widget
    if(!lookmarkWidget->GetDragAndDropTargetSet()->HasTarget(this->TopDragAndDropTarget))
      {
      lookmarkWidget->GetDragAndDropTargetSet()->AddTarget(this->TopDragAndDropTarget);
      lookmarkWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(this->TopDragAndDropTarget, this, "DragAndDropEndCommand");
      lookmarkWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(this->TopDragAndDropTarget, this, "DragAndDropPerformCommand");
//      lookmarkWidget->SetDragAndDropStartCommand(this->TopDragAndDropTarget->GetFrame(), this->TopDragAndDropTarget->GetFrame(), "DragAndDropStartCallback");
      }

    }
  for(i=numberOfLookmarkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    lmkFolderWidget->GetDragAndDropTargetSet()->SetEnable(1);
    // must check to see if the widgets are descendants of this container widget if so dont add as target
    // for each lmk container, add its internal frame as target to this
    for(j=numberOfLookmarkFolders-1;j>=0;j--)
      {
      this->LmkFolderWidgets->GetItem(j,targetLmkFolder);
      if(targetLmkFolder!=lmkFolderWidget && !this->IsWidgetInsideFolder(lmkFolderWidget,targetLmkFolder))
        {
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkFolder->GetSeparatorFrame()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkFolder->GetSeparatorFrame());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkFolder->GetSeparatorFrame(), this, "DragAndDropEndCommand");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkFolder->GetSeparatorFrame(), targetLmkFolder, "DragAndDropPerformCommand");
//          lmkFolderWidget->SetDragAndDropStartCommand(targetLmkFolder->GetSeparatorFrame()->GetFrame(), this, "DragAndDropStartCallback");
          }
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkFolder->GetNestedSeparatorFrame()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkFolder->GetNestedSeparatorFrame());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkFolder->GetNestedSeparatorFrame(), this, "DragAndDropEndCommand");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkFolder->GetNestedSeparatorFrame(), targetLmkFolder, "DragAndDropPerformCommand");
//          lmkFolderWidget->SetDragAndDropStartCommand(targetLmkFolder->GetNestedSeparatorFrame(), this, "DragAndDropStartCallback");
          }
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkFolder->GetLabelFrame()->GetLabel()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkFolder->GetLabelFrame()->GetLabel());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkFolder->GetLabelFrame()->GetLabel(), this, "DragAndDropEndCommand");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkFolder->GetLabelFrame()->GetLabel(), targetLmkFolder, "DragAndDropPerformCommand");
          }
        }
      }
    // add lmk widgets as targets to this lookmark as well, they will become the "after widgets"
    for(j=numberOfLookmarkWidgets-1;j>=0;j--)
      {
      this->KWLookmarks->GetItem(j,targetLmkWidget);
      if(!this->IsWidgetInsideFolder(lmkFolderWidget,targetLmkWidget))
        {
        if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(targetLmkWidget->GetSeparatorFrame()))
          {
          lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(targetLmkWidget->GetSeparatorFrame());
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(targetLmkWidget->GetSeparatorFrame(), targetLmkWidget, "DragAndDropPerformCommand");
//          lmkFolderWidget->SetDragAndDropStartCommand(targetLmkWidget->GetSeparatorFrame()->GetFrame(), this, "DragAndDropStartCallback");
          lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(targetLmkWidget->GetSeparatorFrame(), this, "DragAndDropEndCommand");
          }
        }
      }

    // add top frame as target for the widget
    if(!lmkFolderWidget->GetDragAndDropTargetSet()->HasTarget(this->TopDragAndDropTarget))
      {
      lmkFolderWidget->GetDragAndDropTargetSet()->AddTarget(this->TopDragAndDropTarget);
      lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetEndCommand(this->TopDragAndDropTarget, this, "DragAndDropEndCommand");
      lmkFolderWidget->GetDragAndDropTargetSet()->SetTargetPerformCommand(this->TopDragAndDropTarget, this, "DragAndDropPerformCommand");
      }
    }
}

//---------------------------------------------------------------------------
int vtkPVLookmarkManager::IsWidgetInsideFolder(vtkKWWidget *parent, vtkKWWidget *lmkItem)
{
  int ret = 0;

  if(parent==lmkItem)
    {
    ret=1;
    }
  else
    {
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      if (this->IsWidgetInsideFolder(parent->GetNthChild(i), lmkItem))
        {
        ret = 1;
        break;
        }
      }
    }
  return ret;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DragAndDropEndCommand( int vtkNotUsed(x), int vtkNotUsed(y), vtkKWWidget *widget, vtkKWWidget *vtkNotUsed(anchor), vtkKWWidget *target)
{
  // guaranteed that this will be the only target callback called because the targets in the lookmark manager
  // are mutually exclusive. "target" will either be the vtkKWLabel of the folder widget or the entire lookmarkWidget frame
  // in the case of the lookmark widget. 

  // enhancement: take the x,y, coords, loop through lookmarks and folders, and see which contain them

  vtkKWLookmark *lmkWidget;
  vtkKWLookmarkFolder *lmkFolder;

  // the target will always be a vtkKWFrame but it might be after the lmkitems label frame or nested within the its internal frame if the lmkitem is a lmkcontainer

  if((lmkFolder = vtkKWLookmarkFolder::SafeDownCast(target->GetParent())))
    {
    this->DragAndDropWidget(widget, lmkFolder);
    this->PackChildrenBasedOnLocation(lmkFolder->GetParent());
    lmkFolder->RemoveDragAndDropTargetCues();
    }
  else if((lmkFolder = vtkKWLookmarkFolder::SafeDownCast(target->GetParent()->GetParent()->GetParent()->GetParent()->GetParent())))
    {
    //target is either a folder's nested separator frame or its label, in both cases we drop in the folder's first slot
    this->DragAndDropWidget(widget, lmkFolder->GetNestedSeparatorFrame());
    this->PackChildrenBasedOnLocation(lmkFolder->GetLabelFrame()->GetFrame());
    lmkFolder->RemoveDragAndDropTargetCues();
    }
  else if( (lmkWidget = vtkKWLookmark::SafeDownCast(target->GetParent())))
    {
    this->DragAndDropWidget(widget, lmkWidget);
    this->PackChildrenBasedOnLocation(lmkWidget->GetParent());
    lmkWidget->RemoveDragAndDropTargetCues();

    }
  else if(target==this->TopDragAndDropTarget)
    {
    this->DragAndDropWidget(widget, this->TopDragAndDropTarget);
    this->PackChildrenBasedOnLocation(this->TopDragAndDropTarget->GetParent());
    this->Script("%s configure -bd 0 -relief flat", this->TopDragAndDropTarget->GetWidgetName());
    }

  this->DestroyUnusedLmkWidgets(this->LmkScrollFrame);

  this->ResetDragAndDropTargetSetAndCallbacks();

  // this is needed to enable the scrollbar in the lmk mgr
  this->KWLookmarks->GetItem(0,lmkWidget);
  if(lmkWidget)
    {
    this->GetPVApplication()->GetMainView()->ForceRender();
    lmkWidget->GetLmkMainFrame()->PerformShowHideFrame();
    this->GetPVApplication()->GetMainView()->ForceRender();
    lmkWidget->GetLmkMainFrame()->PerformShowHideFrame();
    this->GetPVApplication()->GetMainView()->ForceRender();
    }
}


//----------------------------------------------------------------------------
int vtkPVLookmarkManager::DragAndDropWidget(vtkKWWidget *widget,vtkKWWidget *AfterWidget)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }

  this->Checkpoint();


  // renumber location vars of siblings widget is leaving
  // handle case when moving within same level
  int oldLoc;
  vtkKWLookmark *lmkWidget;
  vtkKWLookmark *afterLmkWidget;
  vtkKWLookmarkFolder *afterLmkFolder;
  vtkKWLookmarkFolder *lmkFolder;
  vtkPVLookmark *lookmark;
  vtkKWWidget *dstPrnt;
  vtkIdType loc;
  int ret = 0;

  if((lmkWidget = vtkKWLookmark::SafeDownCast(widget)))
    {
    if(!this->KWLookmarks->IsItemPresent(lmkWidget))
      return 0;

    oldLoc = lmkWidget->GetLocation();
    lmkWidget->SetLocation(-1);
    this->DecrementHigherSiblingLmkItemLocationIndices(widget->GetParent(),oldLoc);

    int newLoc;
    if((afterLmkWidget = vtkKWLookmark::SafeDownCast(AfterWidget)))
      {
      newLoc = afterLmkWidget->GetLocation()+1;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }
    else if((afterLmkFolder = vtkKWLookmarkFolder::SafeDownCast(AfterWidget)))
      {
      newLoc = afterLmkFolder->GetLocation()+1;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }
    else
      {
      newLoc = 0;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }

    vtkKWLookmark *newLmkWidget = vtkKWLookmark::New();
    newLmkWidget->SetParent(dstPrnt);
    newLmkWidget->Create(this->GetPVApplication());
    this->Script("pack %s -fill both -expand yes -padx 8",newLmkWidget->GetWidgetName());
    ret = this->KWLookmarks->FindItem(lmkWidget,loc);
    this->PVLookmarks->GetItem(loc,lookmark);
    newLmkWidget->SetLookmarkName(lmkWidget->GetLookmarkName());
    newLmkWidget->SetDataset(lookmark->GetDataset());
    this->SetLookmarkIconCommand(newLmkWidget,loc);
//    newLmkWidget->SetLookmark(lookmark);
    newLmkWidget->SetLocation(newLoc);
    newLmkWidget->SetComments(lmkWidget->GetComments());

    unsigned char *decodedImageData = new unsigned char[9216];
    vtkBase64Utilities *decoder = vtkBase64Utilities::New();
    decoder->Decode((unsigned char*)lookmark->GetImageData(),9216,decodedImageData);
    vtkKWIcon *icon = vtkKWIcon::New();
    icon->SetImage(decodedImageData,48,48,4,9216);
    newLmkWidget->SetLookmarkImage(icon);
    delete [] decodedImageData;
    decoder->Delete();
    icon->Delete();

    this->KWLookmarks->RemoveItem(loc);
    this->KWLookmarks->InsertItem(loc,newLmkWidget);

    this->RemoveItemAsDragAndDropTarget(lmkWidget);
    this->Script("destroy %s", lmkWidget->GetWidgetName());
//    lmkWidget->GetLookmark()->Delete();
    lmkWidget->Delete();
    }
  else if((lmkFolder = vtkKWLookmarkFolder::SafeDownCast(widget)))
    {
    if(!this->LmkFolderWidgets->IsItemPresent(lmkFolder))
      return 0;

    oldLoc = lmkFolder->GetLocation();
    lmkFolder->SetLocation(-1);
    this->DecrementHigherSiblingLmkItemLocationIndices(widget->GetParent(),oldLoc);
    int newLoc;
    if((afterLmkWidget = vtkKWLookmark::SafeDownCast(AfterWidget)))
      {
      newLoc = afterLmkWidget->GetLocation()+1;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }
    else if((afterLmkFolder = vtkKWLookmarkFolder::SafeDownCast(AfterWidget)))
      {
      newLoc = afterLmkFolder->GetLocation()+1;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }
    else
      {
      newLoc = 0;
      this->IncrementHigherSiblingLmkItemLocationIndices(AfterWidget->GetParent(),newLoc);
      dstPrnt = AfterWidget->GetParent();
      }

    vtkKWLookmarkFolder *newLmkFolder = vtkKWLookmarkFolder::New();
    newLmkFolder->SetParent(dstPrnt);
    newLmkFolder->Create(this->GetPVApplication());
    newLmkFolder->SetFolderName(lmkFolder->GetLabelFrame()->GetLabel()->GetText());
    newLmkFolder->SetLocation(newLoc);

    this->Script("pack %s -fill both -expand yes -padx 8",newLmkFolder->GetWidgetName());
    this->LmkFolderWidgets->FindItem(lmkFolder,loc);
    this->LmkFolderWidgets->RemoveItem(loc);
    this->LmkFolderWidgets->InsertItem(loc,newLmkFolder);

    //loop through all children to this container's LabeledFrame

    vtkKWWidget *parent = lmkFolder->GetLabelFrame()->GetFrame();
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->MoveCheckedChildren(
        parent->GetNthChild(i), 
        newLmkFolder->GetLabelFrame()->GetFrame());
      }

// need to delete the source folder
    this->RemoveItemAsDragAndDropTarget(lmkFolder);
    this->Script("destroy %s", lmkFolder->GetWidgetName());
    lmkFolder->Delete();
    }

  return 1;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ImportLookmarksInternal(int locationOfLmkItemAmongSiblings, vtkXMLDataElement *lmkElement, vtkKWWidget *parent)
{
  char *tempname;
  vtkKWLookmark *lookmarkWidget;
  vtkPVLookmark *newLookmark;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType j,numLmks, numFolders;

  if(!strcmp("LmkFolder",lmkElement->GetName()))
    {
    lmkFolderWidget = vtkKWLookmarkFolder::New();
    lmkFolderWidget->SetParent(parent);
 //   lmkFolderWidget->SetParent(this->LmkScrollFrame->GetFrame());
    lmkFolderWidget->Create(this->GetPVApplication());
    this->Script("pack %s -fill both -expand yes -padx 8",lmkFolderWidget->GetWidgetName());

    lmkFolderWidget->SetFolderName(lmkElement->GetAttribute("Name"));
    lmkFolderWidget->SetLocation(locationOfLmkItemAmongSiblings);

    numFolders = this->LmkFolderWidgets->GetNumberOfItems();
    this->LmkFolderWidgets->InsertItem(numFolders,lmkFolderWidget);
    
    // use the label frame of this lmk container as the parent frame in which to pack into (constant)
    // for each xml element (either lookmark or lookmark container) recursively call import with the appropriate location and vtkXMLDataElement
    for(j=0; j<lmkElement->GetNumberOfNestedElements(); j++)
      {
      ImportLookmarksInternal(j,lmkElement->GetNestedElement(j),lmkFolderWidget->GetLabelFrame()->GetFrame());
      }
    }
  else if(!strcmp("LmkFile",lmkElement->GetName()))
    {
    // in this case locationOfLmkItemAmongSiblings is the number of lookmark element currently in the first level of the lookmark manager which is why we start from that index
    // the parent is the lookmark manager's label frame
    for(j=0; j<lmkElement->GetNumberOfNestedElements(); j++)
      {
      ImportLookmarksInternal(j+locationOfLmkItemAmongSiblings,lmkElement->GetNestedElement(j),this->LmkScrollFrame->GetFrame());
      }
    }
  else if(!strcmp("Lmk",lmkElement->GetName()))
    {
    // note that in the case of a lookmark, no recursion is done

    // this uses a vtkXMLLookmarkElement to create a vtkPVLookmark object
    newLookmark = this->GetPVLookmark(lmkElement);
    tempname = newLookmark->GetName();

    // create lookmark widget
    lookmarkWidget = vtkKWLookmark::New();
    lookmarkWidget->SetParent(parent);
 //   lookmarkWidget->SetParent(this->LmkScrollFrame->GetFrame());
    lookmarkWidget->Create(this->GetPVApplication());
    this->Script("pack %s -fill both -expand yes -padx 8",lookmarkWidget->GetWidgetName());

    // initialize lookmark widget attributes
    lookmarkWidget->SetLookmarkName(tempname);
    lookmarkWidget->SetDataset(newLookmark->GetDataset());
    lookmarkWidget->SetLocation(locationOfLmkItemAmongSiblings);
    if(lmkElement->GetAttribute("Comments"))
      lookmarkWidget->SetComments(newLookmark->GetComments());
    numLmks = this->KWLookmarks->GetNumberOfItems();
    this->SetLookmarkIconCommand(lookmarkWidget,numLmks);
//    lookmarkWidget->SetLookmark(newLookmark);

    // convert raw image data stored in file to a thumbnail in the lookmark widget
    unsigned char *decodedImageData = new unsigned char[9216];
    vtkBase64Utilities *decoder = vtkBase64Utilities::New();
    unsigned char *sourceImage = (unsigned char*)lmkElement->GetAttribute("ImageData");
    decoder->Decode(sourceImage,9216,decodedImageData);
    vtkKWIcon *icon = vtkKWIcon::New();
    icon->SetImage(decodedImageData,48,48,4,9216);
    lookmarkWidget->SetLookmarkImage(icon);
    char *lmkImageData = new char[12889];
    strncpy(lmkImageData,lmkElement->GetAttribute("ImageData"),12888);
    lmkImageData[12888] = '\0';
    newLookmark->SetImageData(lmkImageData);
    decoder->Delete();
    icon->Delete();
    delete [] decodedImageData;
    delete [] lmkImageData;

    this->KWLookmarks->InsertItem(numLmks,lookmarkWidget);
    this->PVLookmarks->InsertItem(numLmks,newLookmark);
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SetLookmarkIconCommand(vtkKWLookmark *lmkWidget, vtkIdType index)
{
  ostrstream viewCallback;
  viewCallback << "ViewLookmarkCallback " << index << ends;
  lmkWidget->GetLmkIcon()->UnsetBind("<Button-1>");
  lmkWidget->GetLmkIcon()->UnsetBind("<Double-1>");
  lmkWidget->GetLmkIcon()->SetBind(this, "<Button-1>", viewCallback.str());
  lmkWidget->GetLmkIcon()->SetBind(this, "<Double-1>", viewCallback.str());
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::UnsetLookmarkIconCommand(vtkKWLookmark *lmkWidget)
{
  lmkWidget->GetLmkIcon()->UnsetBind("<Button-1>");
  lmkWidget->GetLmkIcon()->UnsetBind("<Double-1>");
}


//----------------------------------------------------------------------------
char* vtkPVLookmarkManager::PromptForLookmarkFile(int saveFlag)
{
  ostrstream str;
  vtkKWLoadSaveDialog* dialog = vtkKWLoadSaveDialog::New();
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  char *tempName = new char[100]; 

  if(saveFlag)
    dialog->SaveDialogOn();

  dialog->Create(this->GetPVApplication(), 0);

  if (win)
    {
    dialog->SetParent(this->LmkScrollFrame);
    }
  dialog->SetDefaultExtension(".lmk");
  str << "{{} {.lmk} } ";
  str << "{{All files} {*}}" << ends;
  dialog->SetFileTypes(str.str());
  str.rdbuf()->freeze(0);

  if(!dialog->Invoke())
    {
    dialog->Delete();
    return 0;
    }

  this->Script("focus %s",this->GetWidgetName());


  dialog->Delete();
  delete [] tempName;

  return dialog->GetFileName();
  
}

//----------------------------------------------------------------------------
vtkPVLookmark *vtkPVLookmarkManager::GetPVLookmark(vtkXMLDataElement *elem)
{
  vtkPVLookmark *lmk = vtkPVLookmark::New();

  char *lookmarkName = new char[strlen(elem->GetAttribute("Name"))+1]; 
  strcpy(lookmarkName,elem->GetAttribute("Name"));
  lmk->SetName(lookmarkName);
  delete [] lookmarkName;

  if(elem->GetAttribute("Comments"))
    {
    char *lookmarkComments = new char[strlen(elem->GetAttribute("Comments"))+1];
    strcpy(lookmarkComments,elem->GetAttribute("Comments"));
    this->DecodeNewlines(lookmarkComments);
    lmk->SetComments(lookmarkComments);
    delete [] lookmarkComments;
    }
  
  if(elem->GetAttribute("StateScript"))
    {
    char *lookmarkScript = new char[strlen(elem->GetAttribute("StateScript"))+1];
    strcpy(lookmarkScript,elem->GetAttribute("StateScript"));
    this->DecodeNewlines(lookmarkScript);
    lmk->SetStateScript(lookmarkScript);
    delete [] lookmarkScript;
    }

  if(elem->GetAttribute("Dataset"))
    {
    char *lookmarkDataset = new char[strlen(elem->GetAttribute("Dataset"))+1];
    strcpy(lookmarkDataset,elem->GetAttribute("Dataset"));
    lmk->SetDataset(lookmarkDataset);
    delete [] lookmarkDataset;
    }
 
  double centerOfRotation[3];
  elem->GetScalarAttribute("XCenterOfRotation",centerOfRotation[0]);
  elem->GetScalarAttribute("YCenterOfRotation",centerOfRotation[1]);
  elem->GetScalarAttribute("ZCenterOfRotation",centerOfRotation[2]);
  lmk->SetCenterOfRotation(centerOfRotation[0],centerOfRotation[1],centerOfRotation[2]);

  return lmk;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::EncodeNewlines(char *string)
{
  int i;
  int len = strlen(string);
  for(i=0;i<len;i++)
    {
    if(string[i]=='\n')
      {
      string[i]='~';
      }
    } 
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DecodeNewlines(char *string)
{
  int i;
  int len = strlen(string);
  for(i=0;i<len;i++)
    {
    if(string[i]=='~')
      {
      string[i]='\n';
      }
    } 
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::UpdateLookmarkCallback()
{
  vtkPVLookmark *lookmark;
  vtkKWLookmark *lookmarkWidget;
  vtkIdType numLmkWidgets,lmkIndex;
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  int numChecked = 0;

  // called before any change to the lookmark manager

  numLmkWidgets = this->KWLookmarks->GetNumberOfItems();

  for(lmkIndex=0; lmkIndex<numLmkWidgets; lmkIndex++)
    {
    this->KWLookmarks->GetItem(lmkIndex,lookmarkWidget);
    if(lookmarkWidget->GetSelectionState())
      numChecked++;
    }
  if(numChecked==0)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "No Lookmark Selected", 
      "To update a lookmark with a new view, first select only one lookmark by checking its box. Then  go to \"Edit\" --> \"Update Lookmark\".", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }
  else if(numChecked > 1)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "Multiple Lookmarks Selected", 
      "To update a lookmark with a new view, first select only one lookmark by checking its box. Then  go to \"Edit\" --> \"Update Lookmark\".", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  this->Checkpoint();

  // if no lookmarks are selected, display dialog
  // if more than one are selected, ask which one
  for(lmkIndex=0; lmkIndex<numLmkWidgets; lmkIndex++)
    {
    this->PVLookmarks->GetItem(lmkIndex,lookmark);
    this->KWLookmarks->GetItem(lmkIndex,lookmarkWidget);

    if(lookmarkWidget->GetSelectionState())
      {
      this->UpdateLookmarkInternal(lmkIndex);
      lookmarkWidget->SetSelectionState(0);
      break;
      }
    }
}

void vtkPVLookmarkManager::UpdateLookmarkInternal(vtkIdType index)
{
  vtkPVLookmark *lookmark;
  vtkKWLookmark *lookmarkWidget;
  vtkKWIcon *lmkIcon;
  this->PVLookmarks->GetItem(index,lookmark);
  this->KWLookmarks->GetItem(index,lookmarkWidget);
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  //create and store a new session state file
  this->StoreStateScript(lookmark);

  // withdraw the pane so that the lookmark will be added corrrectly
  this->Script("wm withdraw %s", this->GetWidgetName());
  if(win->GetTclInteractor())
    this->Script("wm withdraw %s", win->GetTclInteractor()->GetWidgetName());
  this->Script("focus %s",win->GetWidgetName());
  for(int i=0;i<4;i++)
    {
    this->Script("update");
    this->GetPVRenderView()->ForceRender();
    }

  //update the thumbnail to reflect view in data window
  lmkIcon = this->GetIconOfRenderWindow(this->GetPVRenderView()->GetRenderWindow());
  this->GetPVRenderView()->ForceRender();
  this->Display();
  lookmarkWidget->SetLookmarkImage(lmkIcon);
  lookmark->SetImageData(this->GetEncodedImageData(lmkIcon));
  lmkIcon->Delete();
  lookmark->SetCenterOfRotation(this->PVApplication->GetMainWindow()->GetCenterOfRotationStyle()->GetCenter());
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::CreateLookmarkCallback()
{
  vtkIdType numLmkWidgets = this->KWLookmarks->GetNumberOfItems();
  vtkKWLookmark *lookmarkWidget;
  vtkPVLookmark *newLookmark;
  vtkPVReaderModule *mod;
  vtkPVSource *reader;
  vtkPVSource *temp;
  vtkPVSource *src;
  char *lmkname;
  char *datasetLabel;
  char *datasetName;
  int indexOfNewLmkWidget;
  vtkKWIcon *lmkIcon;
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  // if the pipeline is empty, don't add
  if(this->GetPVApplication()->GetMainWindow()->GetSourceList("Sources")->GetNumberOfItems()==0)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "No Data Loaded", 
      "To create a lookmark you must first open your data and view some feature of interest. Then press \"Create Lookmark\" in either the main window or in the \"Edit\" menu.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

 // this->GetTraceHelper()->AddEntry("$kw(%s) CreateLookmark",
 //                     this->GetTclName());

  // what if the main window is not maximized in screen? 

  //find the reader to use by getting the reader of the current pvsource
  src = win->GetCurrentPVSource();
  while((temp = src->GetPVInput(0)))
    src = temp;
  reader = src;
  if(reader->IsA("vtkPVReaderModule"))
    {
    mod = vtkPVReaderModule::SafeDownCast(reader);
    }
  else
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "Error Creating Lookmark", 
      "Lookmarking ParaView source is not yet supported", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  this->Checkpoint();

  // create and initialize pvlookmark:
  newLookmark = vtkPVLookmark::New();
  lmkname = this->GetUnusedLookmarkName();
  newLookmark->SetName(lmkname);
  newLookmark->SetCenterOfRotation(win->GetCenterOfRotationStyle()->GetCenter());
  datasetLabel = (char *)mod->GetFileEntry()->GetValue();
  datasetName = new char[strlen(datasetLabel)+1];
  strcpy(datasetName,datasetLabel);
  newLookmark->SetDataset(datasetName);
  delete [] datasetName;
  this->StoreStateScript(newLookmark);

  // create and initialize kwlookmark:
  lookmarkWidget = vtkKWLookmark::New();
  // all new lmk widgets get appended to end of lmk mgr thus its parent is the LmkListingFrame:
  lookmarkWidget->SetParent(this->LmkScrollFrame->GetFrame());
  lookmarkWidget->Create(this->GetPVApplication());
  this->Script("pack %s -fill both -expand yes -padx 8",lookmarkWidget->GetWidgetName());
  lookmarkWidget->SetLookmarkName(newLookmark->GetName());
  lookmarkWidget->SetDataset(newLookmark->GetDataset());
  this->SetLookmarkIconCommand(lookmarkWidget,numLmkWidgets);
  // since the direct children of the LmkListingFrame will always be either lmk widgets or containers
  // counting them will give us the appropriate location to assign the new lmk:
  indexOfNewLmkWidget = this->GetNumberOfChildLmkItems(this->LmkScrollFrame->GetFrame());
  lookmarkWidget->SetLocation(indexOfNewLmkWidget);
  // necessary for widget to know about lmk obj for renaming
//  lookmarkWidget->SetLookmark(newLookmark);

  // withdraw the pane so that the lookmark will be added corrrectly
  this->Script("wm withdraw %s", this->GetWidgetName());
  if(win->GetTclInteractor())
    this->Script("wm withdraw %s", win->GetTclInteractor()->GetWidgetName());
  this->Script("focus %s",win->GetWidgetName());
  for(int i=0;i<4;i++)
    {
    this->Script("update");
    this->GetPVRenderView()->ForceRender();
    }

  lmkIcon = this->GetIconOfRenderWindow(this->GetPVRenderView()->GetRenderWindow());
  this->GetPVRenderView()->ForceRender();
  this->Display();
  lookmarkWidget->SetLookmarkImage(lmkIcon);
  newLookmark->SetImageData(this->GetEncodedImageData(lmkIcon));
  lmkIcon->Delete();

  // store objs
  this->KWLookmarks->InsertItem(numLmkWidgets,lookmarkWidget);
  this->PVLookmarks->InsertItem(numLmkWidgets,newLookmark);

  this->ResetDragAndDropTargetSetAndCallbacks();

  this->Script("update");

//   Try to get the scroll bar to initialize properly (show correct portion).
  this->Script("%s yview moveto 1",
               this->LmkScrollFrame->GetFrame()->GetParent()->GetWidgetName());

}

//----------------------------------------------------------------------------
vtkKWIcon *vtkPVLookmarkManager::GetIconOfRenderWindow(vtkRenderWindow *renderWindow)
{

  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(renderWindow);
  w2i->ShouldRerenderOff();
  w2i->Update();

  this->GetPVRenderView()->GetRenderWindow()->SwapBuffersOn();
  this->GetPVRenderView()->GetRenderWindow()->Frame();

  int* dim = w2i->GetOutput()->GetDimensions();
  float width = dim[0];
  float height = dim[1];

  int *extent = w2i->GetOutput()->GetExtent();
  int extentW = extent[1] - extent[0] + 1;
  int extentH = extent[3] - extent[2] + 1;
  float extentN = 0;

  vtkImageClip *iclip = vtkImageClip::New();
  if(width>height)
    {
    int extentD = extentW - extentH;
    extentN = extentH;
    int arg1 = extent[0]+ extentD/2;
    int arg2 = extent[1]-extentD/2;
    iclip->SetOutputWholeExtent(arg1,arg2,extent[2],extent[3],extent[4],extent[5]);
    }
  else if(width<height)
    {
    int extentD = extentH - extentW;
    extentN = extentW;
    int arg1 = extent[2]+extentD/2;
    int arg2 = extent[3]-extentD/2;
    iclip->SetOutputWholeExtent(extent[0],extent[1],arg1,arg2,extent[4],extent[5]);
    }
  else
    {
    extentN = extentW;
    iclip->SetOutputWholeExtent(extent[0],extent[1],extent[2],extent[3],extent[4],extent[5]);
    }
  iclip->SetInput(w2i->GetOutput());
  iclip->Update();

//  int scaledW = width/20;
//  int scaledH = height/20;

  vtkImageResample *resample = vtkImageResample::New();
  resample->SetAxisMagnificationFactor(0,48/extentN);
  resample->SetAxisMagnificationFactor(1,48/extentN);
  resample->SetInput(iclip->GetOutput());
  resample->Update();

  vtkImageData *img_data = resample->GetOutput();
  int *wext = img_data->GetWholeExtent();

  vtkKWIcon* icon = vtkKWIcon::New();
  icon->SetImage(
    static_cast<unsigned char*>(img_data->GetScalarPointer()), 
    wext[1] - wext[0] + 1,
    wext[3] - wext[2] + 1,
    img_data->GetNumberOfScalarComponents(),
    0,
    vtkKWIcon::IMAGE_OPTION_FLIP_V);

  w2i->Delete();
  resample->Delete();
  iclip->Delete();

  return icon;
}

//----------------------------------------------------------------------------
char *vtkPVLookmarkManager::GetEncodedImageData(vtkKWIcon *icon)
{
  const unsigned char *imageData = icon->GetData();
  unsigned char *encodedImageData = new unsigned char[12289];
  vtkBase64Utilities *encoder = vtkBase64Utilities::New();
  unsigned long size = encoder->Encode(imageData,9216,encodedImageData);
  encodedImageData[size] = '\0';
  encoder->Delete();

  return (char *)encodedImageData;
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::StoreStateScript(vtkPVLookmark *lmk)
{
  FILE *lookmarkScript;
  char *buf = new char[300];
  char *stateScript;
  ostrstream state;
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  win->SetSaveVisibleSourcesOnlyFlag(1);
  win->SaveState("tempLookmarkState.pvs");
  win->SetSaveVisibleSourcesOnlyFlag(0);

  //read the session state file in to a new vtkPVLookmark
  if((lookmarkScript = fopen("tempLookmarkState.pvs","r")) != NULL)
    {
    while(fgets(buf,300,lookmarkScript))
      state << buf;
    }
  state << ends;
  fclose(lookmarkScript);
  delete [] buf;
  stateScript = new char[strlen(state.str())+1];
  strcpy(stateScript,state.str());
  lmk->SetStateScript(stateScript);
  delete [] stateScript;

  remove("tempLookmarkState.pvs");
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SaveLookmarksCallback()
{
  char *filename;
  ostrstream str;

//  this->Checkpoint();

  this->SetButtonFrameState(0);

  if(!(filename = this->PromptForLookmarkFile(1)))
    {
    this->SetButtonFrameState(1);
    return;
    }

#ifndef _WIN32
  if ( !getenv("HOME") )
    {
    return;
    }
  str << getenv("HOME") << "/.ParaViewlmk" << ends;
#else
  if ( !getenv("HOMEPATH") )
    {
    return;
    }
  str << "C:" << getenv("HOMEPATH") << "\\#ParaViewlmk#" << ends;
#endif

  if(!strcmp(filename,str.str()))
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Cannot Save to Application Lookmark File", 
      "Please select a different lookmark file to save to. The one you have chosen is restricted for use by the ParaView application.",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }
  this->SaveLookmarksInternal(filename);

  this->SetButtonFrameState(1);
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SaveFolderCallback()
{
  char *filename;
  ostrstream str;
  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType i;
  int numChecked = 0;
  vtkKWLookmarkFolder *rootFolder = NULL;

  int errorFlag = 0;
//  this->Checkpoint();

  vtkIdType numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      numChecked++;
      }
    }

  if(numChecked==0)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "No Folders Selected", 
      "To export a folder of lookmarks to a lookmark file, first select a folder by checking its box. Then go to \"File\" --> \"Export Folder\"",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

  this->SetButtonFrameState(0);

  if(!(filename = this->PromptForLookmarkFile(1)))
    {
    this->SetButtonFrameState(1);
    return;
    }


#ifndef _WIN32
  if ( !getenv("HOME") )
    {
    this->SetButtonFrameState(1);
    return;
    }
  str << getenv("HOME") << "/.ParaViewlmk" << ends;
#else
  if ( !getenv("HOMEPATH") )
    {
    this->SetButtonFrameState(1);
    return;
    }
  str << "C:" << getenv("HOMEPATH") << "\\#ParaViewlmk#" << ends;
#endif

  if(!strcmp(filename,str.str()))
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Cannot Save to Application Lookmark File", 
      "Please select a different lookmark file to save to. The one you have chosen is restricted for use by the ParaView application.",
      vtkKWMessageDialog::ErrorIcon);
    this->SetButtonFrameState(1);
    return;
    }


  // increment thru folders until we find one that is selected
  // then check to see if each subsequent selected folder is a descendant of it
  // if it is not, check to see if the stored folder is a descendant of it
  // if this is not the case either, we have multiple folders selected
  numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      if(rootFolder==NULL)
        {
        rootFolder = lmkFolderWidget;
        }
      else if(this->IsWidgetInsideFolder(lmkFolderWidget,rootFolder))
        {
        errorFlag = 0;
        rootFolder = lmkFolderWidget;
        }
      else if(!this->IsWidgetInsideFolder(rootFolder,lmkFolderWidget) && rootFolder->GetParent()==lmkFolderWidget->GetParent())
        {
        errorFlag = 1;
        }
      else
        {
        errorFlag = 1;
        break;
        }
      }
    }

  if(errorFlag)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Multiple Folders Selected", 
      "To export a folder of lookmarks to a lookmark file, first select a folder by checking its box. Then go to \"File\" --> \"Export Folder\"",
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());
    this->SetButtonFrameState(1);

    return;
    }

  if(rootFolder)
    {
    //make sure all selected lookmarks are inside folder, if so, we can rename this folder
    vtkIdType numLmkWidgets = this->KWLookmarks->GetNumberOfItems();
    for(i=numLmkWidgets-1;i>=0;i--)
      {
      this->KWLookmarks->GetItem(i,lookmarkWidget);
      if(lookmarkWidget->GetSelectionState()==1)
        {
        if(!this->IsWidgetInsideFolder(rootFolder,lookmarkWidget))
          {
          vtkKWMessageDialog::PopupMessage(
            this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Multiple Lookmarks and Folders Selected", 
            "To export a folder of lookmarks to a lookmark file, first select a folder by checking its box. Then go to \"File\" --> \"Export Folder\"",
            vtkKWMessageDialog::ErrorIcon);
          this->Script("focus %s",this->GetWidgetName());
          this->SetButtonFrameState(1);

          return;
          }
        }
      }
    this->SaveFolderInternal(filename,rootFolder);
    }

  this->SetButtonFrameState(1);

  vtkIdType numberOfLookmarkWidgets, numberOfLookmarkFolders;
  numberOfLookmarkWidgets = this->KWLookmarks->GetNumberOfItems();
  for(i=numberOfLookmarkWidgets-1;i>=0;i--)
    {
    this->KWLookmarks->GetItem(i,lookmarkWidget);
    lookmarkWidget->SetSelectionState(0);
    }
  numberOfLookmarkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numberOfLookmarkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    lmkFolderWidget->SetSelectionState(0);
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SaveLookmarksInternal(char *filename)
{
  ifstream *infile;
  ofstream *outfile;
  vtkXMLLookmarkElement *root;
  vtkXMLDataParser *parser;
  
  // write out an empty lookmark file so that the parser will not complain
  outfile = new ofstream(filename,ios::trunc);
  if ( !outfile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  *outfile << "<LmkFile></LmkFile>";
  outfile->close();

  infile = new ifstream(filename);
  if ( !infile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }
  if ( infile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  parser = vtkXMLDataParser::New();
  parser->SetStream(infile);
  parser->Parse();
  root = (vtkXMLLookmarkElement *)parser->GetRootElement();

  this->CreateNestedXMLElements(this->LmkScrollFrame->GetFrame(),root);

  infile->close();
  outfile = new ofstream(filename,ios::trunc);
  if ( !outfile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  root->PrintXML(*outfile,vtkIndent(1));
  outfile->close();
  parser->Delete();

  delete infile;
  delete outfile;
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SaveFolderInternal(char *filename, vtkKWLookmarkFolder *folder)
{
  ifstream *infile;
  ofstream *outfile;
  vtkXMLLookmarkElement *root;
  vtkXMLDataParser *parser;
  
  // write out an empty lookmark file so that the parser will not complain
  outfile = new ofstream(filename,ios::trunc);
  if ( !outfile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  *outfile << "<LmkFile></LmkFile>";
  outfile->close();

  infile = new ifstream(filename);
  if ( !infile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }
  if ( infile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  parser = vtkXMLDataParser::New();
  parser->SetStream(infile);
  parser->Parse();
  root = (vtkXMLLookmarkElement *)parser->GetRootElement();

//  this->CreateNestedXMLElements(folder->GetLabelFrame()->GetFrame()->GetFrame(),root);

  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;

  int nextLmkItemIndex=0;
  int counter=0;

  // loop through the children numberOfChildren times
  // if we come across a lmk item whose packed location among its siblings is the next one we're looking for
  //   recurse and break out of the inner loop, init traversal of children and repeat
  //   
  // the two loops are necessary because the user can change location of lmk items and
  // the vtkKWWidgetCollection of children is ordered by when the item was created, not reordered each time its packed (moved)

  vtkKWWidget *parent = folder->GetLabelFrame()->GetFrame();

  while (counter < parent->GetNumberOfChildren())
    {
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      vtkKWWidget *child = parent->GetNthChild(i);
      if(child->IsA("vtkKWLookmark"))
        {
        lookmarkWidget = vtkKWLookmark::SafeDownCast(child);
        if(this->KWLookmarks->IsItemPresent(lookmarkWidget))
          {
          if(lookmarkWidget->GetLocation()==nextLmkItemIndex)
            {
            this->CreateNestedXMLElements(lookmarkWidget,root);
            nextLmkItemIndex++;
            break;
            }
          }
        }
      else if(child->IsA("vtkKWLookmarkFolder"))
        {
        lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(child);
        if(this->LmkFolderWidgets->IsItemPresent(lmkFolderWidget))
          {
          if(lmkFolderWidget->GetLocation()==nextLmkItemIndex)
            {
            this->CreateNestedXMLElements(lmkFolderWidget,root);
            nextLmkItemIndex++;
            break;
            }
          }
        }
      }
    counter++;
    }

  infile->close();
  outfile = new ofstream(filename,ios::trunc);
  if ( !outfile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  root->PrintXML(*outfile,vtkIndent(1));
  outfile->close();
  parser->Delete();

  delete infile;
  delete outfile;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::SaveLookmarksInternal(ostream *os)
{
  ifstream *infile;
  ofstream *outfile;
  vtkXMLLookmarkElement *root;
  vtkXMLDataParser *parser;

  // write out an empty lookmark file so that the parser will not complain
  outfile = new ofstream("./tempLmkFile",ios::trunc);
  if ( !outfile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }
  if ( outfile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  *outfile << "<LmkFile></LmkFile>";
  outfile->close();

  infile = new ifstream("./tempLmkFile");
  if ( !infile )
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }
  if ( infile->fail())
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Could Not Open Lookmark File", 
      "File might have been moved, deleted, or its permissions changed.", 
      vtkKWMessageDialog::ErrorIcon);
    this->Script("focus %s",this->GetWidgetName());

    return;
    }

  parser = vtkXMLDataParser::New();
  parser->SetStream(infile);
  parser->Parse();
  root = (vtkXMLLookmarkElement *)parser->GetRootElement();

  this->CreateNestedXMLElements(this->LmkScrollFrame->GetFrame(),root);

  infile->close();
  root->PrintXML(*os,vtkIndent(1));
  parser->Delete();

  remove("./tempLmkFile");

  delete infile;
  delete outfile;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::CreateNestedXMLElements(vtkKWWidget *lmkItem, vtkXMLDataElement *dest)
{
//  vtkXMLLookmarkWriter *writer = vtkXMLLookmarkWriter::New();

  if(lmkItem->IsA("vtkKWLookmarkFolder") || lmkItem==this->LmkScrollFrame->GetFrame())
    {
    vtkXMLDataElement *folder = NULL;
    if(lmkItem->IsA("vtkKWLookmarkFolder"))
      {
      vtkKWLookmarkFolder *oldLmkFolder = vtkKWLookmarkFolder::SafeDownCast(lmkItem);
      if(this->LmkFolderWidgets->IsItemPresent(oldLmkFolder))
        {
        folder = vtkXMLDataElement::New();
        folder->SetName("LmkFolder");
        folder->SetAttribute("Name",oldLmkFolder->GetLabelFrame()->GetLabel()->GetText());
        dest->AddNestedElement(folder);

        vtkKWLookmark *lookmarkWidget;
        vtkKWLookmarkFolder *lmkFolderWidget;

        int nextLmkItemIndex=0;
        int counter=0;

        // loop through the children numberOfChildren times
        // if we come across a lmk item whose packed location among its siblings is the next one we're looking for
        //   recurse and break out of the inner loop, init traversal of children and repeat
        //   
        // the two loops are necessary because the user can change location of lmk items and
        // the vtkKWWidgetCollection of children is ordered by when the item was created, not reordered each time its packed (moved)

        vtkKWWidget *parent = 
          oldLmkFolder->GetLabelFrame()->GetFrame();
        
        while (counter < parent->GetNumberOfChildren())
          {
          int nb_children = parent->GetNumberOfChildren();
          for (int i = 0; i < nb_children; i++)
            {
            vtkKWWidget *child = parent->GetNthChild(i);
            if(child->IsA("vtkKWLookmark"))
              {
              lookmarkWidget = vtkKWLookmark::SafeDownCast(child);
              if(this->KWLookmarks->IsItemPresent(lookmarkWidget))
                {
                if(lookmarkWidget->GetLocation()==nextLmkItemIndex)
                  {
                  this->CreateNestedXMLElements(lookmarkWidget,folder);
                  nextLmkItemIndex++;
                  break;
                  }
                }
              }
            else if(child->IsA("vtkKWLookmarkFolder"))
              {
              lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(child);
              if(this->LmkFolderWidgets->IsItemPresent(lmkFolderWidget))
                {
                if(lmkFolderWidget->GetLocation()==nextLmkItemIndex)
                  {
                  this->CreateNestedXMLElements(lmkFolderWidget,folder);
                  nextLmkItemIndex++;
                  break;
                  }
                }
              }
            }
          counter++;
          }
        folder->Delete();
        }
      }
    else if(lmkItem==this->LmkScrollFrame->GetFrame())
      {
      // destination xmldataelement stays the same
      folder = dest;

      vtkKWWidget *parent = lmkItem;
      
      vtkKWLookmark *lookmarkWidget;
      vtkKWLookmarkFolder *lmkFolderWidget;

      int nextLmkItemIndex=0;
      int counter=0;

      // loop through the children numberOfChildren times
      // if we come across a lmk item whose packed location among its siblings is the next one we're looking for
      //   recurse and break out of the inner loop, init traversal of children and repeat
      //   
      // the two loops are necessary because the user can change location of lmk items and
      // the vtkKWWidgetCollection of children is ordered by when the item was created, not reordered each time its packed (moved)

      while (counter < parent->GetNumberOfChildren())
        {
        int nb_children = parent->GetNumberOfChildren();
        for (int i = 0; i < nb_children; i++)
          {
          vtkKWWidget *child = parent->GetNthChild(i);
          if(child->IsA("vtkKWLookmark"))
            {
            lookmarkWidget = vtkKWLookmark::SafeDownCast(child);
            if(this->KWLookmarks->IsItemPresent(lookmarkWidget))
              {
              if(lookmarkWidget->GetLocation()==nextLmkItemIndex)
                {
                this->CreateNestedXMLElements(lookmarkWidget,folder);
                nextLmkItemIndex++;
                break;
                }
              }
            }
          else if(child->IsA("vtkKWLookmarkFolder"))
            {
            lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(child);
            if(this->LmkFolderWidgets->IsItemPresent(lmkFolderWidget))
              {
              if(lmkFolderWidget->GetLocation()==nextLmkItemIndex)
                {
                this->CreateNestedXMLElements(lmkFolderWidget,folder);
                nextLmkItemIndex++;
                break;
                }
              }
            }
          }
        counter++;
        }
      }
    }
  else if(lmkItem->IsA("vtkKWLookmark"))
    {
    vtkIdType lmkIndex;
    vtkKWLookmark *lookmarkWidget = vtkKWLookmark::SafeDownCast(lmkItem);
    vtkPVLookmark *lookmark;

    if(this->KWLookmarks->IsItemPresent(lookmarkWidget))
      {
      this->KWLookmarks->FindItem(lookmarkWidget,lmkIndex);
      this->PVLookmarks->GetItem(lmkIndex,lookmark);
      if(lookmark)
        {
        char* lookmarkComments = new char[strlen(lookmarkWidget->GetComments())+1];
        strcpy(lookmarkComments,lookmarkWidget->GetComments());
        lookmarkComments[strlen(lookmarkWidget->GetComments())] = '\0';
        this->EncodeNewlines(lookmarkComments);
        lookmark->SetComments(lookmarkComments);

        //need to convert newlines in script and image data to encoded character before writing to xml file
        char *stateScript = lookmark->GetStateScript();
        this->EncodeNewlines(stateScript);

        vtkXMLLookmarkElement *elem = vtkXMLLookmarkElement::New();
        elem->SetName("Lmk");
        elem->SetAttribute("Name",lookmarkWidget->GetLookmarkName());
        elem->SetAttribute("Comments", lookmark->GetComments());
        elem->SetAttribute("StateScript", lookmark->GetStateScript());
        elem->SetAttribute("ImageData", lookmark->GetImageData());
        elem->SetAttribute("Dataset", lookmark->GetDataset());
        
        float *temp2;
        temp2 = lookmark->GetCenterOfRotation();
        elem->SetFloatAttribute("XCenterOfRotation", temp2[0]);
        elem->SetFloatAttribute("YCenterOfRotation", temp2[1]);
        elem->SetFloatAttribute("ZCenterOfRotation", temp2[2]);
 
        dest->AddNestedElement(elem);

//        writer->SetObject(lookmark);
//        writer->CreateInElement(dest);

        this->DecodeNewlines(stateScript);
        delete [] lookmarkComments;
        lookmark->SetComments(NULL);

        elem->Delete();
        }
      }
    }
  else
    {
    // if the widget is not a lmk item, recurse with its children widgets but the same destination element as args

    vtkKWWidget *parent = lmkItem;
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      vtkKWWidget *widget = parent->GetNthChild(i);
      this->CreateNestedXMLElements(widget, dest);
      }
    }
//  writer->Delete();
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RenameLookmarkCallback()
{
  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType i;
  int numChecked =0;

  // increment thru folders until we find one that is selected
  // then check to see if each subsequent selected folder is a descendant of it
  // if it is not, check to see if the stored folder is a descendant of it
  // if this is not the case either, we have multiple folders selected
  vtkIdType numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "A Folder is Selected", 
        "To rename a lookmark, select only one by checking its box. Then go to \"Edit\" --> \"Rename Lookmark\".",
        vtkKWMessageDialog::ErrorIcon);
      return;
      }
    }

  // no folders selected
  // only allow one lookmark to be selected now
  vtkKWLookmark *selectedLookmark = NULL;
  vtkIdType numLmkWidgets = this->KWLookmarks->GetNumberOfItems();
  for(i=numLmkWidgets-1;i>=0;i--)
    {
    this->KWLookmarks->GetItem(i,lookmarkWidget);
    if(lookmarkWidget->GetSelectionState()==1)
      {
      selectedLookmark = lookmarkWidget;
      numChecked++;
      if(numChecked>1)
        {
        vtkKWMessageDialog::PopupMessage(
          this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Multiple Lookmarks Selected", 
        "To rename a lookmark, select only one by checking its box. Then go to \"Edit\" --> \"Rename Lookmark\".",
          vtkKWMessageDialog::ErrorIcon);
        return;
        }
      }
    }

  if(selectedLookmark)
    {
    this->Checkpoint();
    selectedLookmark->EditLookmarkCallback();
    selectedLookmark->SetSelectionState(0);
    }

  if(numChecked==0)   // none selected
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "No Lookmarks Selected", 
        "To rename a lookmark, select only one by checking its box. Then go to \"Edit\" --> \"Rename Lookmark\".",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RenameFolderCallback()
{
  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType i;
  vtkKWLookmarkFolder *rootFolder = NULL;
  int errorFlag = 0;

  // increment thru folders until we find one that is selected
  // then check to see if each subsequent selected folder is a descendant of it
  // if it is not, check to see if the stored folder is a descendant of it
  // if this is not the case either, we have multiple folders selected
  vtkIdType numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      if(rootFolder==NULL)
        {
        rootFolder = lmkFolderWidget;
        }
      else if(this->IsWidgetInsideFolder(lmkFolderWidget,rootFolder))
        {
        errorFlag = 0;
        rootFolder = lmkFolderWidget;
        }
      else if(!this->IsWidgetInsideFolder(rootFolder,lmkFolderWidget) && rootFolder->GetParent()==lmkFolderWidget->GetParent())
        {
        errorFlag = 1;
        }
      else
        {
        errorFlag = 1;
        break;
        }
      }
    }

  if(errorFlag)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Multiple Folders Selected", 
      "To rename a folder, select only one by checking its box. Then go to \"Edit\" --> \"Rename Folder\".",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }


  if(rootFolder)
    {
    //make sure all selected lookmarks are inside folder, if so, we can rename this folder
    vtkIdType numLmkWidgets = this->KWLookmarks->GetNumberOfItems();
    for(i=numLmkWidgets-1;i>=0;i--)
      {
      this->KWLookmarks->GetItem(i,lookmarkWidget);
      if(lookmarkWidget->GetSelectionState()==1)
        {
        if(!this->IsWidgetInsideFolder(rootFolder,lookmarkWidget))
          {
          vtkKWMessageDialog::PopupMessage(
            this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "Multiple Lookmarks and Folders Selected", 
            "To rename a folder, select only one by checking its box. Then go to \"Edit\" --> \"Rename Folder\".",
            vtkKWMessageDialog::ErrorIcon);
          return;
          }
        }
      }

    this->Checkpoint();

    rootFolder->EditCallback();
    rootFolder->SetSelectionState(0);
    return;
    }
  else
    {
    // no folders selected
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "No Folders Selected", 
      "To rename a folder, select only one by checking its box. Then go to \"Edit\" --> \"Rename Folder\".",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RemoveCallback()
{
  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkIdType i;
  int numChecked = 0;


  vtkIdType numLmkWidgets = this->KWLookmarks->GetNumberOfItems();
  for(i=numLmkWidgets-1;i>=0;i--)
    {
    this->KWLookmarks->GetItem(i,lookmarkWidget);
    if(lookmarkWidget->GetSelectionState()==1)
      {
      numChecked++;
      }
    }
  vtkIdType numLmkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  for(i=numLmkFolders-1;i>=0;i--)
    {
    this->LmkFolderWidgets->GetItem(i,lmkFolderWidget);
    if(lmkFolderWidget->GetSelectionState()==1)
      {
      numChecked++;
      }
    }

  if(numChecked==0)   // none selected
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "No Lookmarks or Folders Selected", 
      "To remove lookmarks or folders, first select them by checking their boxes. Then go to \"Edit\" --> \"Remove Item(s)\".",
      vtkKWMessageDialog::ErrorIcon);
    return;
    }

  if ( !vtkKWMessageDialog::PopupYesNo(
         this->GetPVApplication(), this->GetPVApplication()->GetMainWindow(), "RemoveItems",
         "Remove Selected Items", 
         "Are you sure you want to remove the selected items from the Lookmark Manager?", 
         vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::RememberYes |
         vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault ))
    {
    return;
    }

  this->Checkpoint();

  this->RemoveCheckedChildren(this->LmkScrollFrame->GetFrame(),0);

  // update the callbacks to the thumbnails
  numLmkWidgets = this->KWLookmarks->GetNumberOfItems();
  for(i=0;i<numLmkWidgets;i++)
    {
    this->KWLookmarks->GetItem(i,lookmarkWidget);
    this->SetLookmarkIconCommand(lookmarkWidget,i);
    }

  this->Script("%s yview moveto 0",
               this->LmkScrollFrame->GetFrame()->GetParent()->GetWidgetName());

}



//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RemoveItemAsDragAndDropTarget(vtkKWWidget *target)
{
  vtkIdType numberOfLookmarkWidgets = this->KWLookmarks->GetNumberOfItems();
  vtkIdType numberOfLookmarkFolders = this->LmkFolderWidgets->GetNumberOfItems();
  vtkKWLookmarkFolder *targetLmkFolder;
  vtkKWLookmarkFolder *lmkFolderWidget;
  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmark *targetLmkWidget;
  vtkIdType j=0;

  for(j=numberOfLookmarkFolders-1;j>=0;j--)
    {
    this->LmkFolderWidgets->GetItem(j,lmkFolderWidget);
    if(target != lmkFolderWidget)
      {
      targetLmkWidget = vtkKWLookmark::SafeDownCast(target);
      if(targetLmkWidget)
        lmkFolderWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkWidget->GetSeparatorFrame());
      targetLmkFolder = vtkKWLookmarkFolder::SafeDownCast(target);
      if(targetLmkFolder)
        {
        lmkFolderWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetSeparatorFrame());
        lmkFolderWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetNestedSeparatorFrame());
        lmkFolderWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetLabelFrame()->GetLabel());
        }
      }
    }

  for(j=numberOfLookmarkWidgets-1;j>=0;j--)
    {
    this->KWLookmarks->GetItem(j,lookmarkWidget);
    if(target != lookmarkWidget)
      {
      targetLmkWidget = vtkKWLookmark::SafeDownCast(target);
      if(targetLmkWidget)
        lookmarkWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkWidget->GetSeparatorFrame());
      targetLmkFolder = vtkKWLookmarkFolder::SafeDownCast(target);
      if(targetLmkFolder)
        {
        lookmarkWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetSeparatorFrame());
        lookmarkWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetNestedSeparatorFrame());
        lookmarkWidget->GetDragAndDropTargetSet()->RemoveTarget(targetLmkFolder->GetLabelFrame()->GetLabel());
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DecrementHigherSiblingLmkItemLocationIndices(vtkKWWidget *parent, int locationOfLmkItemBeingRemoved)
{
  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  int siblingLocation=0;

  int nb_children = parent->GetNumberOfChildren();
  for (int i = 0; i < nb_children; i++)
    {
    vtkKWWidget *sibling = parent->GetNthChild(i);
    if(sibling->IsA("vtkKWLookmark"))
      {
      lookmarkWidget = vtkKWLookmark::SafeDownCast(sibling);
      if(lookmarkWidget)
        {
        siblingLocation = lookmarkWidget->GetLocation();
        if(siblingLocation>locationOfLmkItemBeingRemoved)
          lookmarkWidget->SetLocation(siblingLocation-1);
        }
      }
    else if(sibling->IsA("vtkKWLookmarkFolder"))
      {
      lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(sibling);
      if(lmkFolderWidget)
        {
        siblingLocation = lmkFolderWidget->GetLocation();
        if(siblingLocation > locationOfLmkItemBeingRemoved)
          lmkFolderWidget->SetLocation(siblingLocation-1);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::IncrementHigherSiblingLmkItemLocationIndices(vtkKWWidget *parent, int locationOfLmkItemBeingInserted)
{
  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;
  int siblingLocation=0;

  int nb_children = parent->GetNumberOfChildren();
  for (int i = 0; i < nb_children; i++)
    {
    vtkKWWidget *sibling = parent->GetNthChild(i);
    if(sibling->IsA("vtkKWLookmark"))
      {
      lookmarkWidget = vtkKWLookmark::SafeDownCast(sibling);
      siblingLocation = lookmarkWidget->GetLocation();
      if(siblingLocation>=locationOfLmkItemBeingInserted)
        lookmarkWidget->SetLocation(siblingLocation+1); 
      }
    else if(sibling->IsA("vtkKWLookmarkFolder"))
      {
      lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(sibling);
      siblingLocation = lmkFolderWidget->GetLocation();
      if(siblingLocation>=locationOfLmkItemBeingInserted)
        lmkFolderWidget->SetLocation(siblingLocation+1);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::PackChildrenBasedOnLocation(vtkKWWidget *parent)
{
  parent->UnpackChildren();
  vtkKWLookmark *lookmarkWidget;
  vtkKWLookmarkFolder *lmkFolderWidget;

  // pack the nested frame if inside a folder, otherwise we are in the top level and we need to pack the top frame first
  if((lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(parent->GetParent()->GetParent()->GetParent()->GetParent())))
    {
    this->Script("pack %s -anchor nw -expand t -fill x",
                  lmkFolderWidget->GetNestedSeparatorFrame()->GetWidgetName());
    this->Script("%s configure -height 12",lmkFolderWidget->GetNestedSeparatorFrame()->GetWidgetName()); 
    }
  else
    {
    this->Script("pack %s -anchor w -fill both -side top",
                  this->TopDragAndDropTarget->GetWidgetName());
    this->Script("%s configure -height 12",
                  this->TopDragAndDropTarget->GetWidgetName());
    }

  int nextLmkItemIndex=0;
  int counter=0;

  while (counter < parent->GetNumberOfChildren())
    {
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      vtkKWWidget *child = parent->GetNthChild(i);
      if(child->IsA("vtkKWLookmark"))
        {
        lookmarkWidget = vtkKWLookmark::SafeDownCast(child);
        if(this->KWLookmarks->IsItemPresent(lookmarkWidget))
          {
          if(lookmarkWidget->GetLocation()==nextLmkItemIndex)
            {
            lookmarkWidget->Pack();
            this->Script("pack %s -fill both -expand yes -padx 8",lookmarkWidget->GetWidgetName());
            nextLmkItemIndex++;
            break;
            }
          }
        }
      else if(child->IsA("vtkKWLookmarkFolder"))
        {
        lmkFolderWidget = vtkKWLookmarkFolder::SafeDownCast(child);
        if(this->LmkFolderWidgets->IsItemPresent(lmkFolderWidget))
          {
          if(lmkFolderWidget->GetLocation()==nextLmkItemIndex)
            {
            lmkFolderWidget->Pack();
            this->Script("pack %s -fill both -expand yes -padx 8",lmkFolderWidget->GetWidgetName());
            nextLmkItemIndex++;
            break;
            }
          }
        }
      }
    counter++;
    }
}

//----------------------------------------------------------------------------
char *vtkPVLookmarkManager::GetUnusedLookmarkName()
{
  char *name = new char[50];
  vtkKWLookmark *lmkWidget;
  vtkIdType k,numberOfItems;
  int i=0;
  numberOfItems = this->KWLookmarks->GetNumberOfItems();
  
  while(i<=numberOfItems)
    {
    sprintf(name,"Lookmark%d",i);
    k=0;
    this->KWLookmarks->GetItem(k,lmkWidget);
    while(k<numberOfItems && strcmp(name,lmkWidget->GetLookmarkName()))
      {
      k++;
      this->KWLookmarks->GetItem(k,lmkWidget);
      }
    if(k==numberOfItems)
      break;  //there was not match so this lmkname is valid
    i++;
    }

  return name;
}



//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ViewLookmarkWithCurrentDataset(vtkPVLookmark *oldLookmark)
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  this->Checkpoint();

  // execute state script stored with this lookmark except
  // don't use reader assigned to lookmark, use the current one
  vtkPVSource *src, *temp;
  src = win->GetCurrentPVSource();
  while((temp = src->GetPVInput(0)))
    src = temp;
  if(src->IsA("vtkPVReaderModule"))
    {
    char *temp_script = new char[strlen(oldLookmark->GetStateScript())+1];
    strcpy(temp_script,oldLookmark->GetStateScript());
    // get the camera props to restore after
    vtkCamera *cam = this->GetPVRenderView()->GetRenderer()->GetActiveCamera();
    vtkCamera *camera = cam->NewInstance();
    camera->SetParallelScale(cam->GetParallelScale());
    camera->SetViewAngle(cam->GetViewAngle());
    camera->SetClippingRange(cam->GetClippingRange());
    camera->SetFocalPoint(cam->GetFocalPoint());
    camera->SetPosition(cam->GetPosition());
    camera->SetViewUp(cam->GetViewUp());

    this->ParseAndExecuteStateScript(src,temp_script,oldLookmark,0);
//    this->CreateLookmarkCallback();

    // copy the parameters of the current camera for this class
    // into the active camera on the client and server
    vtkSMProxy* renderModuleProxy = this->GetPVApplication()->
      GetRenderModuleProxy();
    vtkSMDoubleVectorProperty* dvp;
 
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renderModuleProxy->GetProperty("CameraPosition"));
    if (dvp)
      {
      dvp->SetElements(camera->GetPosition());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraPosition.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renderModuleProxy->GetProperty("CameraFocalPoint"));
    if (dvp)
      {
      dvp->SetElements(camera->GetFocalPoint());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraFocalPoint.");
      }
  
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renderModuleProxy->GetProperty("CameraViewUp"));
    if (dvp)
      {
      dvp->SetElements(camera->GetViewUp());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraFocalPoint.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renderModuleProxy->GetProperty("CameraViewAngle"));
    if (dvp)
      {
      dvp->SetElement(0, camera->GetViewAngle());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraViewAngle.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renderModuleProxy->GetProperty("CameraClippingRange"));
    if (dvp)
      {
      dvp->SetElements(camera->GetClippingRange());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraClippingRange.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renderModuleProxy->GetProperty("CameraParallelScale"));
    if (dvp)
      {
      dvp->SetElement(0, camera->GetParallelScale());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraParallelScale.");
      }
    renderModuleProxy->UpdateVTKObjects(); 
    this->GetPVRenderView()->EventuallyRender();
    camera->Delete();
    }

}



//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ViewLookmarkCallback(vtkIdType lmkItemIndex)
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  vtkPVSource *pvs;
  vtkPVLookmark *lookmark;
  this->PVLookmarks->GetItem(lmkItemIndex,lookmark);
  vtkKWLookmark *lookmarkWidget;
  this->KWLookmarks->GetItem(lmkItemIndex,lookmarkWidget);
  vtkPVSource *reader;
  vtkPVSource *currentSource = win->GetCurrentPVSource();


  this->UnsetLookmarkIconCommand(lookmarkWidget);

  //try to delete preexisting filters belonging to this lookmark - for when lmk has been visited before in this session
  lookmark->DeletePVSources();

  // this prevents other filters' visibility from disturbing the lookmark view
  this->TurnFiltersOff();

  if(lookmarkWidget->IsLockedToDataset()==0)
    {
    this->ViewLookmarkWithCurrentDataset(lookmark);
    this->SetLookmarkIconCommand(lookmarkWidget,lmkItemIndex);
    return;
    }


  //  If the lookmark is clicked and this checkbox is checked
  //    and the dataset is currently loaded - same behavior as now
  //    and the dataset is not loaded but exists on disk at the stored path - load the dataset into paraview without prompting the user
  //    and the dataset is not loaded and does not exist on disk at the stored path - prompt the user for dataset; perhaps have an "update dataset path" option
  //    ignores that fact that there might be other datasets loaded in paraview at the moment

  if(!(reader = this->SearchForDefaultDatasetInSourceList(lookmark->GetDataset())))
    {
    FILE *file;
    if( (file = fopen(lookmark->GetDataset(),"r")) != NULL)
      {
      fclose(file);
      //look for dataset at stored path and open automatically if found
      if(win->Open(lookmark->GetDataset()) == VTK_OK)
        {
        reader = win->GetCurrentPVSource();
        reader->AcceptCallback();
        }
      else
        {
        this->SetLookmarkIconCommand(lookmarkWidget,lmkItemIndex);
        return;
        }
      }
    else
      {
      //ask user for dataset
      vtkKWMessageDialog::PopupMessage(
        this->GetPVApplication(), win, "Could Not Find Default Data Set", 
        "You will now be asked to select a different data set.", 
        vtkKWMessageDialog::ErrorIcon);

      win->OpenCallback();
      reader = win->GetCurrentPVSource();
      if(reader==currentSource || !reader->IsA("vtkPVReaderModule"))
        {
        this->SetLookmarkIconCommand(lookmarkWidget,lmkItemIndex);
        return;
        }
      reader->AcceptCallback();

      this->Script("focus %s",this->GetWidgetName());

      vtkPVReaderModule *rm = vtkPVReaderModule::SafeDownCast(reader);
      lookmarkWidget->SetDataset((char *)rm->GetFileEntry()->GetValue());
      lookmark->SetDataset(rm->GetFileEntry()->GetValue());
      }
    }

  // needed? since done in Parse method?
  //set the reader to the current pv source so that the input will automatically be set to the reader
  win->SetCurrentPVSource(reader);

  char *temp_script = new char[strlen(lookmark->GetStateScript())+1];
  strcpy(temp_script,lookmark->GetStateScript());

  this->ParseAndExecuteStateScript(reader,temp_script,lookmark,1);


  // this is needed to update the eyeballs based on recent changes to visibility of filters
  // handle case where there is no source in the source window
  pvs = win->GetCurrentPVSource();
  if(pvs && pvs->GetNotebook())
    this->GetPVRenderView()->UpdateNavigationWindow(pvs,0);

  this->SetLookmarkIconCommand(lookmarkWidget,lmkItemIndex);
  win->SetCenterOfRotation(lookmark->GetCenterOfRotation());

  delete [] temp_script;
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::TurnFiltersOff()
{
  //turn all sources invisible
  vtkPVSource *pvs;
  vtkPVSourceCollection *col = this->GetPVApplication()->GetMainWindow()->GetSourceList("Sources");
  if (col == NULL)
    {
    return;
    }
  vtkCollectionIterator *it = col->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>( it->GetCurrentObject() );
    pvs->SetVisibility(0);
    it->GoToNextItem();
    }
  it->Delete();
}

vtkPVSource *vtkPVLookmarkManager::SearchForDefaultDatasetInSourceList(char *defaultName)
{
  vtkPVSource *pvs;
  vtkPVSource *reader = NULL;
  char *targetName;
  vtkPVReaderModule *mod;

  vtkPVSourceCollection *col = this->GetPVApplication()->GetMainWindow()->GetSourceList("Sources");
  if (col == NULL)
    {
    return NULL;
    }
  vtkCollectionIterator *it = col->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>( it->GetCurrentObject() );
    pvs->SetVisibility(0);
    if(pvs->IsA("vtkPVReaderModule"))
      {
      mod = vtkPVReaderModule::SafeDownCast(pvs);
      targetName = (char *)mod->GetFileEntry()->GetValue();
      if(!strcmp(targetName,defaultName))
        {
        reader = pvs;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  return reader;
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::ParseAndExecuteStateScript(vtkPVSource *reader,char *script, vtkPVLookmark *lmk, int useDatasetFlag)
{
  vtkPVScale *scale;
  vtkPVArraySelection *arraySelection;
  vtkPVLabeledToggle *labeledToggle;
  vtkPVFileEntry *fileEntry;
  vtkPVSelectTimeSet *selectTimeSet;
  vtkPVVectorEntry *vectorEntry;
  vtkPVSelectionList *selectionList;
  vtkPVStringEntry *stringEntry;
  vtkPVSelectWidget *selectWidget;
  vtkPVMinMax *minMaxWidget;
#ifdef PARAVIEW_USE_EXODUS
  vtkPVBasicDSPFilterWidget *dspWidget;
#endif
  vtkPVSource *src;
  vtkPVWidget *pvWidget;
  float tvalue=0;
  char *ptr2;
  char *field;
  double fval,xval,yval,zval; 
  int i=0;
  char *name = new char[50];
  char *data = new char[50];
  char *tok;
  int val;
  char *readername=NULL; 
  char *ptr1;
  char *ptr;
  char cmd[200];

  this->Script("[winfo toplevel %s] config -cursor watch", 
                this->GetWidgetName());

  this->GetPVRenderView()->StartBlockingRender();

  ptr = strtok(script,"\r\n");

  while(ptr)
    {
    if(!readername && !strstr(ptr,"InitializeReadCustom"))
      {
      this->Script(ptr);
      ptr = strtok(NULL,"\r\n");
      }
    else if(strstr(ptr,"InitializeReadCustom"))
      {
      ptr1 = this->GetReaderTclName(ptr);
      readername = new char[25];
      strcpy(readername,ptr1);
      ptr = strtok(NULL,"\r\n");
      }
    else if(strstr(ptr,"GetPVWidget"))
      {
      //loop through collection till found, operate accordingly leaving else statement with ptr one line past 
      vtkCollectionIterator *it = reader->GetWidgets()->NewIterator();
      it->InitTraversal();
 
      for (i = 0; i < reader->GetWidgets()->GetNumberOfItems(); i++)
        {
        pvWidget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
        if(strstr(ptr,pvWidget->GetTraceHelper()->GetObjectName()))
          {
          if((scale = vtkPVScale::SafeDownCast(pvWidget)) && useDatasetFlag==1)
            {
            ptr = strtok(NULL,"\r\n");
            tok = strstr(ptr,"SetValue");
            tok+=9;
            tvalue = atof(tok);
            scale->SetValue(tvalue);
            ptr = strtok(NULL,"\r\n");
            }
          else if((arraySelection = vtkPVArraySelection::SafeDownCast(pvWidget)))
            { 
            ptr = strtok(NULL,"\r\n");
            while(strstr(ptr,"SetArrayStatus"))
              {
              val = this->GetArrayStatus(name,ptr);
              //only turn the variable on, not off, because some other filter might be using the variable
              if(val)
                arraySelection->SetArrayStatus(name,val);
              ptr = strtok(NULL,"\r\n");  
              }
            arraySelection->Accept();
            arraySelection->ModifiedCallback();
            }
          else if((labeledToggle = vtkPVLabeledToggle::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            val = this->GetIntegerScalarWidgetValue(ptr);
            labeledToggle->SetState(val);
//            labeledToggle->Accept();
            labeledToggle->ModifiedCallback();
            ptr = strtok(NULL,"\r\n");
            }
          else if((selectWidget = vtkPVSelectWidget::SafeDownCast(pvWidget)))
            {
            //get the third token of the next line and take off brackets which will give you the value of select widget:
            ptr = strtok(NULL,"\r\n");
            tok = this->GetFieldName(ptr);
            selectWidget->SetCurrentValue(tok); 
            //ignore next line
            ptr = strtok(NULL,"\r\n");
            ptr = strtok(NULL,"\r\n");
            tok = this->GetFieldName(ptr);
            arraySelection = vtkPVArraySelection::SafeDownCast(selectWidget->GetPVWidget(tok));
            ptr = strtok(NULL,"\r\n");
            while(strstr(ptr,"SetArrayStatus"))
              {
              val = this->GetArrayStatus(name,ptr);
              arraySelection->SetArrayStatus(name,val);
              ptr = strtok(NULL,"\r\n"); 
              }
            arraySelection->Accept();
            arraySelection->ModifiedCallback();
            } 
          else if((selectTimeSet = vtkPVSelectTimeSet::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            selectTimeSet->SetTimeValueCallback(this->GetStringEntryValue(ptr));
            selectTimeSet->ModifiedCallback();
            ptr = strtok(NULL,"\r\n");
            }
          else if((vectorEntry = vtkPVVectorEntry::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            vectorEntry->SetValue(this->GetVectorEntryValue(ptr));
            vectorEntry->ModifiedCallback();
            ptr = strtok(NULL,"\r\n");
            }
          else if((fileEntry = vtkPVFileEntry::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            ptr = strtok(NULL,"\r\n");
            }
          else if((selectionList = vtkPVSelectionList::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            val = this->GetIntegerScalarWidgetValue(ptr);
            selectionList->SetCurrentValue(val);
            selectionList->ModifiedCallback();
            ptr = strtok(NULL,"\r\n");
            }
          else if((stringEntry = vtkPVStringEntry::SafeDownCast(pvWidget)))
            {
            //  This widget is used
            ptr = strtok(NULL,"\r\n");
            ptr = strtok(NULL,"\r\n");
            }
          else if((minMaxWidget = vtkPVMinMax::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            minMaxWidget->SetMaxValue(this->GetIntegerScalarWidgetValue(ptr));
            ptr = strtok(NULL,"\r\n");
            minMaxWidget->SetMinValue(this->GetIntegerScalarWidgetValue(ptr));
            ptr = strtok(NULL,"\r\n");
            }
#ifdef PARAVIEW_USE_EXODUS
          else if((dspWidget = vtkPVBasicDSPFilterWidget::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            dspWidget->ChangeDSPFilterMode(this->GetStringValue(ptr));
            ptr = strtok(NULL,"\r\n");
            dspWidget->ChangeCutoffFreq(this->GetStringValue(ptr));
            ptr = strtok(NULL,"\r\n");
            dspWidget->SetFilterLength(atoi(this->GetStringValue(ptr)));
            ptr = strtok(NULL,"\r\n");
            }
#endif
          else   //if we do not support this widget yet, advance and break loop
            {
            ptr = strtok(NULL,"\r\n");
            }
          break;
          }
        it->GoToNextItem();
        }
      //widget in state file is not in widget collection
      if(i==reader->GetWidgets()->GetNumberOfItems())
        {
        ptr = strtok(NULL,"\r\n");
        }
      it->Delete();
      }
    else if(strstr(ptr,"AcceptCallback"))
      {
      //update Display page
      reader->AcceptCallback();

      // next line will either contain "ColorByProperty" "ColorByPointField" or "ColorByCellField"
      vtkPVDisplayGUI *pvData = reader->GetPVOutput(); 
      ptr = strtok(NULL,"\r\n");

      if(strstr(ptr,"ColorByCellField"))
        {
        field = this->GetFieldNameAndValue(ptr,&val);
        //pvData->ColorByCellField(field,val);
        pvData->ColorByArray(field, vtkSMDisplayProxy::CELL_FIELD_DATA);
        }
      else if(strstr(ptr,"ColorByPointField"))
        {
        field = this->GetFieldNameAndValue(ptr,&val);
        //pvData->ColorByPointField(field,val);
        pvData->ColorByArray(field, vtkSMDisplayProxy::POINT_FIELD_DATA);
        
        }
      else if(strstr(ptr,"ColorByProperty"))
        {
        pvData->ColorByProperty();
        }

      ptr = strtok(NULL,"\r\n");

      if(strstr(ptr,"DrawVolume"))
        {
        pvData->DrawVolume();
        ptr = strtok(NULL,"\r\n");
        if(strstr(ptr,"VolumeRenderPointField"))
          {
          field = this->GetFieldNameAndValue(ptr,&val);
          //pvData->VolumeRenderPointField(field,val);
          pvData->VolumeRenderByArray(field, vtkSMDisplayProxy::POINT_FIELD_DATA);
          }
        else if(strstr(ptr,"VolumeRenderCellField"))
          {
          field = this->GetFieldNameAndValue(ptr,&val);
          //pvData->VolumeRenderCellField(field,val);
          pvData->VolumeRenderByArray(field, vtkSMDisplayProxy::CELL_FIELD_DATA);
          }
        ptr = strtok(NULL,"\r\n");
        }
      // this line sets the partdisplay variable
      ptr+=4;
      ptr2 = ptr;
      while(*ptr2!=' ')
        ptr2++;
      *ptr2='\0';
      strcpy(data,ptr);

      vtkSMDisplayProxy *display = reader->GetDisplayProxy();
      ptr = strtok(NULL,"\r\n");
      while(strstr(ptr,data)) 
        {
        if(strstr(ptr,"SetColor"))
          {
          this->GetDoubleVectorWidgetValue(ptr,&xval,&yval,&zval);
          display->SetColorCM(xval,yval,zval);
          }
        else if(strstr(ptr,"SetRepresentation"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetRepresentationCM(val); 
          }
        else if(strstr(ptr,"SetUseImmediateMode"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetImmediateModeRenderingCM(val); 
          }
        else if(strstr(ptr,"SetScalarVisibility"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetScalarVisibilityCM(val); 
          }
        else if(strstr(ptr,"SetDirectColorFlag"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          // when DirectColorFlag = 0,
          // color mode is Default (=1).
          // when DirectColorFlag = 1
          // color mode is MapScalars (=0).
          display->SetColorModeCM(!val); 
          }
        else if(strstr(ptr,"SetInterpolateColorsFlag"))
          {
          // This is "InterpolateColors" while property
          // "InterpolateColorsBeforeMapping". 
          // These are opposite concepts.
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetInterpolateScalarsBeforeMappingCM(val); 
          }
        else if(strstr(ptr,"SetInterpolation"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetInterpolationCM(val); 
          }
        else if(strstr(ptr,"SetPointSize"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetPointSizeCM(val); 
          }
        else if(strstr(ptr,"SetLineWidth"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetLineWidthCM(val); 
          }
        else if(strstr(ptr,"SetOpacity"))
          {
          fval = this->GetDoubleScalarWidgetValue(ptr);
          display->SetOpacityCM(fval); 
          }
        else if(strstr(ptr,"SetTranslate"))
          {
          this->GetDoubleVectorWidgetValue(ptr,&xval,&yval,&zval);
          display->SetPositionCM(xval,yval,zval); 
          }
        else if(strstr(ptr,"SetScale"))
          {
          this->GetDoubleVectorWidgetValue(ptr,&xval,&yval,&zval);
          display->SetScaleCM(xval,yval,zval); 
          }
        else if(strstr(ptr,"SetOrigin"))
          {
          this->GetDoubleVectorWidgetValue(ptr,&xval,&yval,&zval);
          display->SetOriginCM(xval,yval,zval); 
          }
        else if(strstr(ptr,"SetOrientation"))
          {
          this->GetDoubleVectorWidgetValue(ptr,&xval,&yval,&zval);
          display->SetOrientationCM(xval,yval,zval); 
          }   
        ptr = strtok(NULL,"\r\n");
        }
      break;
      }
    else
      {
      ptr = strtok(NULL,"\r\n");
      }
    }

  if(strstr(ptr,readername) && strstr(ptr,"SetCubeAxesVisibility"))
    {
    val = this->GetIntegerScalarWidgetValue(ptr);
    reader->SetCubeAxesVisibility(val);
    ptr = strtok(NULL,"\r\n");
    }
  if(strstr(ptr,readername) && strstr(ptr,"SetPointLabelVisibility"))
    {
    val = this->GetIntegerScalarWidgetValue(ptr);
    reader->SetPointLabelVisibility(val);
    ptr = strtok(NULL,"\r\n");
    }

  reader->GetPVOutput()->Update();

  while(ptr)
    {

    // want to set the reader as the current source before this line executes (this way if the reader is the input source in the script 
    // it will be set as the input of this newly created source by default without explicitly setting it as input - which we can't do since we don't want to use its Tcl name)
    if(strstr(ptr,"CreatePVSource"))
      {
      this->GetPVApplication()->GetMainWindow()->SetCurrentPVSource(reader);
      }

    if(strstr(ptr,"AcceptCallback"))
      {
      // the current source would be the filter just created
      src = this->GetPVApplication()->GetMainWindow()->GetCurrentPVSource();

      // append the lookmark name to its filter name : strip off any characters after '-' because a lookmark name could already have been appended to this filter name
      char *srcLabel = new char[100];
      strcpy(srcLabel,src->GetLabel());
      if(strstr(srcLabel,"-"))
        {
        ptr1 = srcLabel;
        while(*ptr1!='-')
          ptr1++;
        ptr1++;
        *ptr1 = '\0';
        }
      else
        {
        strcat(srcLabel,"-");
        }    
      vtkKWLookmark *lmkWidget;
      vtkIdType k;
      this->PVLookmarks->FindItem(lmk,k);
      this->KWLookmarks->GetItem(k,lmkWidget);
    
      strcat(srcLabel,lmkWidget->GetLookmarkName());
      src->SetLabel(srcLabel);

      //add all pvsources created by this lmk to its collection
      lmk->AddPVSource(src);
      delete [] srcLabel;
      }

    if(strstr(ptr,"SetVisibility") && strstr(ptr,readername))
      {
      val = this->GetIntegerScalarWidgetValue(ptr);
      reader->SetVisibility(val);
      }

    if(strstr(ptr,readername) || (useDatasetFlag==0 && strstr(ptr,"SetCameraState")))
      {
      ptr = strtok(NULL,"\r\n");
      continue;
      }

    // encode special '%' character with a preceeding '%' for printf cmd in call to Script()
    if(strstr(ptr,"%"))
      {
      ptr1 = ptr;
      i = 0;
      while(*ptr1)
        {
        if(*ptr1=='%')
          {
          cmd[i] = *ptr1;
          i++;
          }
        cmd[i] = *ptr1;
        ptr1++;
        i++;
        }      
      cmd[i] = '\0';
      ptr = cmd;
      }

    this->Script(ptr);

  

    ptr = strtok(NULL,"\r\n");
    }

  this->Script("[winfo toplevel %s] config -cursor {}", 
                this->GetWidgetName());

  this->GetPVRenderView()->EndBlockingRender();


  delete [] name;
  delete [] data;
  delete [] readername;
}



//----------------------------------------------------------------------------
char* vtkPVLookmarkManager::GetVectorEntryValue(char *line)
{
  char *tok;
  
  if((tok = strstr(line,"SetValue")))
    {
    tok+=9;
    }
  
  return tok; 
}

//----------------------------------------------------------------------------
char* vtkPVLookmarkManager::GetStringEntryValue(char *line)
{
  char *tok;
  char *ptr;

  
  if((tok = strstr(line,"{")))
    {
    tok++;
    ptr = tok;
    while(*ptr!='}')
      {
      ptr++;
      }
    *ptr = '\0';
    }
  else if((tok = strstr(line,"\"")))
    {
    tok++;
    ptr = tok;
    while(*ptr!='"')
      {
      ptr++;
      }
    *ptr = '\0';
    }
  
  return tok; 
}

//----------------------------------------------------------------------------
char* vtkPVLookmarkManager::GetStringValue(char *line)
{
  char *ptr;
  
  ptr = line;
  ptr+=strlen(line);
  while(*ptr!=' ')
    {
    ptr--;
    }
  ptr++;

  return ptr;
}

//----------------------------------------------------------------------------
int vtkPVLookmarkManager::GetArrayStatus(char *name, char *line)
{
  char *tok;
  char *ptr;

  tok = strstr(line,"{");
  tok++;
  ptr = tok;
  int i=0;
  while(*ptr!='}')
    {
    name[i++] = *ptr;
    ptr++;
    }
  name[i] = '\0';
  ptr+=2;
  
  return atoi(ptr); 
}

//----------------------------------------------------------------------------
int vtkPVLookmarkManager::GetIntegerScalarWidgetValue(char *ptr)
{
  char *ptr2 = ptr;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  return atoi(ptr2);
}

//----------------------------------------------------------------------------
double vtkPVLookmarkManager::GetDoubleScalarWidgetValue(char *ptr)
{
  char *ptr2 = ptr;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  return atof(ptr2);
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::GetDoubleVectorWidgetValue(char *ptr,double *xval,double *yval,double *zval)
{
  char *ptr2 = ptr;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  *xval = atof(ptr2);

  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  *yval = atof(ptr2);

  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  *zval = atof(ptr2);
}

//----------------------------------------------------------------------------
char* vtkPVLookmarkManager::GetReaderTclName(char *ptr)
{
  char *ptr1;
  char *ptr2;
  ptr1 = ptr;
  ptr1+=4;
  ptr2 = ptr1;
  while(*ptr2!=' ')
    ptr2++;
  *ptr2 = '\0';
  return ptr1;
}

//----------------------------------------------------------------------------
char *vtkPVLookmarkManager::GetFieldName(char *ptr)
{
  char *field;
  char *ptr2;
  ptr2 = ptr;
  while(*ptr2!='{')
    ptr2++;
  ptr2++;
  field = ptr2;
  while(*ptr2!='}')
    ptr2++;
  *ptr2 = '\0';
  return field;
}

//----------------------------------------------------------------------------
char *vtkPVLookmarkManager::GetFieldNameAndValue(char *ptr, int *val)
{
  char *field;
  char *ptr2;
  ptr2 = ptr;
  while(*ptr2!='{')
    ptr2++;
  ptr2++;
  field = ptr2;
  while(*ptr2!='}')
    ptr2++;
  *ptr2 = '\0';
  ptr2+=2;
  *val = atoi(ptr2);
  return field;
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::NewFolderCallback()
{
  vtkIdType numFolders, numItems;
  vtkKWLookmarkFolder *lmkFolderWidget;

  this->Checkpoint();

  lmkFolderWidget = vtkKWLookmarkFolder::New();
  // append to the end of the lookmark manager:
  lmkFolderWidget->SetParent(this->LmkScrollFrame->GetFrame());
  lmkFolderWidget->Create(this->GetPVApplication());
  this->Script("pack %s -fill both -expand yes -padx 8",lmkFolderWidget->GetWidgetName());
  this->Script("%s configure -height 8",lmkFolderWidget->GetLabelFrame()->GetFrame()->GetWidgetName());
  lmkFolderWidget->SetFolderName("New Folder");

  // get the location index to assign the folder
  numItems = this->GetNumberOfChildLmkItems(this->LmkScrollFrame->GetFrame());
  lmkFolderWidget->SetLocation(numItems);

  numFolders = this->LmkFolderWidgets->GetNumberOfItems();
  this->LmkFolderWidgets->InsertItem(numFolders,lmkFolderWidget);

  this->ResetDragAndDropTargetSetAndCallbacks();

  this->Script("update");

  // Try to get the scroll bar to initialize properly (show correct portion).
  this->Script("%s yview moveto 1", 
               this->LmkScrollFrame->GetFrame()->GetParent()->GetWidgetName());

}


//----------------------------------------------------------------------------
int vtkPVLookmarkManager::GetNumberOfChildLmkItems(vtkKWWidget *parentFrame)
{
  int location = 0;
  vtkKWLookmark *lmkWidget;
  vtkKWLookmarkFolder *lmkFolder;

  int nb_children = parentFrame->GetNumberOfChildren();
  for (int i = 0; i < nb_children; i++)
    {
    vtkKWWidget *widget = parentFrame->GetNthChild(i);
    if(widget->IsA("vtkKWLookmark"))
      {
      lmkWidget = vtkKWLookmark::SafeDownCast(widget);
      if(this->KWLookmarks->IsItemPresent(lmkWidget))
        location++;
      }
    else if(widget->IsA("vtkKWLookmarkFolder"))
      {
      lmkFolder = vtkKWLookmarkFolder::SafeDownCast(widget);
      if(this->LmkFolderWidgets->IsItemPresent(lmkFolder))
        location++;
      }
    }
  return location;
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::DestroyUnusedLmkWidgets(vtkKWWidget *lmkItem)
{
  if(lmkItem->IsA("vtkKWLookmarkFolder"))
    {
    vtkKWLookmarkFolder *oldLmkFolder = vtkKWLookmarkFolder::SafeDownCast(lmkItem);
    if(!this->LmkFolderWidgets->IsItemPresent(oldLmkFolder))
      {
      oldLmkFolder->RemoveFolder();
      this->Script("destroy %s", oldLmkFolder->GetWidgetName());
      if(oldLmkFolder)
        {
        oldLmkFolder = NULL;
        }
      }
    else
      {
      vtkKWWidget *parent = 
        oldLmkFolder->GetLabelFrame()->GetFrame();
      int nb_children = parent->GetNumberOfChildren();
      for (int i = 0; i < nb_children; i++)
        {
        this->DestroyUnusedLmkWidgets(parent->GetNthChild(i));
        }
      }
    }
  else
    {
    vtkKWWidget *parent = lmkItem;
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->DestroyUnusedLmkWidgets(parent->GetNthChild(i));
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::MoveCheckedChildren(vtkKWWidget *nestedWidget, vtkKWWidget *packingFrame)
{
  vtkIdType loc;

  // Beginning at the Lookmark Manager's internal frame, we are going through each of its nested widgets (nestedWidget)

  if(nestedWidget->IsA("vtkKWLookmarkFolder"))
    {
    vtkKWLookmarkFolder *oldLmkFolder = vtkKWLookmarkFolder::SafeDownCast(nestedWidget);
    if(this->LmkFolderWidgets->IsItemPresent(oldLmkFolder))
      {
      vtkKWLookmarkFolder *newLmkFolder = vtkKWLookmarkFolder::New();
      newLmkFolder->SetParent(packingFrame);
      newLmkFolder->Create(this->GetPVApplication());
      newLmkFolder->SetFolderName(oldLmkFolder->GetLabelFrame()->GetLabel()->GetText());
      newLmkFolder->SetLocation(oldLmkFolder->GetLocation());
      this->Script("pack %s -fill both -expand yes -padx 8",newLmkFolder->GetWidgetName());

      this->LmkFolderWidgets->FindItem(oldLmkFolder,loc);
      this->LmkFolderWidgets->RemoveItem(loc);
      this->LmkFolderWidgets->InsertItem(loc,newLmkFolder);

      //loop through all children to this container's LabeledFrame

      vtkKWWidget *parent = 
        oldLmkFolder->GetLabelFrame()->GetFrame();
      int nb_children = parent->GetNumberOfChildren();
      for (int i = 0; i < nb_children; i++)
        {
        this->MoveCheckedChildren(
          parent->GetNthChild(i),
          newLmkFolder->GetLabelFrame()->GetFrame());
        }
// deleting old folder
      this->RemoveItemAsDragAndDropTarget(oldLmkFolder);
      this->Script("destroy %s", oldLmkFolder->GetWidgetName());
      oldLmkFolder->Delete();
      }
    }
  else if(nestedWidget->IsA("vtkKWLookmark"))
    {
    vtkPVLookmark *lookmark;
    vtkKWLookmark *oldLmkWidget = vtkKWLookmark::SafeDownCast(nestedWidget);
  
    if(this->KWLookmarks->IsItemPresent(oldLmkWidget))
      {
      vtkKWLookmark *newLmkWidget = vtkKWLookmark::New();
      newLmkWidget->SetParent(packingFrame);
      newLmkWidget->Create(this->GetPVApplication());
      this->Script("pack %s -fill both -expand yes -padx 8",newLmkWidget->GetWidgetName());

      this->KWLookmarks->FindItem(oldLmkWidget,loc);
      this->PVLookmarks->GetItem(loc,lookmark);
      newLmkWidget->SetLookmarkName(oldLmkWidget->GetLookmarkName());
      newLmkWidget->SetDataset(lookmark->GetDataset());
      this->SetLookmarkIconCommand(newLmkWidget,loc);
//      newLmkWidget->SetLookmark(lookmark);
      newLmkWidget->SetLocation(oldLmkWidget->GetLocation());
      newLmkWidget->SetComments(oldLmkWidget->GetComments());

      unsigned char *decodedImageData = new unsigned char[9216];
      vtkBase64Utilities *decoder = vtkBase64Utilities::New();
      decoder->Decode((unsigned char*)lookmark->GetImageData(),9216,decodedImageData);
      vtkKWIcon *icon = vtkKWIcon::New();
      icon->SetImage(decodedImageData,48,48,4,9216);
      newLmkWidget->SetLookmarkImage(icon);
      delete [] decodedImageData;
      decoder->Delete();
      icon->Delete();

      this->KWLookmarks->RemoveItem(loc);
      this->KWLookmarks->InsertItem(loc,newLmkWidget);

      this->RemoveItemAsDragAndDropTarget(oldLmkWidget);
      this->Script("destroy %s", oldLmkWidget->GetWidgetName());
//      oldLmkWidget->GetLookmark()->Delete();
      oldLmkWidget->Delete();
      }
    }
  else
    {
    vtkKWWidget *parent = nestedWidget;
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->MoveCheckedChildren(
        parent->GetNthChild(i), packingFrame);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmarkManager::RemoveCheckedChildren(vtkKWWidget *nestedWidget, 
                                                 int forceRemoveFlag)
{
  vtkIdType loc;

  // Beginning at the Lookmark Manager's internal frame, we are going through
  // each of its nested widgets (nestedWidget)

  if(nestedWidget->IsA("vtkKWLookmarkFolder"))
    {
    vtkKWLookmarkFolder *oldLmkFolder = 
      vtkKWLookmarkFolder::SafeDownCast(nestedWidget);
    if(this->LmkFolderWidgets->IsItemPresent(oldLmkFolder))
      {
      if(oldLmkFolder->GetSelectionState() || forceRemoveFlag)
        {
        this->RemoveItemAsDragAndDropTarget(oldLmkFolder);
        this->DecrementHigherSiblingLmkItemLocationIndices(
          oldLmkFolder->GetParent(),oldLmkFolder->GetLocation());
        this->LmkFolderWidgets->FindItem(oldLmkFolder,loc);
        this->LmkFolderWidgets->RemoveItem(loc);

        //loop through all children to this container's LabeledFrame

        vtkKWWidget *parent = 
          oldLmkFolder->GetLabelFrame()->GetFrame();
        int nb_children = parent->GetNumberOfChildren();
        for (int i = 0; i < nb_children; i++)
          {
          this->RemoveCheckedChildren(
            parent->GetNthChild(i), 1);
          }

        this->RemoveItemAsDragAndDropTarget(oldLmkFolder);
        this->Script("destroy %s",oldLmkFolder->GetWidgetName());
        oldLmkFolder->Delete();
        oldLmkFolder = NULL;
        }
      else
        {
        vtkKWWidget *parent = 
          oldLmkFolder->GetLabelFrame()->GetFrame();
        int nb_children = parent->GetNumberOfChildren();
        for (int i = 0; i < nb_children; i++)
          {
          this->RemoveCheckedChildren(
            parent->GetNthChild(i), 0);
          }
        }
      }
    }
  else if(nestedWidget->IsA("vtkKWLookmark"))
    {
    vtkPVLookmark *lookmark;
    vtkKWLookmark *oldLmkWidget = vtkKWLookmark::SafeDownCast(nestedWidget);
  
    if(this->KWLookmarks->IsItemPresent(oldLmkWidget))
      {
      if(oldLmkWidget->GetSelectionState() || forceRemoveFlag)
        {
        this->RemoveItemAsDragAndDropTarget(oldLmkWidget);
        this->DecrementHigherSiblingLmkItemLocationIndices(
          oldLmkWidget->GetParent(),oldLmkWidget->GetLocation());
        this->KWLookmarks->FindItem(oldLmkWidget,loc);
        this->PVLookmarks->GetItem(loc,lookmark);
        this->KWLookmarks->RemoveItem(loc);
        this->PVLookmarks->RemoveItem(loc);
        this->Script("destroy %s", oldLmkWidget->GetWidgetName());
//        oldLmkWidget->GetLookmark()->Delete();
        oldLmkWidget->Delete();
        oldLmkWidget = NULL;
        lookmark->Delete();
        lookmark = NULL; 
        }
      }
    }
  else
    {
    vtkKWWidget *parent = nestedWidget;
    int nb_children = parent->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->RemoveCheckedChildren(
        parent->GetNthChild(i), forceRemoveFlag);
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmarkManager::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

}

 
//----------------------------------------------------------------------------
void vtkPVLookmarkManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

