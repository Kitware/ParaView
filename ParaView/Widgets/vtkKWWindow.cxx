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
#ifdef _WIN32
#include "vtkKWRegisteryUtilities.h"
#endif
#include "KitwareLogo.h"
#include "vtkKWPointerArray.h"

#define NUMBER_OF_RECENT_FILES 5

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
  
  int Same( const char *filename, vtkKWObject *target, 
	    const char *command )
    {
      if ( !this->Command || !this->File || !filename || !command) 
	{
	return 0;
	}
      int res1 = strcmp(filename, this->File)==0;
      int res2 = strcmp(command, this->Command)==0;
      return res2;
    }
  int InsertToMenu(int pos, vtkKWMenu *menu);
  void SetFile(const char *file)
    {
      if ( this->File )
	{
	free ( this->File );
	this->File = 0;
	}
      if ( file )
	{
	this->File = strdup( file );
	}
    }
  void SetCommand(const char *command)
    {
      if ( this->Command )
	{
	free ( this->Command );
	this->Command = 0;
	}
      if ( command )
	{
	this->Command = strdup( command );
	}
    }
  void SetTarget(vtkKWObject *target)
    {
      this->Target = target;
    }
  char *GetFile() { return this->File; }
  char *GetCommand() { return this->Command; }
  
  static int TotalCount;

private:
  int ReferenceCount;

  vtkKWWindowMenuEntry()
    {
      this->File    = 0;
      this->Target  = 0;
      this->Command = 0;
      this->ReferenceCount = 1;
      this->TotalCount ++;
    }
  ~vtkKWWindowMenuEntry();
  char *File;
  char *Command;
  vtkKWObject *Target;  
};

int vtkKWWindowMenuEntry::TotalCount = 0;

vtkKWWindowMenuEntry::~vtkKWWindowMenuEntry()
{
  if ( this->File )
    {
    free( this->File );
    }
  if ( this->Command )
    {
    free( this->Command );
    }
  this->TotalCount --;
}

int vtkKWWindowMenuEntry::InsertToMenu( int pos, vtkKWMenu *menu )
{
  if ( this->File && this->Target && this->Command )
    {
    char *file = strdup(this->File);
    file[0] = pos + '0';
    menu->InsertCommand( menu->GetIndex("Close") - 1, 
			 file, this->Target, this->Command, 0 );
    //cout << "Insert to menu: " << file << " (" << this->Command << ")"
    //<< endl;
    free( file );
    return 1;
    }
  return 0;
}

//------------------------------------------------------------------------------
vtkKWWindow* vtkKWWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWWindow");
  if(ret)
    {
    return (vtkKWWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWWindow;
}

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
}

