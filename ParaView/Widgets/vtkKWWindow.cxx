/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWindow.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWViewCollection.h"
#include "vtkKWNotebook.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWWindowCollection.h"
#include "vtkKWRegisteryUtilities.h"
#include "KitwareLogo.h"
#include "vtkKWPointerArray.h"
#include "vtkKWEvent.h"

class vtkKWWindowMenuEntry
{
public:
  static vtkKWWindowMenuEntry *New() 
    {return new vtkKWWindowMenuEntry;};

  void Delete() {
    this->UnRegister(); }
  void Register() { this->ReferenceCount ++; }
  void Register(vtkObject *) {
    this->Register(); }
  void UnRegister()
    {
      if (--this->ReferenceCount <= 0)
	{
	delete this;
	}
    }
  void UnRegister(vtkObject *) {
    this->UnRegister(); }
  
  int Same( const char *filename, const char*fullfile, vtkKWObject *target, 
	    const char *command )
    {
      if ( !this->Command || !this->File || !filename || !target || !command) 
	{
	return 0;
	}
      return (strcmp(fullfile, this->FullFile) == 0);
    }
  int InsertToMenu(int pos, vtkKWMenu *menu);
  void SetFile(const char *file)
    {
      if ( this->File )
	{
	delete [] this->File;
	this->File = 0;
	}
      if ( file )
	{
	this->File = strcpy(new char[strlen(file)+1], file);
	}
    }
  void SetFullFile(const char *file)
    {
      if ( this->FullFile )
	{
	delete [] this->FullFile;
	this->FullFile = 0;
	}
      if ( file )
	{
	this->FullFile = strcpy(new char[strlen(file)+1], file);
	}
    }
  void SetCommand(const char *command)
    {
      if ( this->Command )
	{
	delete [] this->Command;
	this->Command = 0;
	}
      if ( command )
	{
	this->Command = strcpy(new char[strlen(command)+1], command);
	}
    }
  void SetTarget(vtkKWObject *target)
    {
      this->Target = target;
    }
  char *GetFile() { return this->File; }
  char *GetFullFile() { return this->FullFile; }
  char *GetCommand() { return this->Command; }
  vtkKWObject *GetTarget() { return this->Target; }
  
  static int TotalCount;

private:
  int ReferenceCount;

  vtkKWWindowMenuEntry()
    {
      this->File    = 0;
      this->FullFile= 0;
      this->Target  = 0;
      this->Command = 0;
      this->ReferenceCount = 1;
      this->TotalCount ++;
    }
  ~vtkKWWindowMenuEntry();
  char *File;
  char *FullFile;
  char *Command;
  vtkKWObject *Target;  
};

int vtkKWWindowMenuEntry::TotalCount = 0;

vtkKWWindowMenuEntry::~vtkKWWindowMenuEntry()
{
  if ( this->File )
    {
    delete [] this->File;
    }
  if ( this->FullFile )
    {
    delete [] this->FullFile;
    }
  if ( this->Command )
    {
    delete [] this->Command;
    }
  this->TotalCount --;
}

int vtkKWWindowMenuEntry::InsertToMenu( int pos, vtkKWMenu *menu )
{
  if ( this->File && this->Target && this->Command )
    {
    char *file = strcpy(new char[strlen(this->File)+1], this->File);
    file[0] = pos + '0';
    ostrstream str;
    str << this->Command << " \"" << this->FullFile << "\"" << ends;
    menu->InsertCommand( menu->GetIndex("Close") - 1, 
			 file, this->Target, str.str(), 0 );
    delete [] file;
    delete [] str.str();
    return 1;
    }
  return 0;
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWindow );

int vtkKWWindowCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

