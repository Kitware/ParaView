/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSourcesNavigationWindow.cxx
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
#include "vtkPVSourcesNavigationWindow.h"

#include "vtkKWApplication.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"

#include <stdarg.h>

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVSourcesNavigationWindow );

//------------------------------------------------------------------------------
vtkPVSourcesNavigationWindow::vtkPVSourcesNavigationWindow()
{
  this->Width     = -1;
  this->Height    = -1;
  this->Canvas    = vtkKWWidget::New();
  this->ScrollBar = vtkKWWidget::New();
  this->PopupMenu = vtkKWMenu::New();
}

//------------------------------------------------------------------------------
vtkPVSourcesNavigationWindow::~vtkPVSourcesNavigationWindow()
{
  if (this->Canvas)
    {
    this->Canvas->Delete();
    }
  if (this->ScrollBar)
    {
    this->ScrollBar->Delete();
    }
  if ( this->PopupMenu )
    {
    this->PopupMenu->Delete();
    }
}

//------------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::CalculateBBox(vtkKWWidget* canvas, 
                                                 const char* name, 
                                                 int bbox[4])
{
  const char *result;
  // Get the bounding box for the name. We may need to highlight it.
  result = this->Script("%s bbox %s", canvas->GetWidgetName(), name);
  sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);
}

//------------------------------------------------------------------------------
const char* vtkPVSourcesNavigationWindow::CreateCanvasItem(const char *format, ...)
{
  char event[16000];
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  return this->Script(event);
}

//------------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::ChildUpdate(vtkPVSource*,int)
{
  cout << "Subclass should do this." << endl;
  cout << "I am " << this->GetClassName() << endl;
  abort();
}

//------------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::Update(vtkPVSource *currentSource, int nobind)
{
  // Clear the canvas
  this->Script("%s delete all", this->Canvas->GetWidgetName());

  this->ChildUpdate(currentSource, nobind);

  this->Reconfigure();
}

void vtkPVSourcesNavigationWindow::Reconfigure()
{
  this->Script("pack forget %s", this->ScrollBar->GetWidgetName());
  int bbox[4];
  this->CalculateBBox(this->Canvas, "all", bbox);
  int height = atoi(this->Script("winfo height %s", 
                                 this->Canvas->GetWidgetName()));
  if ( height > 1 && (bbox[3] - bbox[1]) > height )
    {
    this->Script("pack %s -fill both -side right", 
                 this->ScrollBar->GetWidgetName());
    }

  this->Script("%s configure -scrollregion \"%d %d %d %d\"", 
               this->Canvas->GetWidgetName(), 
               0, bbox[1], 341, bbox[3]);
  this->PostChildUpdate();
  
}


//------------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Window already created");
    return;
    }

  this->SetApplication(app);

  ostrstream opts;
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s %s", wname, (args?args:""));

  if (this->Width > 0 && this->Height > 0)
    {
    opts << " -width " << this->Width << " -height " << this->Height;
    }
  else if (this->Width > 0)
    {
    opts << " -width " << this->Width;
    }
  else if (this->Height > 0)
    {
    opts << " -height " << this->Height;
    }

  opts << " -bg white" << ends;

  char* optstr = opts.str();
  this->Canvas->SetParent(this);
  this->Canvas->Create(this->Application, "canvas", optstr); 
  delete[] optstr;

  ostrstream command;
  this->ScrollBar->SetParent(this);
  command << "-command \"" <<  this->Canvas->GetWidgetName()
          << " yview\"" << ends;
  char* commandStr = command.str();
  this->ScrollBar->Create(this->Application, "scrollbar", commandStr);
  delete[] commandStr;

  this->Script("%s configure -yscrollcommand \"%s set\"", 
               this->Canvas->GetWidgetName(),
               this->ScrollBar->GetWidgetName());

  this->Canvas->SetBind(this, "<Configure>", "Reconfigure");

  this->Script("pack %s -fill both -expand t -side left", 
               this->Canvas->GetWidgetName());
  this->PopupMenu->SetParent(this);
  this->PopupMenu->Create(this->Application, "-tearoff 0");
  this->PopupMenu->AddCommand("Delete", this, "DeleteWidget", 0, 
                              "Delete current widget");
  char *var = this->PopupMenu->CreateCheckButtonVariable(this, "Visibility");
  this->PopupMenu->AddCheckButton("Visibility", var, this, "Visibility", 0,
                                  "Set visibility for the current object");  
  delete [] var;
  this->PopupMenu->AddCascade("Representation", 0, 0);
  this->PopupMenu->AddCascade("Interpolation", 0, 0);
  /*
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  this->PopupMenu->AddCascade(
    "VTK Filters", pvApp->GetMainWindow()->GetFilterMenu(),
    4, "Choose a filter from a list of VTK filters");
  */
  this->ChildCreate();
}