vtkKWWindow::~vtkKWWindow()
{
  if ( this->RecentFiles )
    {
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
  this->MenuFile->AddCommand("Load Script", this, "LoadScript", 0);

  // add render quality setting
  this->PageMenu->SetTearOff(0);
  this->PageMenu->Create(this->Application,"");

  char* rbv = 
    this->PageMenu->CreateRadioButtonVariable(this,"PageSetup");
  // now add our own menu options 
  this->Script( "set %s 0", rbv );
  this->PageMenu->AddRadioButton(0,"100 DPI",rbv,this,"OnPrint1", 0);
  this->PageMenu->AddRadioButton(1,"150 DPI",rbv,this,"OnPrint2", 1);
  this->PageMenu->AddRadioButton(2,"300 DPI",rbv,this,"OnPrint3", 0);
  delete [] rbv;
  // add the Print option

#ifdef _WIN32
  this->MenuFile->AddCascade("Page Setup", this->PageMenu,8);
#endif

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

void vtkKWWindow::OnPrint1() 
{
  this->PrintTargetDPI = 100;
}
void vtkKWWindow::OnPrint2() 
{
  this->PrintTargetDPI = 150;
}
void vtkKWWindow::OnPrint3() 
{
  this->PrintTargetDPI = 300;
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
	       this->Menu->GetWidgetName());  
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

  this->Script("tk_getOpenFile -title \"Load Script\" -filetypes {{{Tcl Script} {.tcl}}}");
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


void vtkKWWindow::AddRecentFilesToMenu(char *key, vtkKWObject *target)
{
#ifdef _WIN32
  int i;
  char fkey[1024];
  char *KeyName[4] = {"File1","File2","File3","File4"};
  char *CmdName[4] = {"File1Cmd","File2Cmd","File3Cmd","File4Cmd"};
  char Cmd[1024];
  
  if (!key)
    {
    sprintf(fkey,"Software\\Kitware\\%s\\MRU",this->GetClassName());
    }
  else
    {
    sprintf(fkey,"Software\\Kitware\\%s\\MRU",key);
    }
  
  HKEY hKey;
  this->NumberOfMRUFiles = 0;
  if(RegOpenKeyEx(HKEY_CURRENT_USER, fkey, 
		  0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
    return;
    }
  else
    {
    char File[1024];
    
    this->GetMenuFile()->InsertSeparator(
      this->GetMenuFile()->GetIndex("Close") - 1);

    for (i = 0; i < 4; i++)
      {
      vtkKWRegisteryUtilities::ReadAValue(hKey, File, KeyName[i],"");
      vtkKWRegisteryUtilities::ReadAValue(hKey, Cmd, CmdName[i],"Open");
      if (strlen(File) > 1)
        {
        char *cmd = new char [strlen(Cmd) + strlen(File) + 10];
        sprintf(cmd,"%s {%s}", Cmd, File);
	this->InsertRecentFileToMenu(File, target, cmd);
	delete [] cmd;
        }    
      }
    this->UpdateRecentMenu();
    }
  RegCloseKey(hKey);
  
#endif
}

void vtkKWWindow::AddRecentFile(char *key, char *name,vtkKWObject *target,
                                const char *command)
{
#ifdef _WIN32
  char fkey[1024];
  char File[1024];
  char Cmd[1024];
  
  if (!key)
    {
    sprintf(fkey,"Software\\Kitware\\%s\\MRU",this->GetClassName());
    }
  else
    {
    sprintf(fkey,"Software\\Kitware\\%s\\MRU",key);
    }
  
  HKEY hKey;
  DWORD dwDummy;

  if(RegCreateKeyEx(HKEY_CURRENT_USER, fkey,
		    0, "", REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, 
		    NULL, &hKey, &dwDummy) != ERROR_SUCCESS) 
    {
    return;
    }
  else
    {
    // if this is the same as the current File1 then ignore
    vtkKWRegisteryUtilities::ReadAValue(hKey, File,"File1","");
    if (!strcmp(name,File))
      {
      RegCloseKey(hKey);
      return;
      }
    
    // if this is the first addition
    if (!this->NumberOfMRUFiles)
      {
      this->GetMenuFile()->InsertSeparator(
        this->GetMenuFile()->GetIndex("Close") - 1);
      }
    
    // remove the old entry number 4
    vtkKWRegisteryUtilities::ReadAValue(hKey, File,"File4","");
    /*
    if (strlen(File) > 1)
      {
      this->GetMenuFile()->DeleteMenuItem(
        this->GetMenuFile()->GetIndex("Close") - 2);
      this->NumberOfMRUFiles--;
      }
    */
    // move the other three down
    vtkKWRegisteryUtilities::ReadAValue(hKey, File,"File3","");
    vtkKWRegisteryUtilities::ReadAValue(hKey, Cmd,"File3Cmd","");
    RegSetValueEx(hKey, "File4", 0, REG_SZ, 
		  (CONST BYTE *)(const char *)File, strlen(File)+1);
    RegSetValueEx(hKey, "File4Cmd", 0, REG_SZ, 
		  (CONST BYTE *)(const char *)Cmd, strlen(Cmd)+1);
    vtkKWRegisteryUtilities::ReadAValue(hKey, File,"File2","");
    vtkKWRegisteryUtilities::ReadAValue(hKey, Cmd,"File2Cmd","");
    RegSetValueEx(hKey, "File3", 0, REG_SZ, 
		  (CONST BYTE *)(const char *)File, strlen(File)+1);
    RegSetValueEx(hKey, "File3Cmd", 0, REG_SZ, 
		  (CONST BYTE *)(const char *)Cmd, strlen(Cmd)+1);
    vtkKWRegisteryUtilities::ReadAValue(hKey, File,"File1","");
    vtkKWRegisteryUtilities::ReadAValue(hKey, Cmd,"File1Cmd","");
    RegSetValueEx(hKey, "File2", 0, REG_SZ, 
		  (CONST BYTE *)(const char *)File, strlen(File)+1);
    RegSetValueEx(hKey, "File2Cmd", 0, REG_SZ, 
		  (CONST BYTE *)(const char *)Cmd, strlen(Cmd)+1);
    RegSetValueEx(hKey, "File1", 0, REG_SZ, 
		  (CONST BYTE *)(const char *)name, 
		  strlen(name)+1);
    RegSetValueEx(hKey, "File1Cmd", 0, REG_SZ, 
		  (CONST BYTE *)(const char *)command, 
                  strlen(command)+1);

    this->NumberOfMRUFiles++;
    // add the new entry
    if (strlen(File) > 1)
      {
      char cmd[1024];
      sprintf(cmd,"%s {%s}",command, name);
      //cout << "Insert: " << name << " - " << cmd  << endl;
      this->InsertRecentFileToMenu(name, target, cmd);
      this->UpdateRecentMenu();
      /*
      char *file = new char [strlen(File) + 3];
      if (strlen(name) > 40)
        {
        char *name2 = new char [strlen(name)+1];
        sprintf(name2,"%s",name);
        name2[36] = '.';
        name2[37] = '.';
        name2[38] = '.';
        name2[39] = '\0';
        this->GetMenuFile()->InsertCommand(
          this->GetFileMenuIndex()+2,name2,target,cmd);
        delete [] name2;
        }
      else
        {
        this->GetMenuFile()->InsertCommand(
          this->GetFileMenuIndex()+2,name,target,cmd);
        }
      */
      }
    else
      {
      cout << "What is this" << endl;
      cout << "?????????????????????????????????????????????????" << endl;
      cout << "?????????????????????????????????????????????????" << endl;
      cout << "?????????????????????????????????????????????????" << endl;
      /*
      char cmd[1024];
      sprintf(cmd,"%s {%s}",command, name);
      if (strlen(name) > 40)
        {
        char *name2 = new char [strlen(name)+1];
        sprintf(name2,"%s",name);
        name2[36] = '.';
        name2[37] = '.';
        name2[38] = '.';
        name2[39] = '\0';
        this->GetMenuFile()->InsertCommand(
          this->GetFileMenuIndex()+2,name2,target,cmd);
        delete [] name2;
        }
      else
        {
        this->GetMenuFile()->InsertCommand(
          this->GetMenuFile()->GetIndex("Close")-1,name,target,cmd);
        }
      */
      }
    }
  RegCloseKey(hKey);
    
#endif
}

int vtkKWWindow::GetFileMenuIndex()
{
  // first find Close
  int clidx = this->GetMenuFile()->GetIndex("Close");  
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
  this->ExtractRevision(os,"$Revision: 1.40 $");
}

int vtkKWWindow::ExitDialog()
{
  ostrstream title;
  title << "Exit " << this->GetApplication()->GetApplicationName() << ends;
  char* ttl = title.str();
  ostrstream str;
  str << "Are you sure you want to exit " << this->GetApplication()->GetApplicationName() << "?" << ends;
  char* msg = str.str();
  
  int ret = vtkKWMessageDialog::PopupYesNo(
    this->GetApplication(), vtkKWMessageDialog::Question,
    ttl, msg);

  delete[] msg;
  delete[] ttl;
 
  return ret;
}

void vtkKWWindow::UpdateRecentMenu()
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
    for ( cc = 0; cc<NUMBER_OF_RECENT_FILES; cc++ ) 
      {
      vtkKWWindowMenuEntry *kc;
      if ( ( kc = (vtkKWWindowMenuEntry *)this->RecentFiles->Lookup(cc) ) )
	{
	kc->InsertToMenu(cc, this->GetMenuFile());
	this->RealNumberOfMRUFiles ++;
	}
      }
    }
}

void vtkKWWindow::InsertRecentFileToMenu(const char *filename, 
					 vtkKWObject *target, 
					 const char *command)
{
  //cout << "Insert: " << filename << endl;
  this->PrintRecentFiles();
  char *file = new char [strlen(filename) + 3];
  if ( strlen(filename) <= 40 )
    {
    sprintf(file, "  %s", filename);
    }
  else
    {
    int ii;
    int lastI, lastJ;
    for(ii=0; ii<=15; ii++)
      {
      if ( filename[ii] == '/' )
	{
	lastI = ii;
	}
      }
    for(ii=strlen(filename); static_cast<unsigned int>(ii)>=(strlen(filename)-25); ii--)
      {
      if ( filename[ii] == '/' )
	{
	lastJ = ii;
	}
      }
    char format[20];
    sprintf(format, "  %%.%ds...%%s", lastI+1);
    sprintf(file, format, filename, filename + lastJ);
    }
  //sprintf(file, "%d %.40s%s", i, filename, (strlen(filename)>40)?"...":"");

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
  for ( cc = 0; static_cast<unsigned int>(cc) < this->RecentFiles->GetSize(); cc ++ )
    {
    kc = (vtkKWWindowMenuEntry *)
      this->RecentFiles->Lookup(cc);
    if ( kc->Same( file, target, command ) )
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
    recent->SetTarget( target );
    recent->SetCommand( command );
    this->NumberOfMRUFiles++;
   }

  // prepend it to array  
  this->RecentFiles->Prepend( recent );
  while ( this->RecentFiles->GetSize() >= NUMBER_OF_RECENT_FILES )
    {
    if ( ( kc = (vtkKWWindowMenuEntry *)
	   this->RecentFiles->Lookup(NUMBER_OF_RECENT_FILES-1) ) )
      {
      kc->Delete();
      this->RecentFiles->Remove(NUMBER_OF_RECENT_FILES-1);
      }
    }

  delete [] file;
  this->PrintRecentFiles();
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
  for( cc=0; static_cast<unsigned int>(cc) < this->RecentFiles->GetSize(); cc ++ )
    {
    vtkKWWindowMenuEntry *kc;
    kc = (vtkKWWindowMenuEntry *)this->RecentFiles->Lookup(cc);
    if ( kc )
      {
      cout << " - " << kc->GetFile() << endl;      
      }
    else
      {
      cout << " -- null " << endl;
      }
    }
}
