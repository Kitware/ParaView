/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWindow.cxx
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
#include "vtkKWApplication.h"
#include "vtkPVWindow.h"
#include "vtkKWActorComposite.h"
#include "vtkOutlineFilter.h"
#include "vtkObjectFactory.h"
#include "vtkKWPolyDataCompositeCollection.h"
#include "vtkKWCompositeCollection.h"
#include "vtkPVOpenDialog.h"
#include "vtkKWDialog.h"

//--------------------------------------------------------------------------
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

vtkPVWindow::vtkPVWindow()
{  
  this->CommandFunction = vtkPVWindowCommand;
  this->DataPath = NULL;
  this->PolyDataSets = vtkKWPolyDataCompositeCollection::New();
  this->RetrieveMenu = vtkKWWidget::New();
  this->CreateMenu = vtkKWWidget::New();
}

vtkPVWindow::~vtkPVWindow()
{
  if (this->DataPath)
    {
    this->DataPath->UnRegister(this);
    }
  this->DataPath = NULL;
  this->PolyDataSets->Delete();
}

void vtkPVWindow::Create(vtkKWApplication *app, char *args)
{
  // invoke super method first
  this->vtkKWWindow::Create(app,"");
  this->Script("wm geometry %s 900x700+0+0",
                      this->GetWidgetName());
  
  Tcl_Interp *interp = this->Application->GetMainInterp();

  // create the top level
  this->Script(
    "%s insert 0 command -label {New Window} -command {%s NewWindow}",
    this->GetMenuFile()->GetWidgetName(), this->GetTclName());
  
  this->Script(
    "%s insert 1 command -label {Open} -command {%s Open}",
    this->GetMenuFile()->GetWidgetName(), this->GetTclName());
  this->Script(
    "%s insert 2 command -label {Save} -command {%s Save}",
    this->GetMenuFile()->GetWidgetName(), this->GetTclName());
  
  this->CreateMenu->SetParent(this->GetMenu());
  this->CreateMenu->Create(this->Application,"menu","-tearoff 0");
  this->Script("%s insert 2 cascade -label {Create} -menu %s -underline 0",
               this->Menu->GetWidgetName(),this->CreateMenu->GetWidgetName());

  this->RetrieveMenu->SetParent(this->GetMenu());
  this->RetrieveMenu->Create(this->Application,"menu","-tearoff 0");
  this->Script(
    "%s insert 3 cascade -label {Retrieve} -menu %s -underline 0",
    this->GetMenu()->GetWidgetName(),this->RetrieveMenu->GetWidgetName());

  this->SetStatusText("Version 1.0 beta");
  this->CreateDefaultPropertiesParent();
  
  this->Script( "wm withdraw %s", this->GetWidgetName());

  // create the main view
  // we keep a handle to them as well
  this->MainView = vtkKWRenderView::New();
  this->MainView->SetParent(this->ViewFrame);
  this->MainView->Create(this->Application,"-width 200 -height 200");
  this->AddView(this->MainView);
  // force creation of the properties parent
  this->MainView->GetPropertiesParent();
  this->MainView->MakeSelected();
  this->MainView->ShowViewProperties();
  this->MainView->SetupBindings();
  this->MainView->Delete();
  this->Script( "pack %s -expand yes -fill both", 
                this->MainView->GetWidgetName());

  char cmd[1024];
  sprintf(cmd,"%s Open", this->GetTclName());
  this->AddRecentFilesToMenu(NULL,NULL,cmd);
  
  this->Script( "wm deiconify %s", this->GetWidgetName());
}

void vtkPVWindow::Open(char *name)
{
  vtkPVOpenDialog *dlg = vtkPVOpenDialog::New();
  dlg->Create(this->Application,"");
  
  // open a volume for viewing
  if (dlg->IsFileValid(name))
    {
    dlg->SetFileName(name);
    this->Load(dlg);
    }
  dlg->Delete();
}

void vtkPVWindow::Open()
{
  vtkPVOpenDialog *dlg = vtkPVOpenDialog::New();
  dlg->Create(this->Application,"");

  // open a volume for viewing
  if (dlg->Invoke())
    {
    this->Load(dlg);
    }
  dlg->Delete();
}

void vtkPVWindow::NewWindow()
{
  vtkPVWindow *nw = vtkPVWindow::New();
  nw->Create(this->Application,"");
  this->Application->AddWindow(nw);  
  nw->Delete();
}


void vtkPVWindow::Load(vtkPVOpenDialog *dlg)
{
  // what type of file are we loading, for tfun just load tfun
  char *path = dlg->GetFileName();
 
  // open a session
  //
  if (!strcmp(path + strlen(path) - 4,".svs"))
    {
    this->CloseData();

    // add to list of recently opened files
    char cmd[1024];
    sprintf(cmd,"%s Open",this->GetTclName());
//    this->AddRecentFile(NULL, NULL, dlg->GetFileName(),cmd);
    
    // open the file
    int oldrm = this->MainView->GetRenderMode();
    this->MainView->SetRenderModeToDisabled();
    istream *fptr;
    fptr = new ifstream(path, ios::in);
    this->Serialize(*fptr);
    this->Script("update");
    this->MainView->SetRenderMode(oldrm);
    this->MainView->Render();
    delete fptr;
    }

  // open a data file
  else
    {
    this->CloseData();
    
    // create a new volume
    this->DataPath = dlg;
    dlg->Register(this);
    int lrm = this->MainView->GetRenderMode();
    this->MainView->SetRenderModeToDisabled();
    
    // create the composites and add them in
    //
    this->MainView->AddComposite(dlg->GetComposite());
    dlg->GetComposite()->SetPVWindow(this);
    this->MainView->MakeSelected();
    this->MainView->SetSelectedComposite(dlg->GetComposite());
    dlg->GetComposite()->ShowProperties();
    this->MainView->Reset();
    
    char temp[1024];
    sprintf(temp,"%s: %s",this->Application->GetApplicationName(),
            dlg->GetFileName());
    this->Script("wm title %s {%s}", this->GetWidgetName(),temp);
    
    // add to list of recently opened files
    char cmd[1024];
    sprintf(cmd,"%s Open",this->GetTclName());
    this->AddRecentFile(NULL, dlg->GetFileName(), this, cmd);
    
    this->Script("update");
    this->MainView->SetRenderMode(lrm);
    this->MainView->Render();
    }
}

