/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWDialog.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWDialog );
vtkCxxRevisionMacro(vtkKWDialog, "1.36");

int vtkKWDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWDialog::vtkKWDialog()
{
  this->CommandFunction = vtkKWDialogCommand;
  this->Done = 1;
  this->TitleString = 0;
  this->SetTitleString("Kitware Dialog");
  this->Beep = 0;
  this->BeepType = 0;
  this->MasterWindow = 0;
  this->InvokeAtPointer = 0;
  this->GrabDialog = 1;
}

//----------------------------------------------------------------------------
vtkKWDialog::~vtkKWDialog()
{
  this->SetTitleString(0);
  this->SetMasterWindow(0);
}

#include "vtkKWMessageDialog.h"

//----------------------------------------------------------------------------
int vtkKWDialog::Invoke()
{
  this->Done = 0;

  this->Application->SetDialogUp(1);

  int width, height;

  int x, y;

  if (this->InvokeAtPointer)
    {
    sscanf(this->Script("concat [winfo pointerx .] [winfo pointery .]"),
           "%d %d", &x, &y);
    }
  else
    {
    int sw, sh;
    sscanf(this->Script("concat [winfo screenwidth .] [winfo screenheight .]"),
           "%d %d", &sw, &sh);

    if (this->GetMasterWindow())
      {
      this->Script("wm geometry %s", this->GetMasterWindow()->GetWidgetName());
      sscanf(this->GetApplication()->GetMainInterp()->result, "%dx%d+%d+%d",
             &width, &height, &x, &y);

      x += width / 2;
      y += height / 2;

      if (x > sw - 200)
        {
        x = sw / 2;
        }
      if (y > sh - 200)
        {
        y = sh / 2;
        }
      }
    else
      {
      x = sw / 2;
      y = sh / 2;
      }
    }

  width = this->GetWidth();
  height = this->GetHeight();

  if (x > width / 2)
    {
    x -= width / 2;
    }
  if (y > height / 2)
    {
    y -= height / 2;
    }

  this->Script("wm geometry %s +%d+%d", this->GetWidgetName(),
               x, y);

  // map the window
  this->Script("wm deiconify %s",this->GetWidgetName());

  this->Script("focus %s",this->GetWidgetName());
  this->Script("update idletasks");
  if (this->GrabDialog)
    {
    this->Grab();
    }
  if ( this->Beep )
    {
    this->Script("bell");
    }

  // do a grab
  // wait for the end
  while (!this->Done)
    {
    Tcl_DoOneEvent(0);    
    }
  if (this->GrabDialog)
    {
    this->ReleaseGrab();
    }
  this->Application->SetDialogUp(0);
  return (this->Done-1);
}

//----------------------------------------------------------------------------
void vtkKWDialog::Display()
{
  this->Done = 0;

  // map the window
  this->Script("wm deiconify %s",this->GetWidgetName());
  this->Script("focus %s",this->GetWidgetName());
  this->Script("update idletasks");
  this->Grab();
}

//----------------------------------------------------------------------------
void vtkKWDialog::Cancel()
{
  this->Script("wm withdraw %s",this->GetWidgetName());
  this->ReleaseGrab();

  this->Done = 1;  
}

//----------------------------------------------------------------------------
void vtkKWDialog::OK()
{
  this->Script("wm withdraw %s",this->GetWidgetName());
  this->ReleaseGrab();
  this->Done = 2;  
}

//----------------------------------------------------------------------------
void vtkKWDialog::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Dialog already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  if (this->MasterWindow)
    {
    this->Script("toplevel %s -class %s %s",
                 wname,
                 this->MasterWindow->GetWindowClass(),
                 (args?args:""));
    }
  else
    {
    this->Script("toplevel %s %s",wname,(args?args:""));
    }
  this->Script("wm title %s \"%s\"",wname,this->TitleString);
  this->Script("wm iconname %s \"Dialog\"",wname);
  this->Script("wm protocol %s WM_DELETE_WINDOW {%s Cancel}",
               wname, this->GetTclName());
  this->Script("wm withdraw %s",wname);
  if (this->MasterWindow)
    {
    this->Script("wm transient %s %s", wname, 
                 this->MasterWindow->GetWidgetName());
    }

}

//----------------------------------------------------------------------------
vtkKWWindow *vtkKWDialog::GetMasterWindow()
{
  return this->MasterWindow;
}

//----------------------------------------------------------------------------
void vtkKWDialog::SetMasterWindow(vtkKWWindow* win)
{
  if (this->MasterWindow != win) 
    { 
    this->MasterWindow = win; 
    if (this->MasterWindow) 
      { 
      if (this->Application)
        {
        this->Script("wm transient %s %s", this->GetWidgetName(), 
                     this->MasterWindow->GetWidgetName());
        }
      } 
    this->Modified(); 
    }   
}

//----------------------------------------------------------------------------
void vtkKWDialog::SetTitle( const char* title )
{
  if (this->Application)
    {
    this->Script("wm title %s \"%s\"", this->GetWidgetName(), 
                 title);
    }
  else
    {
    this->SetTitleString(title);
    }
}

//----------------------------------------------------------------------------
int vtkKWDialog::GetWidth()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  return atoi(this->Script("winfo reqwidth %s", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
int vtkKWDialog::GetHeight()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  return atoi(this->Script("winfo reqheight %s", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Beep: " << this->GetBeep() << endl;
  os << indent << "BeepType: " << this->GetBeepType() << endl;
  os << indent << "InvokeAtPointer: " << this->GetInvokeAtPointer() << endl;
}