vtkKWWindow::vtkKWWindow()
{
  this->PropertiesParent = NULL;
  this->SelectedView = NULL;
  this->Views = vtkKWViewCollection::New();

  this->Menu = vtkKWMenu::New();
  this->Menu->SetParent(this);
  
  this->MenuFile = vtkKWMenu::New();
  this->MenuFile->SetParent(this->Menu);

  this->MenuHelp = vtkKWMenu::New();
  this->MenuHelp->SetParent(this->Menu);
  
  this->PageMenu = vtkKWMenu::New();
  this->PageMenu->SetParent(this->MenuFile);
  
  this->ToolbarFrame = vtkKWWidget::New();
  this->ToolbarFrame->SetParent(this);  

  this->MiddleFrame = vtkKWSplitFrame::New();
  this->MiddleFrame->SetParent(this);
  // Default is not interactively resizable.
  this->MiddleFrame->SetSeparatorWidth(0);
  this->MiddleFrame->SetFrame1MinimumWidth(360);
  this->MiddleFrame->SetFrame1Width(360);

  this->ViewFrame = vtkKWWidget::New();
  this->ViewFrame->SetParent(this->MiddleFrame->GetFrame2());

  this->StatusFrame = vtkKWWidget::New();
  this->StatusFrame->SetParent(this);
    
  this->StatusLabel = vtkKWWidget::New();
  this->StatusLabel->SetParent(this->StatusFrame);
  this->StatusImage = vtkKWWidget::New();
  this->StatusImage->SetParent(this->StatusFrame);
  this->StatusImageName = NULL;
  
  this->ProgressFrame = vtkKWWidget::New();
  this->ProgressFrame->SetParent(this->StatusFrame);
  this->ProgressGauge = vtkKWProgressGauge::New();
  this->ProgressGauge->SetParent(this->ProgressFrame);

  this->Notebook = vtkKWNotebook::New();
  
  this->CommandFunction = vtkKWWindowCommand;

  this->MenuEdit = NULL;
  this->MenuView = NULL;
  this->MenuWindow = NULL;
  this->MenuProperties = NULL;
  this->NumberOfMRUFiles = 0;
  this->RealNumberOfMRUFiles = 0;
  this->PrintTargetDPI = 100;

  this->SupportHelp = 1;

  this->WindowClass = NULL;
  this->SetWindowClass("KitwareWidget");

  this->PromptBeforeClose = 1;

  this->RecentFiles = 0;
  this->NumberOfRecentFiles = 5;

  this->ScriptExtension = 0;
  this->SetScriptExtension(".tcl");
}

vtkKWWindow::~vtkKWWindow()
{
  if ( this->RecentFiles )
    {
    vtkKWWindowMenuEntry *kc = 0;
    while( this->RecentFiles->GetSize() > 0 )
      {
      if ( ( kc = (vtkKWWindowMenuEntry *)
	     this->RecentFiles->Lookup( this->RecentFiles->GetSize()-1 ) ) )
	{
	kc->Delete();
	this->RecentFiles->Remove( this->RecentFiles->GetSize()-1 );
	}
      }
    this->RecentFiles->Delete();
    }
  this->Notebook->Delete();
  this->SetPropertiesParent(NULL);
  this->SetSelectedView(NULL);
  if (this->Views)
    {
    this->Views->Delete();
    this->Views = NULL;
    }
  this->Menu->Delete();
  this->PageMenu->Delete();
  this->MenuFile->Delete();
  this->MenuHelp->Delete();
  this->ToolbarFrame->Delete();
  this->ViewFrame->Delete();
  this->MiddleFrame->Delete();
  this->StatusFrame->Delete();
  this->StatusImage->Delete();
  this->StatusLabel->Delete();
  this->ProgressFrame->Delete();
  this->ProgressGauge->Delete();
  
  if (this->MenuEdit)
    {
    this->MenuEdit->Delete();
    }
  if (this->MenuProperties)
    {
    this->MenuProperties->Delete();
    }
  if (this->MenuView)
    {
    this->MenuView->Delete();
    }
  if (this->MenuWindow)
    {
    this->MenuWindow->Delete();
    }
  if (this->StatusImageName)
    {
    delete [] this->StatusImageName;
    }

  this->SetWindowClass(0);
  this->SetScriptExtension(0);
}

void vtkKWWindow::DisplayHelp()
{
  this->Application->DisplayHelp();
}

void vtkKWWindow::AddView(vtkKWView *v) 
{
  v->SetParentWindow(this);
  this->Views->AddItem(v);
}
void vtkKWWindow::RemoveView(vtkKWView *v) 
{
  v->SetParentWindow(NULL);
  this->Views->RemoveItem(v);
}

void vtkKWWindow::CreateDefaultPropertiesParent()
{
  if (!this->PropertiesParent)
    {
    vtkKWWidget *pp = vtkKWWidget::New();
    pp->SetParent(this->MiddleFrame->GetFrame1());
    pp->Create(this->Application,"frame","-bd 0");
    this->Script("pack %s -side left -fill both -expand t -anchor nw",
                 pp->GetWidgetName());
    this->SetPropertiesParent(pp);
    pp->Delete();
    }
  else
    {
    vtkDebugMacro("Properties Parent already set for Window");
    }
}