void vtkPVWindow::Save() 
{
  //char *path;
  //
}

// Description:
// Chaining method to serialize an object and its superclasses.
void vtkPVWindow::SerializeSelf(ostream& os, vtkIndent indent)
{
  // invoke superclass
  this->vtkKWWindow::SerializeSelf(os,indent);

  os << indent << "DataPath ";
  this->DataPath->Serialize(os,indent);
  os << indent << "MainView ";
  this->MainView->Serialize(os,indent);
}

void vtkPVWindow::SerializeToken(istream& is, const char token[1024])
{
  if (!strcmp(token,"DataPath"))
    {
    vtkPVOpenDialog *dlg = vtkPVOpenDialog::New();
    dlg->Create(this->Application,"");
    dlg->Serialize(is);
    this->Load(dlg);
    dlg->Delete();
    return;
    }
  if (!strcmp(token,"MainView"))
    {
    this->MainView->Serialize(is);
    return;
    }

  vtkKWWindow::SerializeToken(is,token);
}

void vtkPVWindow::CloseData()
{
  this->CheckSelectedPolyData();
}

void vtkPVWindow::StorePolyData(vtkKWPolyDataComposite *c)
{
  if (!this->PolyDataSets->IsItemPresent(c))
    {
    this->PolyDataSets->AddItem(c);
    }
  // add it to the retrieve menu
  this->Script(
    "%s add command -label {%s} -command {%s RetrieveDataSet {%s}}",
    this->RetrieveMenu->GetWidgetName(), c->GetName(),
    this->GetTclName(), c->GetName());
}

void vtkPVWindow::RetrieveDataSet(const char *name)
{
  vtkKWPolyDataComposite *c = this->GetPolyDataByName(name);
  if (!c)
    {
    return;
    }
  // make sure we don't accidentally delete the current
  this->CheckSelectedPolyData();
  
  // make this the selected data set
  if (!this->SelectedView->GetComposites()->IsItemPresent(c))
    {
    this->SelectedView->AddComposite(c);
    }
  this->SelectedView->SetSelectedComposite(c);
  c->ShowProperties();
  this->SelectedView->Reset();
}

vtkKWPolyDataComposite *vtkPVWindow::GetPolyDataByName(const char *name)
{
  vtkKWPolyDataComposite *res, *ptr;
  
  res = NULL;
  
  this->PolyDataSets->InitTraversal(); 
  ptr = this->PolyDataSets->GetNextKWPolyDataComposite();
  while (ptr)
    {
    if (!strcmp(ptr->GetName(),name))
      {
      res = ptr;
      }
    ptr = this->PolyDataSets->GetNextKWPolyDataComposite();
    }
  return res;
}


//================================================================
//
// Add sources here
//

void vtkPVWindow::CheckSelectedPolyData()
{
  // get the selected dataset
  vtkKWComposite *com = this->SelectedView->GetSelectedComposite();
  if (!com)
    {
    return;
    }
  
  vtkKWPolyDataComposite *dsc = (vtkKWPolyDataComposite *)com;
  
  // is the selected data set not stored
  if (!this->PolyDataSets->IsItemPresent(dsc))
    {
    // prompt the user if the want to store or replace
    vtkKWDialog *dlg = vtkKWDialog::New();
    vtkKWWidget *ButtonFrame = vtkKWWidget::New();
    ButtonFrame->SetParent(dlg);
    vtkKWWidget *OKButton = vtkKWWidget::New();
    OKButton->SetParent(ButtonFrame);
    vtkKWWidget *CancelButton = vtkKWWidget::New();
    CancelButton->SetParent(ButtonFrame);
    dlg->Create(this->Application,"");
    
    ButtonFrame->Create(this->Application,"frame","");
    OKButton->Create(this->Application,"button","-text Store -width 16");
    OKButton->SetCommand(this, "OK");
    CancelButton->Create(this->Application,"button",
			"-text Discard -width 16");
    CancelButton->SetCommand(this, "Cancel");
    this->Script("pack %s -side left -padx 4 -expand yes",
                 OKButton->GetWidgetName());
    this->Script("pack %s -side left -padx 4 -expand yes",
                 CancelButton->GetWidgetName());
    this->Script("pack %s -side bottom -fill x -pady 4",
                 ButtonFrame->GetWidgetName());    
    
    vtkKWWidget *msg2 = vtkKWWidget::New();
    msg2->SetParent(dlg);
    msg2->Create(this->Application,"label","-text {"
                 "The current dataset has not been stored. You can choose to store it now or discard it.}");
    
    this->Script("pack %s -side bottom -fill x -pady 4",msg2->GetWidgetName());
    
    int result = dlg->Invoke();
    
    if (result)
      {
      this->StorePolyData(dsc);
      }
    
    CancelButton->Delete();
    ButtonFrame->Delete();
    OKButton->Delete();
    msg2->Delete();
    dlg->Delete();
    }
}