//------------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::SetWidth(int width)
{
  if (this->Width == width)
    {
    return;
    }

  this->Modified();
  this->Width = width;

  if (this->Application != NULL)
    {
    this->Script("%s configure -width %d", this->Canvas->GetWidgetName(), 
                 width);
    }
}

//------------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::SetHeight(int height)
{
  if (this->Height == height)
    {
    return;
    }

  this->Modified();
  this->Height = height;

  if (this->Application != NULL)
    {
    this->Script("%s configure -height %d", this->Canvas->GetWidgetName(), 
                 height);
    }
}

//------------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::HighlightObject(const char* widget, int onoff)
{
  this->Script("%s itemconfigure %s -fill %s", 
               this->Canvas->GetWidgetName(), widget,
               (onoff ? "red" : "blue") );
}

//----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::DisplayModulePopupMenu(const char* module, 
                                                   int x, int y)
{
  //cout << "Popup for module: " << module << " at " << x << ", " << y << endl;
  vtkKWApplication *app = this->Application;
  ostrstream str;
  if ( app->EvaluateBooleanExpression("%s IsDeletable", module) )
    {
    str << "ExecuteCommandOnModule " << module << " DeleteCallback" << ends;
    this->PopupMenu->SetEntryCommand("Delete", this, str.str());
    this->PopupMenu->SetState("Delete", vtkKWMenu::Normal);
    }
  else
    {
    this->PopupMenu->SetState("Delete", vtkKWMenu::Disabled);
    }
  str.rdbuf()->freeze(0);
  ostrstream str1;
  if ( !app->EvaluateBooleanExpression("%s GetHideDisplayPage", module) )
    {
    this->PopupMenu->SetState("Visibility", vtkKWMenu::Normal);
    this->PopupMenu->SetState("Representation", vtkKWMenu::Normal);
    this->PopupMenu->SetState("Interpolation", vtkKWMenu::Normal);
    char *var = this->PopupMenu->CreateCheckButtonVariable(this, "Visibility");
    str1 << "[ " << module << " GetPVOutput ] SetVisibility $" 
         << var << ";"
         << "[ [ Application GetMainWindow ] GetMainView ] EventuallyRender" 
         <<  ends;
    this->PopupMenu->SetEntryCommand("Visibility", str1.str());
    if ( app->EvaluateBooleanExpression("[ %s GetPVOutput ] GetVisibility",
                                        module) )
      {
      this->Script("set %s 1", var);
      }
    else
      {
      this->Script("set %s 0", var);
      }
    delete [] var;
    this->Script("%s SetCascade [ %s GetIndex \"Representation\" ] "
                 "[ [ [ [ %s GetPVOutput ] GetRepresentationMenu ] "
                 "GetMenu ] GetWidgetName ]",
                 this->PopupMenu->GetTclName(),
                 this->PopupMenu->GetTclName(), module);

    this->Script("%s SetCascade [ %s GetIndex \"Interpolation\" ] "
                 "[ [ [ [ %s GetPVOutput ] GetInterpolationMenu ] "
                 "GetMenu ] GetWidgetName ]",
                 this->PopupMenu->GetTclName(),
                 this->PopupMenu->GetTclName(), module);
                 
    }
  else
    {
    this->PopupMenu->SetState("Visibility", vtkKWMenu::Disabled);
    this->PopupMenu->SetState("Representation", vtkKWMenu::Disabled);
    this->PopupMenu->SetState("Interpolation", vtkKWMenu::Disabled);
    }
  this->Script("tk_popup %s %d %d", this->PopupMenu->GetWidgetName(), x, y);
  str1.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::ExecuteCommandOnModule(
  const char* module, const char* command)
{
  //cout << "Executing: " << command << " on module: " << module << endl;
  this->Script("%s %s", module, command);
}

//----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Canvas: " << this->GetCanvas() << endl;
}