void vtkKWWindow::SetSelectedView(vtkKWView *_arg)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting SelectedView to " << _arg ); 
  if (this->SelectedView != _arg) 
    { 
    if (this->SelectedView != NULL) 
      { 
      this->SelectedView->Deselect(this);
      this->SelectedView->UnRegister(this); 
      }
    this->SelectedView = _arg; 
    if (this->SelectedView != NULL) 
      { 
      this->SelectedView->Register(this); 
      this->SelectedView->Select(this);
      } 
    this->Modified(); 
    } 
}

// invoke the apps exit when selected
void vtkKWWindow::Exit()
{  
  if ( this->ExitDialog() )
    {
    this->PromptBeforeClose = 0;
    this->Application->Exit();
    }
}

// invoke the apps close when selected
void vtkKWWindow::Close()
{
  if ( this->PromptBeforeClose &&
       this->Application->GetWindows()->GetNumberOfItems() <= 1 )
    {
    if ( !this->ExitDialog() )
      {
      return;
      }
    }
  this->CloseNoPrompt();
}

  
void vtkKWWindow::CloseNoPrompt()
{
  vtkKWView *v;

  // Give each view a chance to close
  this->Views->InitTraversal();
  while ((v = this->Views->GetNextKWView()))
    {
    v->Close();
    }

  // Close this window in the application. The
  // application will exit if there are no more windows.
  this->Application->Close(this);
}

void vtkKWWindow::Render()
{
  vtkKWView *v;
  
  this->Views->InitTraversal();
  while ((v = this->Views->GetNextKWView()))
    {
    v->Render();
    }
}

// invoke the apps about dialog when selected
void vtkKWWindow::DisplayAbout()
{
  this->Application->DisplayAbout(this);
}

void vtkKWWindow::SetStatusText(const char *text)
{
  if (text) 
    {
    this->Script("%s configure -text \"%s\"",
                 this->StatusLabel->GetWidgetName(), text);
    }
  else
    {
    this->Script("%s configure -text \"\"",
                 this->StatusLabel->GetWidgetName());
    }
}

// some common menus we provide here
vtkKWMenu *vtkKWWindow::GetMenuEdit()
{
  if (this->MenuEdit)
    {
    return this->MenuEdit;
    }
  
  this->MenuEdit = vtkKWMenu::New();
  this->MenuEdit->SetParent(this->GetMenu());
  this->MenuEdit->SetTearOff(0);
  this->MenuEdit->Create(this->Application,"");
  // Make sure Edit menu is next to file menu
  this->Menu->InsertCascade(1, "Edit", this->MenuEdit, 0);
  return this->MenuEdit;
}

vtkKWMenu *vtkKWWindow::GetMenuView()
{
  if (this->MenuView)
    {
    return this->MenuView;
    }

  this->MenuView = vtkKWMenu::New();
  this->MenuView->SetParent(this->GetMenu());
  this->MenuView->SetTearOff(0);
  this->MenuView->Create(this->Application, "");
  // make sure Help menu is on the right
  if (this->MenuEdit)
    { 
    this->Menu->InsertCascade(2, "View", this->MenuView, 0);
    }
  else
    {
    this->Menu->InsertCascade(1, "View", this->MenuView, 0);
    }
  
  return this->MenuView;
}

vtkKWMenu *vtkKWWindow::GetMenuWindow()
{
  if (this->MenuWindow)
    {
    return this->MenuWindow;
    }

  this->MenuWindow = vtkKWMenu::New();
  this->MenuWindow->SetParent(this->GetMenu());
  this->MenuWindow->SetTearOff(0);
  this->MenuWindow->Create(this->Application, "");
  // make sure Help menu is on the right
  if (this->MenuEdit)
    { 
    this->Menu->InsertCascade(1, "Window", this->MenuWindow, 0);
    }
  else
    {
    this->Menu->InsertCascade(2, "Window", this->MenuWindow, 0);
    }
  
  return this->MenuWindow;
}

