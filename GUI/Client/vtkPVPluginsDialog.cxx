/*=========================================================================

  Module:    vtkPVPluginsDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPluginsDialog.h"

//#include "vtkAssertions.h"

#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWEntryLabeled.h"
#include "vtkObjectFactory.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWPushButton.h"
#include "vtkKWFrame.h"
#include "vtkKWCheckButton.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro( vtkPVPluginsDialog );
vtkCxxRevisionMacro(vtkPVPluginsDialog, "1.6");

int vtkPVPluginsDialogCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPluginsDialog::vtkPVPluginsDialog()
{
  this->CommandFunction = vtkPVPluginsDialogCommand;

  // Build constants widgets
  this->PluginsFrame = vtkKWFrameLabeled::New();
  this->NameButton = vtkKWPushButton::New();
  this->TypeButton = vtkKWPushButton::New();
  this->LoadedButton = vtkKWPushButton::New();
  this->AutoLoadButton = vtkKWPushButton::New();
  this->PathButton = vtkKWPushButton::New();
  this->InfoButton = vtkKWPushButton::New();

  this->ButtonsFrame = vtkKWFrame::New();
  this->AddPluginButton = vtkKWPushButton::New();
  this->HelpButton = vtkKWPushButton::New();
  this->CloseButton = vtkKWPushButton::New();
}

//----------------------------------------------------------------------------
vtkPVPluginsDialog::~vtkPVPluginsDialog()
{
  this->PluginsFrame->Delete();
  this->NameButton->Delete();
  this->TypeButton->Delete();
  this->LoadedButton->Delete();
  this->AutoLoadButton->Delete();
  this->PathButton->Delete();
  this->InfoButton->Delete();
  this->ButtonsFrame->Delete();
  this->AddPluginButton->Delete();
  this->HelpButton->Delete();
  this->CloseButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPluginsDialog::Create(vtkKWApplication *app, const char *args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVPluginsDialog already created");
    return;
    }

  // Invoke super method
  this->Superclass::Create(app, args);

  SetTitle("Kitware Paraview Plugins");

  this->PluginsFrame->SetParent(this);
  this->PluginsFrame->Create(app,"-bd 10");
  this->PluginsFrame->SetLabelText("Plugins");
  this->Script("grid config %s -column 0 -row 0 -columnspan 1 -rowspan 1 -sticky \"nw\"",this->PluginsFrame->GetWidgetName());

  this->NameButton->SetParent(this->PluginsFrame->GetFrame());
  this->NameButton->Create(app,0);
  this->NameButton->SetText("Name");

  this->TypeButton->SetParent(this->PluginsFrame->GetFrame());
  this->TypeButton->Create(app,0);
  this->TypeButton->SetText("Type");
  
  this->LoadedButton->SetParent(this->PluginsFrame->GetFrame());
  this->LoadedButton->Create(app,0);
  this->LoadedButton->SetText("Loaded");

  this->AutoLoadButton->SetParent(this->PluginsFrame->GetFrame());
  this->AutoLoadButton->Create(app,0);
  this->AutoLoadButton->SetText("Auto load");

  this->PathButton->SetParent(this->PluginsFrame->GetFrame());
  this->PathButton->Create(app,0);
  this->PathButton->SetText("Path");

  this->InfoButton->SetParent(this->PluginsFrame->GetFrame());
  this->InfoButton->Create(app,0);
  this->InfoButton->SetText("Information");


  this->Script("grid config %s -column 0 -row 0 -columnspan 1 -rowspan 1 -sticky \"news\"",this->NameButton->GetWidgetName());
  this->Script("grid config %s -column 1 -row 0 -columnspan 1 -rowspan 1 -sticky \"news\"",this->TypeButton->GetWidgetName());
  this->Script("grid config %s -column 2 -row 0 -columnspan 1 -rowspan 1 -sticky \"news\"",this->LoadedButton->GetWidgetName());
  this->Script("grid config %s -column 3 -row 0 -columnspan 1 -rowspan 1 -sticky \"news\"",this->AutoLoadButton->GetWidgetName());
  this->Script("grid config %s -column 4 -row 0 -columnspan 1 -rowspan 1 -sticky \"news\"",this->PathButton->GetWidgetName());
  this->Script("grid config %s -column 5 -row 0 -columnspan 1 -rowspan 1 -sticky \"news\"",this->InfoButton->GetWidgetName());

  this->Script("grid columnconfigure . 0 -weight 1");
  this->Script("grid rowconfigure . 2 -weight 1");

  // bottom buttons: Add a plugin, Help, Close
  this->ButtonsFrame->SetParent(this);
  this->ButtonsFrame->Create(app,"-bd 10");
  this->Script("grid config %s -column 0 -row 1 -columnspan 1 -rowspan 1 -sticky \"news\"",this->ButtonsFrame->GetWidgetName());

  this->AddPluginButton->SetParent(this->ButtonsFrame);
  this->AddPluginButton->Create(app,0);
  this->AddPluginButton->SetText("Add a plug-in");
  this->Script("pack %s -side left -expand true -fill x",this->AddPluginButton->GetWidgetName());

  this->CloseButton->SetParent(this->ButtonsFrame);
  this->CloseButton->Create(app,0);
  this->CloseButton->SetText("Close");
  this->Script("pack %s -side right -expand true -fill x",this->CloseButton->GetWidgetName());
  this->CloseButton->SetCommand(this, "Cancel"); // Cannot be OK: closing the dialog with the window X button returns Cancel.

  this->HelpButton->SetParent(this->ButtonsFrame);
  this->HelpButton->Create(app,0);
  this->HelpButton->SetText("Help");
  this->Script("pack %s -side right -expand true -fill x",this->HelpButton->GetWidgetName());

  // THE FOLLOWING LINES ARE FAKE
#if 0
  const char *FooFilename="libfoo.so";
  const int FooType=1; // 1 means filter
  const int FooIsLoaded=1; // boolean
  const int FooAutoLoad=1; // boolean
  const char *FooPath="/usr/local/paraview/plugins";

  const char *PluginTypeTextFilter="Filter";
  const char *PluginTypeTextDataSetAdaptor="DataSetAdaptor";

  int PluginIndex=1;
  const char *PluginTypeText=0;

  vtkKWLabel *PluginName=vtkKWLabel::New();
  PluginName->SetParent(this->PluginsFrame->GetFrame());
  PluginName->Create(app,0);
  PluginName->SetText(FooFilename);
  this->Script("grid config %s -column 0 -row %d -columnspan 1 -rowspan 1 -sticky \"news\"",PluginName->GetWidgetName(),PluginIndex);
  
  vtkKWLabel *PluginType=vtkKWLabel::New();
  PluginType->SetParent(this->PluginsFrame->GetFrame());
  PluginType->Create(app,0);
  switch(FooType)
    {
    case 1:
      PluginTypeText=PluginTypeTextFilter;
      break;
    case 2:
      PluginTypeText=PluginTypeTextDataSetAdaptor;
      break;
    default:
      ;// error
    }
  PluginType->SetText(PluginTypeText);
  this->Script("grid config %s -column 1 -row %d -columnspan 1 -rowspan 1 -sticky \"news\"",PluginType->GetWidgetName(),PluginIndex);

  vtkKWCheckButton *PluginIsLoaded=vtkKWCheckButton::New();
  PluginIsLoaded->SetParent(this->PluginsFrame->GetFrame());
  PluginIsLoaded->Create(app,0);
  PluginIsLoaded->SetState(FooIsLoaded);
  this->Script("grid config %s -column 2 -row %d -columnspan 1 -rowspan 1 -sticky \"news\"",PluginIsLoaded->GetWidgetName(),PluginIndex);

  vtkKWCheckButton *PluginAutoLoad=vtkKWCheckButton::New();
  PluginAutoLoad->SetParent(this->PluginsFrame->GetFrame());
  PluginAutoLoad->Create(app,0);
  PluginAutoLoad->SetState(FooAutoLoad);
  this->Script("grid config %s -column 3 -row %d -columnspan 1 -rowspan 1 -sticky \"news\"",PluginAutoLoad->GetWidgetName(),PluginIndex);
  
  vtkKWLabel *PluginPath=vtkKWLabel::New();
  PluginPath->SetParent(this->PluginsFrame->GetFrame());
  PluginPath->Create(app,0);
  PluginPath->SetText(FooPath);
  this->Script("grid config %s -column 4 -row %d -columnspan 1 -rowspan 1 -sticky \"news\"",PluginPath->GetWidgetName(),PluginIndex);

  vtkKWPushButton *PluginInfo=vtkKWPushButton::New();
  PluginInfo->SetParent(this->PluginsFrame->GetFrame());
  PluginInfo->Create(app,"-bitmap info -foreground #0a0");
  this->Script("grid config %s -column 5 -row %d -columnspan 1 -rowspan 1 -sticky \"news\"",PluginInfo->GetWidgetName(),PluginIndex);
#endif
}

//----------------------------------------------------------------------------
int vtkPVPluginsDialog::Invoke()
{
  int result=0;

  if (this->IsCreated()) // should be require("is_created",this->IsCreated());
    {
      // Display one row per plugin
#if 0
      if(cached_plugins==0)
        {
          cached_plugins=new Cached_plugin;
        }
      it=cached_plugins->iterator();
      it->start();
      while(!it->is_off())
        {
          it->item();
          it->forth();
        }
#endif
    }
  result=this->Superclass::Invoke();

  //  vtkEnsure("valid_result",(result==0)&&(result!=1));

  return result;
}

//----------------------------------------------------------------------------
void vtkPVPluginsDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PluginsFrame: " << this->PluginsFrame << endl;
  os << indent << "NameButton: " << this->NameButton << endl;
  os << indent << "TypeButton: " << this->TypeButton << endl;
  os << indent << "LoadedButton: " << this->LoadedButton << endl;
  os << indent << "AutoLoadButton: " << this->AutoLoadButton << endl;
  os << indent << "PathButton: " << this->PathButton << endl;
  os << indent << "InfoButton: " << this->InfoButton<< endl;
  os << indent << "ButtonsFrame: " << this->ButtonsFrame << endl;
  os << indent << "AddPluginButton: " << this->AddPluginButton << endl;
  os << indent << "HelpButton: " << this->HelpButton << endl;
  os << indent << "CloseButton: " << this->CloseButton<< endl;
}
