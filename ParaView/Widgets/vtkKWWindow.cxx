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
#include "vtkKWWindow.h"

#include "KitwareLogo.h"
#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWImageLabel.h"
#include "vtkKWLabel.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWNotebook.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWRegisteryUtilities.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWView.h"
#include "vtkKWViewCollection.h"
#include "vtkKWWidgetCollection.h"
#include "vtkKWWindowCollection.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"
#include "vtkVector.txx"

vtkCxxSetObjectMacro(vtkKWWindow, PropertiesParent, vtkKWWidget);

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
  int InsertToMenu(int pos, const char* menuEntry, vtkKWMenu *menu);
  void SetFile(const char *file)
    {
      if ( this->File )
        {
        delete [] this->File;
        this->File = 0;
        }
      if ( file )
        {
        this->File = vtkString::Duplicate(file);
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
        this->FullFile = vtkString::Duplicate(file);
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
        this->Command = vtkString::Duplicate(command);
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

int vtkKWWindowMenuEntry::InsertToMenu( int pos, const char* menuEntry,
                                        vtkKWMenu *menu )
{
  if ( this->File && this->Target && this->Command )
    {
    char *file = vtkString::Duplicate(this->File);
    file[0] = pos + '0';
    ostrstream str;
    str << this->Command << " \"" << this->FullFile << "\"" << ends;
    menu->InsertCommand( menu->GetIndex(menuEntry), 
                         file, this->Target, str.str(), 0, this->FullFile );
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
  this->UseMenuProperties = 1;
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
  this->MiddleFrame->SetSeparatorSize(0);
  this->MiddleFrame->SetFrame1MinimumSize(360);
  this->MiddleFrame->SetFrame1Size(360);

  this->ViewFrame = vtkKWWidget::New();
  this->ViewFrame->SetParent(this->MiddleFrame->GetFrame2());

  this->StatusFrame = vtkKWWidget::New();
  this->StatusFrame->SetParent(this);
    
  this->StatusLabel = vtkKWLabel::New();
  this->StatusLabel->SetParent(this->StatusFrame);
  this->StatusImage = vtkKWWidget::New();
  this->StatusImage->SetParent(this->StatusFrame);
  this->StatusImageName = NULL;
  
  this->ProgressFrame = vtkKWWidget::New();
  this->ProgressFrame->SetParent(this->StatusFrame);
  this->ProgressGauge = vtkKWProgressGauge::New();
  this->ProgressGauge->SetParent(this->ProgressFrame);

  this->TrayFrame = vtkKWFrame::New();
  this->TrayFrame->SetParent(this->StatusFrame);

  this->TrayImage = vtkKWImageLabel::New();
  this->TrayImage->SetParent(this->TrayFrame);

  this->Notebook = vtkKWNotebook::New();
  
  this->CommandFunction = vtkKWWindowCommand;

  this->MenuEdit = NULL;
  this->MenuView = NULL;
  this->MenuWindow = NULL;
  this->MenuProperties = NULL;
  this->MenuPropertiesTitle = NULL;
  this->SetMenuPropertiesTitle("Properties");
  this->NumberOfMRUFiles = 0;
  this->RealNumberOfMRUFiles = 0;
  this->PrintTargetDPI = 100;

  this->SupportHelp = 1;

  this->WindowClass = NULL;
  this->SetWindowClass("KitwareWidget");

  this->PromptBeforeClose = 1;

  this->RecentFilesVector = 0;
  this->NumberOfRecentFiles = 5;

  this->ScriptExtension = 0;
  this->ScriptType = 0;
  this->SetScriptExtension(".tcl");
  this->SetScriptType("Tcl");

  this->InExit = 0;

  this->RecentFilesMenuTag =0;

  this->ExitDialogWidget = 0;
}

vtkKWWindow::~vtkKWWindow()
{
  if ( this->RecentFilesVector )
    {
    vtkKWWindowMenuEntry *kc = 0;
    while( this->RecentFilesVector->GetNumberOfItems() )
      {
      kc = 0;
      unsigned long cc = this->RecentFilesVector->GetNumberOfItems()-1;
      if ( this->RecentFilesVector->GetItem(cc, kc) == VTK_OK && kc )
        {
        kc->Delete();
        this->RecentFilesVector->RemoveItem(cc);
        }
      }
    this->RecentFilesVector->Delete();
    }

  this->SetRecentFilesMenuTag(0);
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
  this->TrayFrame->Delete();
  this->TrayImage->Delete();
  
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
  this->SetScriptType(0);

  this->SetMenuPropertiesTitle(0);

}

void vtkKWWindow::DisplayHelp()
{
  this->Application->DisplayHelp(this);
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
  if (this->Application->GetDialogUp())
    {
    this->Script("bell");
    return;
    }
  if (!this->InExit)
    {
    this->InExit = 1;
    }
  else
    {
    return;
    }
  if ( this->ExitDialog() )
    {
    this->PromptBeforeClose = 0;
    this->Application->Exit();
    }
  else
    {
    this->InExit = 0;
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
  this->StatusLabel->SetLabel(text);
}

const char *vtkKWWindow::GetStatusText()
{
  return this->StatusLabel->GetLabel();
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
  if ( !this->GetUseMenuProperties() )
    {
    return 0;
    }

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
    this->Menu->InsertCascade(3, this->MenuPropertiesTitle, 
                              this->MenuProperties, 0);
    }
  else if (this->MenuView || this->MenuEdit)
    {
    this->Menu->InsertCascade(2, this->MenuPropertiesTitle, 
                              this->MenuProperties, 0);
    }
  else
    { 
    this->Menu->InsertCascade(1, this->MenuPropertiesTitle, 
                              this->MenuProperties, 0);
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
  this->StatusImageName = vtkString::Duplicate(interp->result);
  this->CreateStatusImage();
  this->StatusImage->Create(app,"label",
                            "-relief sunken -bd 1 -height 38 -width 132 -fg #ffffff -bg #ffffff");
  this->Script("%s configure -image %s", this->StatusImage->GetWidgetName(),
               this->StatusImageName);
  this->Script("pack %s -side left -padx 2", 
               this->StatusImage->GetWidgetName());
  this->StatusLabel->Create(app,"-relief sunken -padx 3 -bd 1 -font \"Helvetica 10\" -anchor w");
  this->Script("pack %s -side left -padx 2 -expand yes -fill both",
               this->StatusLabel->GetWidgetName());
  this->Script("pack %s -side bottom -fill x -pady 2",
               this->StatusFrame->GetWidgetName());
  this->ProgressFrame->Create(app, "frame", "-relief sunken -borderwidth 2");
  this->ProgressGauge->SetLength(200);
  this->ProgressGauge->SetHeight(30);
  this->ProgressGauge->Create(app, "");

  this->Script("pack %s -side left -padx 2 -fill y", 
               this->ProgressFrame->GetWidgetName());
  this->Script("pack %s -side right -padx 2 -pady 2",
               this->ProgressGauge->GetWidgetName());

  this->TrayFrame->Create(app, 0);
  this->Script("%s configure -borderwidth 0", this->TrayFrame->GetWidgetName());
  this->TrayImage->Create(app, "");
  this->Script("%s configure -relief sunken -bd 2",
               this->TrayImage->GetWidgetName());
  vtkKWIcon *ico = vtkKWIcon::New();
  ico->SetImageData(vtkKWIcon::ICON_SMALLERRORRED);
  this->TrayImage->SetImageData(ico);
  ico->Delete();
  this->TrayImage->SetBind(this, "<Button-1>", "ProcessErrorClick");
  
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
  this->PageMenu->AddRadioButton(0,"100 DPI",rbv,this,"OnPrint 1 0", 0);
  this->PageMenu->AddRadioButton(1,"150 DPI",rbv,this,"OnPrint 1 1", 1);
  this->PageMenu->AddRadioButton(2,"300 DPI",rbv,this,"OnPrint 1 2", 0);
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

  if ( this->UseMenuProperties )
    {
    rbv = 
      this->GetMenuProperties()->CreateRadioButtonVariable(
        this->GetMenuProperties(),"Radio");
    this->GetMenuProperties()->AddRadioButton(0," Hide Properties", 
                                              rbv, this, "HideProperties", 1,
                                              "Hide the properties frame");
    delete [] rbv;
    }
}

void vtkKWWindow::OnPrint(int propagate, int res)
{
  int dpis[] = { 100, 150, 300 };
  this->PrintTargetDPI = dpis[res];
  if ( propagate )
    {
    float dpi = res;
    this->InvokeEvent(vtkKWEvent::ChangePrinterDPIEvent, &dpi);
    }
  else
    {
    char array[][20] = { "100 DPI", "150 DPI", "300 DPI" };
    this->PageMenu->Invoke( this->PageMenu->GetIndex(array[res]) );
    }
}

void vtkKWWindow::ShowProperties()
{
  this->MiddleFrame->SetFrame1MinimumSize(360);
  //this->MiddleFrame->SetFrame1Size(360);
}

void vtkKWWindow::HideProperties()
{
  // make sure the variable is set, otherwise set it
  if ( this->UseMenuProperties )
    {
    this->GetMenuProperties()->CheckRadioButton(
      this->GetMenuProperties(),"Radio",0);
    }
  this->MiddleFrame->SetFrame1MinimumSize(0);
  this->MiddleFrame->SetFrame1Size(0);
}

void vtkKWWindow::InstallMenu(vtkKWMenu* menu)
{ 
  this->Script("%s configure -menu %s", this->GetWidgetName(),
               menu->GetWidgetName());  
}

void vtkKWWindow::UnRegister(vtkObjectBase *o)
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

  this->Script("tk_getOpenFile -title \"Load Script\" -filetypes {{{%s Script} {%s}}}", this->ScriptType, this->ScriptExtension);
  path = vtkString::Duplicate(this->Application->GetMainInterp()->result);
  if (vtkString::Length(path))
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
  this->Script("set InitialWindow %s", this->GetTclName());
  this->Application->LoadScript(path);
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
  
#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION >= 4)
  Tk_PhotoPutBlock(photo, &block, 0, 0, block.width, block.height, TK_PHOTO_COMPOSITE_SET);
#else
  Tk_PhotoPutBlock(photo, &block, 0, 0, block.width, block.height);
#endif
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

  for (i = 0; i < this->NumberOfRecentFiles; i++)
    {
    sprintf(KeyNameP, "File%d", i);
    sprintf(CmdNameP, "File%dCmd", i);
    this->GetApplication()->DeleteRegisteryValue( 1, "MRU", KeyNameP );
    this->GetApplication()->DeleteRegisteryValue( 1, "MRU", CmdNameP );    
    if ( this->RecentFilesVector )
      {
      vtkKWWindowMenuEntry *vp = 0;
      if ( this->RecentFilesVector->GetItem(i, vp) == VTK_OK && vp )
        {
        this->GetApplication()->SetRegisteryValue(
          1, "MRU", KeyNameP, vp->GetFullFile());
        this->GetApplication()->SetRegisteryValue(
          1, "MRU", CmdNameP, vp->GetCommand());
        }
      }
    }
}

void vtkKWWindow::AddRecentFilesToMenu(char *menuEntry, vtkKWObject *target)
{
  char KeyNameP[10];
  char CmdNameP[10];
  int i = 0;
  char File[1024];
  char Cmd[1024];
  
  char *newMenuEntry = 0;
  if ( menuEntry )
    {
    newMenuEntry = new char[ vtkString::Length(menuEntry)+1 ];
    while ( menuEntry[i] )
      {
      if ( menuEntry[i] == '\\' )
        {
        newMenuEntry[i] = '/';
        }
      else
        {
        newMenuEntry[i] = menuEntry[i];
        }      
      i++;
      }
    newMenuEntry[i] = 0;
    }

  if ( newMenuEntry )
    {
    this->SetRecentFilesMenuTag(newMenuEntry);
    }
  else
    {
    this->SetRecentFilesMenuTag("Close");
    }
  for (i = this->NumberOfRecentFiles-1; i >=0; i--)
    {
    sprintf(KeyNameP, "File%d", i);
    sprintf(CmdNameP, "File%dCmd", i);
    if ( this->GetApplication()->GetRegisteryValue(1, "MRU", KeyNameP, File) )
      {
      if ( this->GetApplication()->GetRegisteryValue(1, "MRU", CmdNameP, Cmd) )
        {
        if (vtkString::Length(File) > 1)
          {
          this->InsertRecentFileToMenu(File, target, Cmd);
          }
        }
      }
    }
  this->UpdateRecentMenu(NULL);
  delete [] newMenuEntry;
}

void vtkKWWindow::AddRecentFile(char *key, char *name, vtkKWObject *target,
                                const char *command)
{  
  const char* filename = this->Application->ExpandFileName(name);
  this->InsertRecentFileToMenu(filename, target, command);
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
  this->ExtractRevision(os,"$Revision: 1.106 $");
}

int vtkKWWindow::ExitDialog()
{
  this->Application->SetBalloonHelpWidget(0);
  if ( this->ExitDialogWidget )
    {
    return 1;
    }
  ostrstream title;
  title << "Exit " << this->GetApplication()->GetApplicationName() << ends;
  char* ttl = title.str();
  ostrstream str;
  str << "Are you sure you want to exit " 
      << this->GetApplication()->GetApplicationName() << "?" << ends;
  char* msg = str.str();
  
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  this->ExitDialogWidget = dlg2;
  dlg2->SetStyleToYesNo();
  dlg2->SetMasterWindow(this);
  dlg2->SetOptions(
     vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::RememberYes |
     vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
  dlg2->SetDialogName("ExitApplication");
  dlg2->Create(this->GetApplication(),"");
  dlg2->SetText( msg );
  dlg2->SetTitle( ttl );
  int ret = dlg2->Invoke();
  this->ExitDialogWidget = 0;
  dlg2->Delete();

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
      this->GetMenuFile()->GetIndex(this->GetRecentFilesMenuTag()) - 1);
    }
  if ( this->RealNumberOfMRUFiles )
    {
    this->GetMenuFile()->DeleteMenuItem(
      this->GetMenuFile()->GetIndex(this->GetRecentFilesMenuTag()) - 1);
    }
  this->RealNumberOfMRUFiles = 0;
  if ( this->RecentFilesVector )
    {
    for ( cc = 0; static_cast<unsigned int>(cc)<this->NumberOfRecentFiles; 
          cc++ ) 
      {
      vtkKWWindowMenuEntry *kc = 0;
      if ( this->RecentFilesVector->GetItem(cc, kc) == VTK_OK && kc )
        {
        kc->InsertToMenu(cc, this->GetRecentFilesMenuTag(),
                         this->GetMenuFile());
        this->RealNumberOfMRUFiles ++;
        }
      }
    if ( this->NumberOfRecentFiles )
      {
      this->GetMenuFile()->InsertSeparator(
        this->GetMenuFile()->GetIndex(this->GetRecentFilesMenuTag()));    
      }
    }
  //this->PrintRecentFiles();
}

void vtkKWWindow::InsertRecentFileToMenu(const char *filename, 
                                         vtkKWObject *target, 
                                         const char *command)
{
  //this->PrintRecentFiles();
  int flen = vtkString::Length(filename);
  char *file = new char [flen + 3];
  if ( flen <= 40 )
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
    int flen = vtkString::Length(filename);
    for(ii=flen; ii >= (flen-25); ii--)
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

  if ( !this->RecentFilesVector )
    {
    this->RecentFilesVector = vtkVector<vtkKWWindowMenuEntry*>::New();
    }

  // Find current one
  vtkKWWindowMenuEntry *recent = 0;
  vtkKWWindowMenuEntry *kc = 0;
  vtkIdType cc;
  for ( cc = 0; cc < this->RecentFilesVector->GetNumberOfItems(); cc ++ )
    {
    kc = 0;
    if ( this->RecentFilesVector->GetItem(cc, kc) == VTK_OK && kc &&
         kc->Same( file, filename, target, command ) )
      {
      recent = kc;
      // delete it from array
      this->RecentFilesVector->RemoveItem(cc);
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
  this->RecentFilesVector->PrependItem( recent );
  while ( this->RecentFilesVector->GetNumberOfItems() > 
          static_cast<vtkIdType>(this->NumberOfRecentFiles) )
    {
    kc = 0;
    if ( this->RecentFilesVector->GetItem(this->NumberOfRecentFiles, kc) ==
         VTK_OK && kc )
      {
      kc->Delete();
      this->RecentFilesVector->RemoveItem(this->NumberOfRecentFiles);
      }
    }

  delete [] file;
  //this->PrintRecentFiles();
}

float vtkKWWindow::GetFloatRegisteryValue(int level, const char* subkey, 
                                          const char* key)
{
  return this->GetApplication()->GetFloatRegisteryValue(level, subkey, key);
}

int vtkKWWindow::GetIntRegisteryValue(int level, const char* subkey, 
                                      const char* key)
{
  return this->GetApplication()->GetIntRegisteryValue(level, subkey, key);
}
void vtkKWWindow::SaveLastPath(vtkKWLoadSaveDialog *dialog, const char* key)
{
  //  "OpenDirectory"
  if ( dialog->GetLastPath() )
    {
    this->GetApplication()->SetRegisteryValue(
      1, "RunTime", key, dialog->GetLastPath());
    }
}

void vtkKWWindow::RetrieveLastPath(vtkKWLoadSaveDialog *dialog, const char* key)
{
  char buffer[1024];
  if ( this->GetApplication()->GetRegisteryValue(1, "RunTime", key, buffer) )
    {
    if ( *buffer )
      {
      dialog->SetLastPath( buffer );
      }  
    }
}

void vtkKWWindow::SaveColor(int level, const char* key, float rgb[3])
{
  this->GetApplication()->SetRegisteryValue(
    level, "RunTime", key, "Color: %f %f %f", rgb[0], rgb[1], rgb[2]);
}

void vtkKWWindow::RetrieveColor(int level, const char* key, float rgb[3])
{
  char buffer[1024];
  rgb[0] = -1;
  rgb[1] = -1;
  rgb[2] = -1;

  if ( this->GetApplication()->GetRegisteryValue(
         level, "RunTime", key, buffer) )
    {
    if ( *buffer )
      {      
      sscanf(buffer, "Color: %f %f %f", rgb, rgb+1, rgb+2);
      }
    }
}

int vtkKWWindow::BooleanRegisteryCheck(int level, const char* key, 
                                       const char* trueval)
{
  return this->GetApplication()->BooleanRegisteryCheck(level, key, trueval);
}


//----------------------------------------------------------------------------
void vtkKWWindow::WarningMessage(const char* message)
{
  vtkKWMessageDialog::PopupMessage(
    this->GetApplication(), this, "VTK Warning",
    message, vtkKWMessageDialog::WarningIcon);
  this->SetErrorIcon(2);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SetErrorIcon(int s)
{
  if (s) 
    {
    this->Script("pack %s -side left -ipady 0 -padx 2 -fill both", 
               this->TrayFrame->GetWidgetName());
    this->Script("pack %s -fill both -ipadx 4 -expand yes", 
                 this->TrayImage->GetWidgetName());
    if ( s > 1 )
      {
      //cout << "Configure with color red" << endl;
      vtkKWIcon *ico = vtkKWIcon::New();
      ico->SetImageData(vtkKWIcon::ICON_SMALLERRORRED);
      this->TrayImage->SetImageData(ico);
      ico->Delete();
      }
    else
      {
      vtkKWIcon *ico = vtkKWIcon::New();
      ico->SetImageData(vtkKWIcon::ICON_SMALLERROR);
      this->TrayImage->SetImageData(ico);
      ico->Delete();
      }
    }
  else
    {
    this->Script("pack forget %s", this->TrayImage->GetWidgetName());
    this->Script("pack forget %s", this->TrayFrame->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::ErrorMessage(const char* message)
{
  //cout << message << endl;
  vtkKWMessageDialog::PopupMessage(
    this->GetApplication(), this, "VTK Error",
    message, vtkKWMessageDialog::ErrorIcon);
  this->SetErrorIcon(2);
}

//----------------------------------------------------------------------------
void vtkKWWindow::PrintRecentFiles()
{
  cout << "PrintRecentFiles" << endl;
  if ( !this->RecentFilesVector && this->NumberOfRecentFiles )
    {
    cout << "Problem with the number: " << this->NumberOfRecentFiles
         << endl;
    }
  if ( !this->RecentFilesVector )
    {
    return;
    }
  unsigned long cc;
  for ( cc = 0; cc< this->NumberOfRecentFiles; cc ++ )
    {
    vtkKWWindowMenuEntry *kc = 0;
    if ( this->RecentFilesVector->GetItem(cc, kc) == VTK_OK )
      {
      cout << "Item: " << kc << endl;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::ProcessErrorClick()
{
  this->SetErrorIcon(1);
}

//----------------------------------------------------------------------------
void vtkKWWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Menu: " << this->GetMenu() << endl;
  os << indent << "MenuFile: " << this->GetMenuFile() << endl;
  os << indent << "MenuPropertiesTitle: " << this->GetMenuPropertiesTitle() 
     << endl;
  os << indent << "Notebook: " << this->GetNotebook() << endl;
  os << indent << "NumberOfRecentFiles: " << this->GetNumberOfRecentFiles() 
     << endl;
  os << indent << "PrintTargetDPI: " << this->GetPrintTargetDPI() << endl;
  os << indent << "ProgressGauge: " << this->GetProgressGauge() << endl;
  os << indent << "PromptBeforeClose: " << this->GetPromptBeforeClose() 
     << endl;
  os << indent << "PropertiesParent: " << this->GetPropertiesParent() << endl;
  os << indent << "ScriptExtension: " << this->GetScriptExtension() << endl;
  os << indent << "ScriptType: " << this->GetScriptType() << endl;
  os << indent << "SelectedView: " << this->GetSelectedView() << endl;
  os << indent << "SupportHelp: " << this->GetSupportHelp() << endl;
  os << indent << "ToolbarFrame: " << this->GetToolbarFrame() << endl;
  os << indent << "UseMenuProperties: " << this->GetUseMenuProperties() 
     << endl;
  os << indent << "ViewFrame: " << this->GetViewFrame() << endl;
  os << indent << "WindowClass: " << this->GetWindowClass() << endl;  
}