vtkKWMenu *vtkKWWindow::GetMenuProperties()
{
  if (this->MenuProperties)
    {
    return this->MenuProperties;
    }
  
  this->MenuProperties = vtkKWMenu::New();
  this->MenuProperties->SetParent(this->GetMenu());
  this->MenuProperties->SetTearOff(0);
  this->MenuProperties->Create(this->Application,"");
  // make sure Help menu is on the right
  if (this->MenuView && this->MenuEdit)
    {
    this->Menu->InsertCascade(3, "Properties", this->MenuProperties, 0);
    }
  else if (this->MenuView || this->MenuEdit)
    {
    this->Menu->InsertCascade(2, "Properties", this->MenuProperties, 0);
    }
  else
    { 
    this->Menu->InsertCascade(1, "Properties", this->MenuProperties, 0);
    }
  return this->MenuProperties;
}

void vtkKWWindow::Create(vtkKWApplication *app, char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Window already created");
    return;
    }

  this->SetApplication(app);
  Tcl_Interp *interp = this->Application->GetMainInterp();

  // create the top level
  wname = this->GetWidgetName();
  this->Script("toplevel %s -visual best %s -class %s",wname,args,
	       this->WindowClass);
  this->Script("wm title %s {%s}",wname,
               app->GetApplicationName());
  this->Script("wm iconname %s {%s}",wname,
               app->GetApplicationName());
  this->Script("wm protocol %s WM_DELETE_WINDOW {%s Close}",
               wname, this->GetTclName());

  this->StatusFrame->Create(app,"frame","");
  this->Script("image create photo -height 34 -width 128");
  this->StatusImageName = new char [strlen(interp->result)+1];
  strcpy(this->StatusImageName,interp->result);
  this->CreateStatusImage();
  this->StatusImage->Create(app,"label",
                            "-relief sunken -bd 1 -height 38 -width 132 -fg #ffffff -bg #ffffff");
  this->Script("%s configure -image %s", this->StatusImage->GetWidgetName(),
               this->StatusImageName);
  this->Script("pack %s -side left -padx 2", 
               this->StatusImage->GetWidgetName());
  this->StatusLabel->Create(app,"label","-relief sunken -padx 3 -bd 1 -font \"Helvetica 10\" -anchor w");
  this->Script("pack %s -side left -padx 2 -expand yes -fill both",
               this->StatusLabel->GetWidgetName());
  this->Script("pack %s -side bottom -fill x -pady 2",
    this->StatusFrame->GetWidgetName());
  this->ProgressFrame->Create(app, "frame", "-relief sunken -borderwidth 2");
  this->ProgressGauge->SetLength(200);
  this->ProgressGauge->SetHeight(30);
  this->ProgressGauge->Create(app, "");
  this->Script("pack %s -side right -padx 2 -fill y", 
	       this->ProgressFrame->GetWidgetName());
  this->Script("pack %s -side right -padx 2 -pady 2",
               this->ProgressGauge->GetWidgetName());
  // To force the toolbar on top, I am create a separate "MiddleFrame" for the ViewFrame and PropertiesParent
  this->MiddleFrame->Create(app);
  this->Script("pack %s -side bottom -fill both -expand t",
    this->MiddleFrame->GetWidgetName());

  this->ViewFrame->Create(app,"frame","");
  this->Script("pack %s -side right -fill both -expand yes",
               this->ViewFrame->GetWidgetName());

  this->ToolbarFrame->Create(app, "frame", "-bd 0");
  this->Script("pack %s -side bottom -fill x -expand no",
    this->ToolbarFrame->GetWidgetName());
  
  // Set up standard menus
  this->Menu->SetTearOff(0);
  this->Menu->Create(app,"");
  this->MenuFile->SetTearOff(0);
  this->MenuFile->Create(app,"");
  this->Menu->AddCascade("File", this->MenuFile, 0);

  // add render quality setting
  this->PageMenu->SetTearOff(0);
  this->PageMenu->Create(this->Application,"");

  char* rbv = 
    this->PageMenu->CreateRadioButtonVariable(this,"PageSetup");
  // now add our own menu options 
  this->Script( "set %s 0", rbv );
  this->PageMenu->AddRadioButton(0,"100 DPI",rbv,this,"OnPrint1 1", 0);
  this->PageMenu->AddRadioButton(1,"150 DPI",rbv,this,"OnPrint2 1", 1);
  this->PageMenu->AddRadioButton(2,"300 DPI",rbv,this,"OnPrint3 1", 0);
  delete [] rbv;
  // add the Print option
  this->MenuFile->AddCascade("Page Setup", this->PageMenu,8);

  this->MenuFile->AddSeparator();
  this->MenuFile->AddCommand("Close", this, "Close", 0);
  this->MenuFile->AddCommand("Exit", this, "Exit", 1);
  // install the menu bar into this window
  this->InstallMenu(this->Menu);
  this->MenuHelp->SetTearOff(0);
  this->MenuHelp->Create(app,"");
  if ( this->SupportHelp )
    {
    this->Menu->AddCascade("Help", this->MenuHelp, 0);
    }
  this->MenuHelp->AddCommand("OnLine Help", this, "DisplayHelp", 0);
  this->MenuHelp->AddCommand("About", this, "DisplayAbout", 0);

  rbv = 
    this->GetMenuProperties()->CreateRadioButtonVariable(
      this->GetMenuProperties(),"Radio");
  this->GetMenuProperties()->AddRadioButton(0," Hide Properties", 
                                            rbv, this, "HideProperties", 1);
  delete [] rbv;
}

