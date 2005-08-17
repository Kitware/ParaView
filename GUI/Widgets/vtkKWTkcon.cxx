/*=========================================================================

  Module:    vtkKWTkcon.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWTkcon.h"

#include "vtkKWApplication.h"
#include "vtkKWText.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"

#include "Utilities/tkcon/vtkKWTkconInit.h"

#include <vtksys/stl/string>

//-------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWTkcon );
vtkCxxRevisionMacro(vtkKWTkcon, "1.1");

//----------------------------------------------------------------------------
class vtkKWTkconInternals
{
public:

  vtkKWMenu *MainMenu;
  vtkKWMenu *PopupMenu;
  vtkKWText *Console;
};

//----------------------------------------------------------------------------
vtkKWTkcon::vtkKWTkcon()
{
  this->Internals = new vtkKWTkconInternals;

  this->Internals->MainMenu = NULL;
  this->Internals->PopupMenu = NULL;
  this->Internals->Console = NULL;
}

//----------------------------------------------------------------------------
vtkKWTkcon::~vtkKWTkcon()
{
  if (this->Internals)
    {
    if (this->Internals->MainMenu)
      {
      this->Internals->MainMenu->Delete();
      this->Internals->MainMenu = NULL;
      }
    if (this->Internals->PopupMenu)
      {
      this->Internals->PopupMenu->Delete();
      this->Internals->PopupMenu = NULL;
      }
    if (this->Internals->Console)
      {
      this->Internals->Console->Delete();
      this->Internals->Console = NULL;
      }
    delete this->Internals;
    }
}

//----------------------------------------------------------------------------
void vtkKWTkcon::Create(vtkKWApplication *app)
{
  if (vtkKWTkconInit::GetInitialized())
    {
    vtkErrorMacro(<< this->GetClassName() 
                  << " sorry, only one tkcon instance can be created in the "
                  "current implementation");
    return;
    }

  vtkKWTkconInit::Initialize(app ? app->GetMainInterp() : NULL);

  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget
  // We need to bypass the vtkKWTopLevel::Create to allow tkcon to
  // create its own toplevel

  this->vtkKWCoreWidget::Create(app);

  ostrstream tk_cmd;

  // http://wiki.tcl.tk/1878

  tk_cmd << "namespace eval tkcon {}" << endl
         << "set tkcon::PRIV(showOnStartup) 0" << endl
         << "set tkcon::PRIV(root) " << this->GetWidgetName() << endl
         << "set tkcon::PRIV(protocol) {tkcon hide}" << endl
         << "set tkcon::OPT(showstatusbar) 0" << endl
         << "set tkcon::OPT(exec) \"\"" << endl
         << "tkcon::Init" << endl
         << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // Now let's map some elements of this console to widgets

  vtksys_stl::string name;

  name = this->Script("set ::tkcon::PRIV(console)");
  this->Internals->Console = vtkKWText::New();
  this->Internals->Console->SetWidgetName(name.c_str());
  this->Internals->Console->vtkKWCoreWidget::Create(app);

  name = this->Script("set ::tkcon::PRIV(menubar)");
  this->Internals->MainMenu = vtkKWMenu::New();
  this->Internals->MainMenu->SetWidgetName(name.c_str());
  this->Internals->MainMenu->vtkKWCoreWidget::Create(app);
  this->Internals->MainMenu->DeleteMenuItem("Console");

  name = this->Script("set ::tkcon::PRIV(menubar)");
  name += ".pop";
  this->Internals->PopupMenu = vtkKWMenu::New();
  this->Internals->PopupMenu->SetWidgetName(name.c_str());
  this->Internals->PopupMenu->vtkKWCoreWidget::Create(app);
  this->Internals->PopupMenu->DeleteMenuItem("Console");

  this->PostCreate();
}

//----------------------------------------------------------------------------
void vtkKWTkcon::AppendText(const char* text)
{
  if (this->Internals && this->Internals->Console)
    {
    this->Internals->Console->AppendText(text);
    this->Internals->Console->AppendText("\n");
    }
}

//----------------------------------------------------------------------------
void vtkKWTkcon::Focus()
{
  this->Superclass::Focus();

  if (this->Internals && this->Internals->Console)
    {
    this->Internals->Console->Focus();
    }
}

//----------------------------------------------------------------------------
void vtkKWTkcon::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Internals)
    {
    this->PropagateEnableState(this->Internals->Console);
    this->PropagateEnableState(this->Internals->MainMenu);
    this->PropagateEnableState(this->Internals->PopupMenu);
    }
}

//----------------------------------------------------------------------------
void vtkKWTkcon::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