void vtkKWWindow::OnPrint1(int propagate) 
{
  this->PrintTargetDPI = 100;
  if ( propagate )
    {
    float dpi = 1;
    this->InvokeEvent(vtkKWEvent::ChangePrinterDPIEvent, &dpi);
    }
  else
    {
    this->PageMenu->Invoke( this->PageMenu->GetIndex("100 DPI") );
    }
}
void vtkKWWindow::OnPrint2(int propagate) 
{
  this->PrintTargetDPI = 150;
  if ( propagate )
    {
    float dpi = 2;
    this->InvokeEvent(vtkKWEvent::ChangePrinterDPIEvent, &dpi);
    }
  else
    {
    this->PageMenu->Invoke( this->PageMenu->GetIndex("150 DPI") );
    }
}
void vtkKWWindow::OnPrint3(int propagate) 
{
  this->PrintTargetDPI = 300;
  if ( propagate )
    {
    float dpi = 3;
    this->InvokeEvent(vtkKWEvent::ChangePrinterDPIEvent, &dpi);
    }
  else
    {
    this->PageMenu->Invoke( this->PageMenu->GetIndex("300 DPI") );
    }
}

void vtkKWWindow::ShowProperties()
{
  this->MiddleFrame->SetFrame1MinimumWidth(360);
  this->MiddleFrame->SetFrame1Width(360);
}

void vtkKWWindow::HideProperties()
{
  // make sure the variable is set, otherwise set it
  this->GetMenuProperties()->CheckRadioButton(
    this->GetMenuProperties(),"Radio",0);
  
  this->MiddleFrame->SetFrame1MinimumWidth(0);
  this->MiddleFrame->SetFrame1Width(0);
}

void vtkKWWindow::InstallMenu(vtkKWMenu* menu)
{ 
  this->Script("%s configure -menu %s", this->GetWidgetName(),
	       menu->GetWidgetName());  
}

void vtkKWWindow::UnRegister(vtkObject *o)
{
  if (!this->DeletingChildren)
    {
    // delete the children if we are about to be deleted
    if (this->ReferenceCount == this->Views->GetNumberOfItems() + 
        this->Children->GetNumberOfItems() + 1)
      {
      if (!(this->Views->IsItemPresent((vtkKWView *)o) ||
            this->Children->IsItemPresent((vtkKWWidget *)o)))
        {
        vtkKWWidget *child;
        vtkKWView *v;
        
        this->DeletingChildren = 1;
        this->Children->InitTraversal();
        while ((child = this->Children->GetNextKWWidget()))
          {
          child->SetParent(NULL);
          }
        // deselect if required
        if (this->SelectedView)
          {
          this->SetSelectedView(NULL);
          }
        this->Views->InitTraversal();
        while ((v = this->Views->GetNextKWView()))
          {
          v->SetParentWindow(NULL);
          }
        this->DeletingChildren = 0;
        }
      }
    }
  
  this->vtkObject::UnRegister(o);
}

void vtkKWWindow::LoadScript()
{
  char *path = NULL;

  this->Script("tk_getOpenFile -title \"Load Script\" -filetypes {{{Tcl Script} {%s}}}", this->ScriptExtension);
  path = 
    strcpy(new char[strlen(this->Application->GetMainInterp()->result)+1], 
	   this->Application->GetMainInterp()->result);
  if (strlen(path) != 0)
    {
    FILE *fin = fopen(path,"r");
    if (!fin)
      {
      vtkWarningMacro("Unable to open script file!");
      }
    else
      {
      this->LoadScript(path);
      }
    delete [] path;
    }
}

void vtkKWWindow::LoadScript(const char *path)
{
  // add this window as a variable
  this->Script("set InitialWindow %s", this->GetTclName());
  this->Script("source {%s}",path);
}

void vtkKWWindow::CreateStatusImage()
{
  int x, y;
  Tk_PhotoHandle photo;
  Tk_PhotoImageBlock block;
  
  block.width = 128;
  block.height = 34;
  block.pixelSize = 3;
  block.pitch = block.width*block.pixelSize;
  block.offset[0] = 0;
  block.offset[1] = 1;
  block.offset[2] = 2;
  block.pixelPtr = new unsigned char [block.pitch*block.height];

  photo = Tk_FindPhoto(this->Application->GetMainInterp(),
		       this->StatusImageName);
  if (!photo)
    {
    vtkWarningMacro("error looking up color ramp image");
    return;
    }
  
  unsigned char *pp = block.pixelPtr;
  float *lp = KITLOGO + 33*128*3;
  for (y = 0; y < 34; y++)
    {
    for (x = 0; x < 128; x++)
      {
      pp[0] = (unsigned char)(*lp*255.0);
      lp++;
      pp[1] = (unsigned char)(*lp*255.0);
      lp++;
      pp[2] = (unsigned char)(*lp*255.0);
      lp++;
      pp += block.pixelSize;
      }
    lp = lp - 2*128*3;
    }
  
  Tk_PhotoPutBlock(photo, &block, 0, 0, block.width, block.height);
  delete [] block.pixelPtr;
}

void vtkKWWindow::StoreRecentMenuToRegistery(char * vtkNotUsed(key))
{
  char KeyNameP[10];
  char CmdNameP[10];
  unsigned int i;
  if ( !this->RealNumberOfMRUFiles )
    {
    return;
    }

  //cout << "Store recent menu to registry; MRU: " 
  //     << this->RealNumberOfMRUFiles << endl;
  //cout << "Storing recent files: " << endl;
  //this->PrintRecentFiles();

  for (i = 0; i < this->NumberOfRecentFiles; i++)
    {
    sprintf(KeyNameP, "File%d", i);
    sprintf(CmdNameP, "File%dCmd", i);
    this->DeleteRegisteryValue( "MRU", KeyNameP );
    this->DeleteRegisteryValue( "MRU", CmdNameP );    
    if ( this->RecentFiles )
      {
      vtkKWWindowMenuEntry *vp = reinterpret_cast<vtkKWWindowMenuEntry *>(
	this->RecentFiles->Lookup(i) );
      if ( vp )
	{
	this->SetRegisteryValue("MRU", KeyNameP, vp->GetFullFile());
	this->SetRegisteryValue("MRU", CmdNameP, vp->GetCommand());
	}
      }
    }
}

void vtkKWWindow::AddRecentFilesToMenu(char *key, vtkKWObject *target)
{
  char KeyNameP[10];
  char CmdNameP[10];
  //cout << "vtkKWWindow::AddRecentFilesToMenu()" << endl;
  int i;
  char File[1024];
  char Cmd[1024];
  this->GetMenuFile()->InsertSeparator(
    this->GetMenuFile()->GetIndex("Close") - 1);

  for (i = this->NumberOfRecentFiles-1; i >=0; i--)
    {
    sprintf(KeyNameP, "File%d", i);
    sprintf(CmdNameP, "File%dCmd", i);
    if ( this->GetRegisteryValue("MRU", KeyNameP, File) )
      {
      if ( this->GetRegisteryValue("MRU", CmdNameP, Cmd) )
	{
	if (strlen(File) > 1)
	  {
	  this->InsertRecentFileToMenu(File, target, Cmd);
	  }
	}
      }
    }
  this->UpdateRecentMenu(key);
}

void vtkKWWindow::AddRecentFile(char *key, char *name,vtkKWObject *target,
                                const char *command)
{
  if ( this->RealNumberOfMRUFiles == 0 )
    {
    this->GetMenuFile()->InsertSeparator(
      this->GetMenuFile()->GetIndex("Close") - 1);    
    }
  this->InsertRecentFileToMenu(name, target, command);
  this->UpdateRecentMenu(key);
  this->StoreRecentMenuToRegistery(key);
}

int vtkKWWindow::GetFileMenuIndex()
{
  int clidx;
  // first find Close
  if ( this->GetMenuFile()->IsItemPresent("Close") )
    {
    clidx = this->GetMenuFile()->GetIndex("Close");  
    }
  else
    {
    // Close was removed, use Exit instead
    clidx = this->GetMenuFile()->GetIndex("Exit");  
    }
  if (this->NumberOfMRUFiles > 0)
    {
    return clidx - this->NumberOfMRUFiles - 2;
    }
  return clidx - 1;  
}

void vtkKWWindow::SerializeRevision(ostream& os, vtkIndent indent)
{
  vtkKWWidget::SerializeRevision(os,indent);
  os << indent << "vtkKWWindow ";
  this->ExtractRevision(os,"$Revision: 1.62 $");
}

int vtkKWWindow::ExitDialog()
{
  // Removing the default handler while we are displaying exit dialog.
  // Note that using wm protocol %s WM_DELETE_WINDOW {} installs
  // the default handlers, that's why we put the ; there.
  this->Script("wm protocol %s WM_DELETE_WINDOW {;}",
               this->GetWidgetName());

  ostrstream title;
  title << "Exit " << this->GetApplication()->GetApplicationName() << ends;
  char* ttl = title.str();
  ostrstream str;
  str << "Are you sure you want to exit " << this->GetApplication()->GetApplicationName() << "?" << ends;
  char* msg = str.str();
  
  int ret = vtkKWMessageDialog::PopupYesNo(
    this->GetApplication(), vtkKWMessageDialog::Question,
    ttl, msg);

  if (!ret)
    {
    this->Script("wm protocol %s WM_DELETE_WINDOW {%s Close}",
		 this->GetWidgetName(), this->GetTclName());
    }
  delete[] msg;
  delete[] ttl;
 
  return ret;
}

void vtkKWWindow::UpdateRecentMenu(char * vtkNotUsed(key))
{  
  int cc;
  for ( cc = 0; cc < this->RealNumberOfMRUFiles; cc ++ )
    {
    this->GetMenuFile()->DeleteMenuItem(
      this->GetMenuFile()->GetIndex("Close") - 2);
    }
  this->RealNumberOfMRUFiles = 0;
  if ( this->RecentFiles )
    {
    for ( cc = 0; static_cast<unsigned int>(cc)<this->NumberOfRecentFiles; 
	  cc++ ) 
      {
      vtkKWWindowMenuEntry *kc;
      if ( ( kc = (vtkKWWindowMenuEntry *)this->RecentFiles->Lookup(cc) ) )
	{
	kc->InsertToMenu(cc, this->GetMenuFile());
	this->RealNumberOfMRUFiles ++;
	}
      }
    }
  //this->PrintRecentFiles();
}

void vtkKWWindow::InsertRecentFileToMenu(const char *filename, 
					 vtkKWObject *target, 
					 const char *command)
{
  char *file = new char [strlen(filename) + 3];
  if ( strlen(filename) <= 40 )
    {
    sprintf(file, "  %s", filename);
    }
  else
    {
    int ii;
    int lastI, lastJ;
    lastI = 0;
    lastJ = 0;
    for(ii=0; ii<=15; ii++)
      {
      if ( filename[ii] == '/' )
	{
	lastI = ii;
	}
      }
    for(ii=strlen(filename); 
        static_cast<unsigned int>(ii)>=(strlen(filename)-25); ii--)
      {
      if ( filename[ii] == '/' )
	{
	lastJ = ii;
	}
      }
    if (!lastJ)
      {
      lastJ = ii;
      }
    char format[20];
    sprintf(format, "  %%.%ds...%%s", lastI+1);
    sprintf(file, format, filename, filename + lastJ);
    }

  if ( !this->RecentFiles )
    {
    this->RecentFiles = vtkKWPointerArray::New();
    }

  //cout << "We have recent files: "
  //     << this->RecentFiles << ", now let us put something in" << endl;
  
  // Find current one
  vtkKWWindowMenuEntry *recent = 0;
  vtkKWWindowMenuEntry *kc = 0;
  int cc;
  for ( cc = 0; 
	static_cast<unsigned int>(cc) < this->RecentFiles->GetSize(); cc ++ )
    {
    kc = (vtkKWWindowMenuEntry *)
      this->RecentFiles->Lookup(cc);
    if ( kc->Same( file, filename, target, command ) )
      {
      recent = kc;
      // delete it from array
      //cout << "Delete from array: " << kc->GetFile() << endl;
      this->RecentFiles->Remove(cc);
      break;
      }
    }

  if ( ! recent )
    {
    recent = vtkKWWindowMenuEntry::New();
    recent->SetFile( file );
    recent->SetFullFile( filename );
    recent->SetTarget( target );
    recent->SetCommand( command );
    this->NumberOfMRUFiles++;
   }

  // prepend it to array  
  this->RecentFiles->Prepend( recent );
  while ( this->RecentFiles->GetSize() > this->NumberOfRecentFiles )
    {
    if ( ( kc = (vtkKWWindowMenuEntry *)
	   this->RecentFiles->Lookup(this->NumberOfRecentFiles) ) )
      {
      kc->Delete();
      this->RecentFiles->Remove(this->NumberOfRecentFiles);
      }
    }

  delete [] file;
  //cout << "---------------------------------------" << endl;
}

void vtkKWWindow::PrintRecentFiles()
{
  if ( !this->RecentFiles )
    {
    cout << "No recent files" << endl;
    return;
    }
  
  cout << "Recent files: " << endl;
  int cc;
  for( cc=0; 
       static_cast<unsigned int>(cc) < this->RecentFiles->GetSize(); cc ++ )
    {
    vtkKWWindowMenuEntry *kc;
    kc = (vtkKWWindowMenuEntry *)this->RecentFiles->Lookup(cc);
    if ( kc )
      {
      cout << " - " << kc->GetFile() << " (" 
	   << kc->GetTarget()->GetTclName() << " " 
	   << kc->GetCommand() << ")"
	   << endl;
      }
    else
      {
      cout << " -- null " << endl;
      }
    }
}


int vtkKWWindow::SetRegisteryValue(const char* subkey, const char* key, 
				   const char* format, ...)
{
  int res = 0;
  char buffer[100];
  char value[16000];
  sprintf(buffer, "%s\\%s", 
	  this->GetApplication()->GetApplicationVersionName(),
	  subkey);
  va_list var_args;
  va_start(var_args, format);
  vsprintf(value, format, var_args);
  va_end(var_args);
  
  vtkKWRegisteryUtilities *reg 
    = this->GetApplication()->GetRegistery(
      this->GetApplication()->GetApplicationName());
  res = reg->SetValue(buffer, key, value);
  //cout << "SetRegisteryValue(" << buffer << ", " << key << ", "
  //     << value << ")" << endl;
  return res;
}

int vtkKWWindow::DeleteRegisteryValue(const char* subkey, const char* key)
{
  int res = 0;
  char buffer[100];
  sprintf(buffer, "%s\\%s", 
	  this->GetApplication()->GetApplicationVersionName(),
	  subkey);
  
  vtkKWRegisteryUtilities *reg 
    = this->GetApplication()->GetRegistery(
      this->GetApplication()->GetApplicationName());
  res = reg->DeleteValue(buffer, key);
  //cout << "DeleteRegisteryValue(" << buffer << ", " << key << ")" << endl;
  return res;
}

int vtkKWWindow::GetRegisteryValue(const char* subkey, const char* key, 
				   char* value)
{
  int res = 0;
  char buff[1024];
  *value = 0;
  char buffer[100];
  sprintf(buffer, "%s\\%s", 
	  this->GetApplication()->GetApplicationVersionName(),
	  subkey);

  vtkKWRegisteryUtilities *reg 
    = this->GetApplication()->GetRegistery(
      this->GetApplication()->GetApplicationName());
  res = reg->ReadValue(buffer, key, buff);
  //cout << "GetRegisteryValue(" << buffer << ", " << key << ")" << endl;
  if ( *buff )
    {
    strcpy(value, buff);
    }  
  return res;
}

float vtkKWWindow::GetFloatRegisteryValue(const char* subkey, const char* key)
{
  float res = 0;
  char buffer[1024];
  if ( this->GetRegisteryValue( subkey, key, buffer ) )
    {
    res = atof(buffer);
    }
  return res;
}

int vtkKWWindow::GetIntRegisteryValue(const char* subkey, const char* key)
{
  int res = 0;
  char buffer[1024];
  if ( this->GetRegisteryValue( subkey, key, buffer ) )
    {
    res = atoi(buffer);
    }
  return res;
}
