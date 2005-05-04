/*=========================================================================

  Module:    vtkKWApplication.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"

#include "vtkKWApplicationSettingsInterface.h"
#include "vtkKWBWidgets.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWObject.h"
#include "vtkKWRegistryHelper.h"
#include "vtkKWBalloonHelpManager.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWidgetsConfigure.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkKWText.h"
#include "vtkTclUtil.h"

#include <stdarg.h>

#include <kwsys/SystemTools.hxx>
#include <kwsys/stl/vector>
#include <kwsys/stl/algorithm>

static Tcl_Interp *Et_Interp = 0;

#ifdef _WIN32
#include "vtkWindows.h"
#include <process.h>
#include <mapi.h>
#include <htmlhelp.h>
#include "Utilities/vtkKWSetApplicationIconTclCommand.h"
#endif

// I need those two Tcl functions. They usually are declared in tclIntDecls.h,
// but Unix build do not have access to VTK's tkInternals include path.
// Since the signature has not changed for years (at least since 8.2),
// let's just prototype them.

EXTERN Tcl_Obj* TclGetLibraryPath _ANSI_ARGS_((void));
EXTERN void TclSetLibraryPath _ANSI_ARGS_((Tcl_Obj * pathPtr));

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWApplication );
vtkCxxRevisionMacro(vtkKWApplication, "1.200");

extern "C" int Vtkcommontcl_Init(Tcl_Interp *interp);
extern "C" int Kwwidgetstcl_Init(Tcl_Interp *interp);

int vtkKWApplicationCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);

//----------------------------------------------------------------------------
class vtkKWApplicationInternals
{
public:
  typedef kwsys_stl::vector<vtkKWWindow*> WindowsContainer;
  typedef kwsys_stl::vector<vtkKWWindow*>::iterator WindowsContainerIterator;

  WindowsContainer Windows;
};

//----------------------------------------------------------------------------
vtkKWApplication::vtkKWApplication()
{
  this->CommandFunction = vtkKWApplicationCommand;
  
  // Instantiate the PIMPL Encapsulation for STL containers

  this->Internals = new vtkKWApplicationInternals;

  // Application name and version

  this->MajorVersion = 1;
  this->MinorVersion = 0;
  this->Name = 
    kwsys::SystemTools::DuplicateString("Sample Application");
  this->VersionName = 
    kwsys::SystemTools::DuplicateString("SampleApplication10");
  this->ReleaseName = 
    kwsys::SystemTools::DuplicateString("unknown");
  this->PrettyName = NULL;

  // Limited Edition Mode name

  this->LimitedEditionMode = 0;
  this->LimitedEditionModeName = NULL;
  char name[1024];
  sprintf(name, "%s Limited Edition", this->Name);
  this->SetLimitedEditionModeName(name);

  this->HelpDialogStartingPage = 
    kwsys::SystemTools::DuplicateString("Introduction.htm");

  this->InstallationDirectory = NULL;
  this->EmailFeedbackAddress  = NULL;

  this->InExit     = 0;
  this->ExitStatus = 0;
  this->ExitAfterLoadScript = 0;
  this->PromptBeforeExit = 1;

  this->DialogUp = 0;

  this->SaveWindowGeometry = 1;

  this->RegistryHelper = NULL;
  this->RegistryLevel = 10;

  this->BalloonHelpManager = NULL;

  this->CharacterEncoding = VTK_ENCODING_UNKNOWN;
  this->SetCharacterEncoding(VTK_ENCODING_ISO_8859_1);

  // About dialog

  this->AboutDialog      = NULL;
  this->AboutDialogImage = NULL;
  this->AboutRuntimeInfo = NULL;

  // Splashscreen

  this->SplashScreen = NULL;
  this->HasSplashScreen = 0;
  this->ShowSplashScreen = 1;

  // Setup Tcl

  this->MainInterp = Et_Interp;
  if (!this->MainInterp)
    {
    vtkErrorMacro(
      "Interpreter not set. This probably means that Tcl was not "
      "initialized properly...");
    return;
    }

  // As a convenience, set the 'Application' Tcl variable to ourself

  this->Script("set Application %s",this->GetTclName());
}

//----------------------------------------------------------------------------
vtkKWApplication::~vtkKWApplication()
{
  this->PrepareForDelete();

  delete this->Internals;
  this->Internals = NULL;

  this->MainInterp = NULL;
  vtkObjectFactory::UnRegisterAllFactories();

  this->SetLimitedEditionModeName(NULL);
  this->SetName(NULL);
  this->SetVersionName(NULL);
  this->SetReleaseName(NULL);
  this->SetPrettyName(NULL);
  this->SetInstallationDirectory(NULL);
  this->SetEmailFeedbackAddress(NULL);
  this->SetHelpDialogStartingPage(NULL);

  if (this->RegistryHelper )
    {
    this->RegistryHelper->Delete();
    this->RegistryHelper = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWApplication::PrepareForDelete()
{
  if (this->AboutDialogImage)
    {
    this->AboutDialogImage->Delete();
    this->AboutDialogImage = NULL;
    }

  if (this->AboutRuntimeInfo)
    {
    this->AboutRuntimeInfo->Delete();
    this->AboutRuntimeInfo = NULL;
    }
    
  if (this->AboutDialog)
    {
    this->AboutDialog->Delete();
    this->AboutDialog = NULL;
    }

  if (this->SplashScreen)
    {
    this->SplashScreen->Delete();
    this->SplashScreen = NULL;
    }

  if (this->BalloonHelpManager )
    {
    this->BalloonHelpManager->Delete();
    this->BalloonHelpManager = NULL;
    }
}
    
//----------------------------------------------------------------------------
void vtkKWApplication::SetApplication(vtkKWApplication*) 
{ 
  vtkErrorMacro( << "Do not set the Application on an Application" << endl); 
}

//----------------------------------------------------------------------------
int vtkKWApplication::AddWindow(vtkKWWindow *win)
{
  if (this->Internals)
    {
    this->Internals->Windows.push_back(win);
    win->Register(this);
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWApplication::RemoveWindow(vtkKWWindow *win)
{
  // If this is the last window, go straight to Exit.
  // This will give the app a chance to ask the user for confirmation.
  // It is likely that this method has been called by vtkKWWindow::Close().
  // In that case, Exit() will call vtkKWWindow::Close() *again*, and we
  // will be back here, but this won't infinite loop since this->Exit()
  // will return false (as we are already "exiting", check this->InExit)

  if (this->GetNumberOfWindows() <= 1 && this->Exit())
    {
    return 1;
    }

  if (this->Internals && win)
    {
    win->PrepareForDelete();

    vtkKWApplicationInternals::WindowsContainerIterator it = 
      kwsys_stl::find(this->Internals->Windows.begin(),
                      this->Internals->Windows.end(),
                      win);
    win->UnRegister(this);
    this->Internals->Windows.erase(it);
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkKWWindow* vtkKWApplication::GetNthWindow(int rank)
{
  if (this->Internals && rank >= 0 && rank < this->GetNumberOfWindows())
    {
    return this->Internals->Windows[rank];
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetNumberOfWindows()
{
  if (this->Internals)
    {
    return this->Internals->Windows.size();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWApplication::Exit()
{
  // Avoid a recursive exit.

  if (this->InExit)
    {
    return 0;
    }

  // If a dialog is still up, complain and bail

  if (this->GetDialogUp())
    {
    this->Script("bell");
    return 0;
    }

  // Prompt confirmation if needed

  if (this->PromptBeforeExit && !this->DisplayExitDialog(NULL))
    {
    return 0;
    }

  this->InExit = 1;

  this->PrepareForDelete();

  // Close all windows
  // This loop might be a little dangerous if a window never closes...
  // We could loop over the given number of window, and test if there
  // are still windows left at the end (= error)

  while (this->GetNumberOfWindows())
    {
    vtkKWWindow *win = this->GetNthWindow(0);
    if (win)
      {
      win->SetPromptBeforeClose(0);
      win->Close();
      }
    }

  return 1;
}
    
//----------------------------------------------------------------------------
Tcl_Interp *vtkKWApplication::InitializeTcl(int argc, 
                                            char *argv[], 
                                            ostream *err)
{
  Tcl_Interp *interp;
  char *args;
  char buf[100];

  (void)err;

  // The call to Tcl_FindExecutable has to be executed *now*, it does more 
  // than just finding the executable (for ex:, it will set variables 
  // depending on the value of TCL_LIBRARY, TK_LIBRARY)

  Tcl_FindExecutable(argv[0]);

  // Create the interpreter

  interp = Tcl_CreateInterp();
  args = Tcl_Merge(argc - 1, argv + 1);
  Tcl_SetVar(interp, (char *)"argv", args, TCL_GLOBAL_ONLY);
  ckfree(args);
  sprintf(buf, "%d", argc-1);
  Tcl_SetVar(interp, (char *)"argc", buf, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, (char *)"argv0", argv[0], TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, (char *)"tcl_interactive", 
             (char *)"0", TCL_GLOBAL_ONLY);

  // Find the path to our internal Tcl/Tk support library/packages
  // if we are not using the installed Tcl/Tk (i.e., if the support
  // file were copied to the build/install dir)
  // Sets the path to the Tcl and Tk library manually
  
#ifdef VTK_TCL_TK_COPY_SUPPORT_LIBRARY

  int has_tcllibpath_env = getenv("TCL_LIBRARY") ? 1 : 0;
  int has_tklibpath_env = getenv("TK_LIBRARY") ? 1 : 0;
  if (!has_tcllibpath_env || !has_tklibpath_env)
    {
    const char *nameofexec = Tcl_GetNameOfExecutable();
    if (nameofexec && kwsys::SystemTools::FileExists(nameofexec))
      {
      char dir_unix[1024], buffer[1024];
      kwsys_stl::string dir = kwsys::SystemTools::GetFilenamePath(nameofexec);
      kwsys::SystemTools::ConvertToUnixSlashes(dir);
      strcpy(dir_unix, dir.c_str());

      // Installed KW application, otherwise build tree/windows
      sprintf(buffer, "%s/..%s/TclTk", dir_unix, KW_INSTALL_LIB_DIR);
      int exists = kwsys::SystemTools::FileExists(buffer);
      if (!exists)
        {
        sprintf(buffer, "%s/TclTk", dir_unix);
        exists = kwsys::SystemTools::FileExists(buffer);
        }
      kwsys_stl::string collapsed = 
        kwsys::SystemTools::CollapseFullPath(buffer);
      sprintf(buffer, collapsed.c_str());
      if (exists)
        {
        // Also prepend our Tcl Tk lib path to the library paths
        // This *is* mandatory if we want encodings files to be found, as they
        // are searched by browsing TclGetLibraryPath().
        // (nope, updating the Tcl tcl_libPath var won't do the trick)
        
        Tcl_Obj *new_libpath = Tcl_NewObj();
        
        // Tcl lib path
        
        if (!has_tcllibpath_env)
        {
        char tcl_library[1024] = "";
        sprintf(tcl_library, "%s/lib/tcl%s", buffer, TCL_VERSION);
        if (kwsys::SystemTools::FileExists(tcl_library))
          {
          if (!Tcl_SetVar(interp, "tcl_library", tcl_library, 
                          TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG))
            {
            if (err)
              {
              *err << "Tcl_SetVar error: " << Tcl_GetStringResult(interp) 
                   << endl;
              }
            return NULL;
            }
          Tcl_Obj *obj = Tcl_NewStringObj(tcl_library, -1);
          if (obj && 
              !Tcl_ListObjAppendElement(interp, new_libpath, obj) != TCL_OK &&
              err)
            {
            *err << "Tcl_ListObjAppendElement error: " 
                 << Tcl_GetStringResult(interp) << endl;
            }
          }
        }
  
        // Tk lib path

        if (!has_tklibpath_env)
          {
          char tk_library[1024] = "";
          sprintf(tk_library, "%s/lib/tk%s", buffer, TK_VERSION);
          if (kwsys::SystemTools::FileExists(tk_library))
            {
            if (!Tcl_SetVar(interp, "tk_library", tk_library, 
                            TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG))
              {
              if (err)
                {
                *err << "Tcl_SetVar error: " << Tcl_GetStringResult(interp) 
                     << endl;
                }
              return NULL;
              }
            Tcl_Obj *obj = Tcl_NewStringObj(tk_library, -1);
            if (obj && 
                !Tcl_ListObjAppendElement(interp, new_libpath, obj) != TCL_OK 
                && err)
              {
              *err << "Tcl_ListObjAppendElement error: " 
                   << Tcl_GetStringResult(interp) << endl;
              }
            }
          }

        TclSetLibraryPath(new_libpath);
        }
      }
    }

#endif

  // Init Tcl

  Et_Interp = interp;

  int status;

  status = Tcl_Init(interp);
  if (status != TCL_OK)
    {
    if (err)
      {
      *err << "Tcl_Init error: " << Tcl_GetStringResult(interp) << endl;
      }
    return NULL;
    }

  // Init Tk

  status = Tk_Init(interp);
  if (status != TCL_OK)
    {
    if (err)
      {
      *err << "Tk_Init error: " << Tcl_GetStringResult(interp) << endl;
      }
    return NULL;
    }
  
  Tcl_StaticPackage(interp, (char *)"Tk", Tk_Init, 0);
    
  // create the SetApplicationIcon command
#ifdef _WIN32
  vtkKWSetApplicationIconTclCommand_DoInit(interp);
#endif

  // Initialize VTK

  if (Vtkcommontcl_Init(interp) != TCL_OK) 
    {
    if (err)
        {
        *err << "Vtkcommontcl_Init error: " 
             << Tcl_GetStringResult(interp) << endl;
        }
    return NULL;
    }

  // Initialize Widgets

  Kwwidgetstcl_Init(interp);

  vtkKWBWidgets::Initialize(interp);

  return interp;
}

//----------------------------------------------------------------------------
void vtkKWApplication::Start()
{ 
  int i;
  
  // look at Tcl for any args
  
  int argc = atoi(this->Script("set argc")) + 1;
  char **argv = new char *[argc];
  argv[0] = NULL;
  for (i = 1; i < argc; i++)
    {
    argv[i] = strdup(this->Script("lindex $argv %d",i-1));
    }
  this->Start(argc,argv);
  
  for (i = 0; i < argc; i++)
    {
    if (argv[i])
      {
      free(argv[i]);
      }
    }
  delete [] argv;
}

//----------------------------------------------------------------------------
void vtkKWApplication::Start(int /*argc*/, char ** /*argv*/)
{ 
  while (this->GetNumberOfWindows())
    {
    this->DoOneTclEvent();
    }
}

//----------------------------------------------------------------------------
void vtkKWApplication::GetApplicationSettingsFromRegistry()
{ 
  // Show balloon help ?

  if (this->HasRegistryValue(
    2, "RunTime", VTK_KW_SHOW_TOOLTIPS_REG_KEY))
    {
    this->GetBalloonHelpManager()->SetShow(
      this->GetIntRegistryValue(
        2, "RunTime", VTK_KW_SHOW_TOOLTIPS_REG_KEY));
    }

  // Save window geometry ?

  if (this->HasRegistryValue(
    2, "Geometry", VTK_KW_SAVE_WINDOW_GEOMETRY_REG_KEY))
    {
    this->SaveWindowGeometry = this->GetIntRegistryValue(
      2, "Geometry", VTK_KW_SAVE_WINDOW_GEOMETRY_REG_KEY);
    }

  // Show splash screen ?

  if (this->HasRegistryValue(
    2, "RunTime", VTK_KW_SHOW_SPLASH_SCREEN_REG_KEY))
    {
    this->ShowSplashScreen = this->GetIntRegistryValue(
      2, "RunTime", VTK_KW_SHOW_SPLASH_SCREEN_REG_KEY);
    }

  if (this->RegistryLevel <= 0)
    {
    this->ShowSplashScreen = 0;
    this->SaveWindowGeometry = 0;
    }
}

//----------------------------------------------------------------------------
void vtkKWApplication::DoOneTclEvent()
{
  Tcl_DoOneEvent(0);
}

//----------------------------------------------------------------------------
int vtkKWApplication::DisplayExitDialog(vtkKWWindow *master)
{
  kwsys_stl::string title = "Exit ";
  title += this->GetPrettyName();

  kwsys_stl::string msg = "Are you sure you want to exit ";
  msg += this->GetPrettyName();
  msg += "?";
  
  vtkKWMessageDialog *dialog = vtkKWMessageDialog::New();
  dialog->SetStyleToYesNo();
  dialog->SetMasterWindow(master);
  dialog->SetOptions(
    vtkKWMessageDialog::QuestionIcon | 
    vtkKWMessageDialog::RememberYes |
    vtkKWMessageDialog::Beep | 
    vtkKWMessageDialog::YesDefault);
  dialog->SetDialogName(VTK_KW_EXIT_DIALOG_NAME);
  dialog->Create(this, NULL);
  dialog->SetText(msg.c_str());
  dialog->SetTitle(title.c_str());

  int ret = dialog->Invoke();
  dialog->Delete();

  // This UI interface usually displays a checkbox offering the choice
  // to prompt for exit or not. Update that UI.

  for (int i = 0; i < this->GetNumberOfWindows(); i++)
    {
    this->GetNthWindow(i)->Update();
    }

  return ret;
}

//----------------------------------------------------------------------------
void vtkKWApplication::DisplayHelpDialog(vtkKWWindow* master)
{
#ifdef _WIN32
  ostrstream temp;
  if (this->InstallationDirectory)
    {
    temp << this->InstallationDirectory << "/";
    }
  temp << this->Name << ".chm";
  if (this->HelpDialogStartingPage)
    {
    temp << "::/" << this->HelpDialogStartingPage;
    }
  temp << ends;
  
  if (!HtmlHelp(NULL, temp.str(), HH_DISPLAY_TOPIC, 0))
    {
    vtkKWMessageDialog::PopupMessage(
      this, master,
      "Loading Help Error",
      "Help file cannot be displayed. This can be a result of "
      "the program being wrongly installed or help file being "
      "corrupted. Please reinstall this program.", 
      vtkKWMessageDialog::ErrorIcon);
    }

  temp.rdbuf()->freeze(0);

#else
  vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
  dlg->SetMasterWindow(master);
  dlg->Create(this,"");
  dlg->SetText(
    "HTML help is included in the help subdirectory of\n"
    "this application. You can view this help using a\n"
    "standard web browser by loading the Help.htm file.");
  dlg->Invoke();  
  dlg->Delete();
#endif
}

//----------------------------------------------------------------------------
void vtkKWApplication::DisplayAboutDialog(vtkKWWindow* master)
{
  if (this->InExit)
    {
    return;
    }

  if (!this->AboutDialog)
    {
    this->AboutDialog = vtkKWMessageDialog::New();
    }

  if (!this->AboutDialog->IsCreated())
    {
    this->AboutDialog->SetMasterWindow(master);
    this->AboutDialog->HideDecorationOn();
    this->AboutDialog->Create(this, "-bd 1 -relief solid");
    }

  this->ConfigureAboutDialog();

  this->AboutDialog->Invoke();
}

//----------------------------------------------------------------------------
void vtkKWApplication::ConfigureAboutDialog()
{
  if (this->HasSplashScreen)
    {
    this->CreateSplashScreen();
    const char *img_name = this->SplashScreen->GetImageName();
    if (img_name)
      {
      if (!this->AboutDialogImage)
        {
        this->AboutDialogImage = vtkKWLabel::New();
        }
      if (!this->AboutDialogImage->IsCreated())
        {
        this->AboutDialogImage->SetParent(this->AboutDialog->GetTopFrame());
        this->AboutDialogImage->Create(this, 0);
        }
      this->Script("%s config -image {%s}",
                   this->AboutDialogImage->GetWidgetName(), img_name);
      this->Script("pack %s -side top", 
                   this->AboutDialogImage->GetWidgetName());
      int w = vtkKWTkUtilities::GetPhotoWidth(this->MainInterp, img_name);
      int h = vtkKWTkUtilities::GetPhotoHeight(this->MainInterp, img_name);
      this->AboutDialog->GetTopFrame()->SetWidth(w);
      this->AboutDialog->GetTopFrame()->SetHeight(h);
      if (w > this->AboutDialog->GetTextWidth())
        {
        this->AboutDialog->SetTextWidth(w);
        }
      this->Script(
        "pack %s -side bottom",  // -expand 1 -fill both
        this->AboutDialog->GetMessageDialogFrame()->GetWidgetName());
      }
    }

  if (!this->AboutRuntimeInfo)
    {
    this->AboutRuntimeInfo = vtkKWText::New();
    }
  if (!this->AboutRuntimeInfo->IsCreated())
    {
    this->AboutRuntimeInfo->SetParent(this->AboutDialog->GetBottomFrame());
    this->AboutRuntimeInfo->Create(this, "-setgrid true");
    this->AboutRuntimeInfo->SetWidth(60);
    this->AboutRuntimeInfo->SetHeight(8);
    this->AboutRuntimeInfo->SetWrapToWord();
    this->AboutRuntimeInfo->EditableTextOff();
    this->AboutRuntimeInfo->UseVerticalScrollbarOn();
    double r, g, b;
    this->AboutRuntimeInfo->GetTextWidget()->GetParent()
      ->GetBackgroundColor(&r, &g, &b);
    this->AboutRuntimeInfo->GetTextWidget()->SetBackgroundColor(r, g, b);
    this->Script("pack %s -side top -padx 2 -expand 1 -fill both",
                 this->AboutRuntimeInfo->GetWidgetName());
    }

  ostrstream title;
  title << "About " << this->GetPrettyName() << ends;
  this->AboutDialog->SetTitle(title.str());
  title.rdbuf()->freeze(0);

  ostrstream str;
  this->AddAboutText(str);
  str << endl;
  this->AddAboutCopyrights(str);
  str << ends;
  this->AboutRuntimeInfo->SetValue( str.str() );
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWApplication::AddAboutText(ostream &os)
{
  os << this->GetPrettyName();
  const char *app_ver_name = this->GetVersionName();
  const char *app_rel_name = this->GetReleaseName();
  if ((app_ver_name && *app_ver_name) || (app_rel_name && *app_rel_name))
    {
    os << " (";
    if (app_ver_name && *app_ver_name)
      {
      os << app_ver_name;
      if (app_rel_name && *app_rel_name)
        {
        os << " ";
        }
      }
    if (app_rel_name && *app_rel_name)
      {
      os << app_rel_name;
      }
    os << ")";
    }
  os << endl;
}

//----------------------------------------------------------------------------
void vtkKWApplication::AddAboutCopyrights(ostream &os)
{
  os << "Tcl/Tk" << endl
     << "  - Copyright (c) 1989-1994 The Regents of the University of "
     << "California." << endl
     << "  - Copyright (c) 1994 The Australian National University." << endl
     << "  - Copyright (c) 1994-1998 Sun Microsystems, Inc." << endl
     << "  - Copyright (c) 1998-2000 Ajuba Solutions." << endl;
}

//----------------------------------------------------------------------------
vtkKWSplashScreen *vtkKWApplication::GetSplashScreen()
{
  if (!this->SplashScreen)
    {
    this->SplashScreen = vtkKWSplashScreen::New();
    }
  return this->SplashScreen;
}

//----------------------------------------------------------------------------
vtkKWRegistryHelper *vtkKWApplication::GetRegistryHelper()
{
  if (!this->RegistryHelper)
    {
    this->RegistryHelper = vtkKWRegistryHelper::New();
    }
  return this->RegistryHelper;
}

//----------------------------------------------------------------------------
vtkKWBalloonHelpManager *vtkKWApplication::GetBalloonHelpManager()
{
  if (!this->BalloonHelpManager)
    {
    this->BalloonHelpManager = vtkKWBalloonHelpManager::New();
    this->BalloonHelpManager->SetApplication(this);
    }
  return this->BalloonHelpManager;
}

//----------------------------------------------------------------------------
int vtkKWApplication::SetRegistryValue(int level, const char* subkey, 
                                       const char* key, 
                                       const char* format, ...)
{
  if (this->GetRegistryLevel() < 0 || this->GetRegistryLevel() < level)
    {
    return 0;
    }
  int res = 0;
  char buffer[REG_KEY_NAME_SIZE_MAX];
  char value[REG_KEY_VALUE_SIZE_MAX];
  sprintf(buffer, "%s\\%s", this->GetApplication()->GetVersionName(), subkey);
  va_list var_args;
  va_start(var_args, format);
  vsprintf(value, format, var_args);
  va_end(var_args);
  
  vtkKWRegistryHelper *reg = this->GetRegistryHelper();
  reg->SetTopLevel(this->GetName());
  res = reg->SetValue(buffer, key, value);
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetRegistryValue(int level, const char* subkey, 
                                       const char* key, char* value)
{
  if (!this->GetApplication() || 
      this->GetRegistryLevel() < 0 || this->GetRegistryLevel() < level)
    {
    return 0;
    }
  int res = 0;
  char buff[REG_KEY_VALUE_SIZE_MAX];
  char buffer[REG_KEY_NAME_SIZE_MAX];
  sprintf(buffer, "%s\\%s", this->GetVersionName(), subkey);

  vtkKWRegistryHelper *reg = this->GetRegistryHelper();
  reg->SetTopLevel(this->GetName());
  res = reg->ReadValue(buffer, key, buff);
  if (*buff && value)
    {
    *value = 0;
    strcpy(value, buff);
    }  
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::DeleteRegistryValue(int level, const char* subkey, 
                                          const char* key)
{
  if (this->GetRegistryLevel() < 0 || this->GetRegistryLevel() < level)
    {
    return 0;
    }
  int res = 0;
  char buffer[REG_KEY_NAME_SIZE_MAX];
  sprintf(buffer, "%s\\%s", this->GetVersionName(), subkey);
  vtkKWRegistryHelper *reg = this->GetRegistryHelper();
  reg->SetTopLevel(this->GetName());
  res = reg->DeleteValue(buffer, key);
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::HasRegistryValue(int level, const char* subkey, 
                                       const char* key)
{
  char buffer[REG_KEY_VALUE_SIZE_MAX];
  return this->GetRegistryValue(level, subkey, key, buffer);
}

//----------------------------------------------------------------------------
float vtkKWApplication::GetFloatRegistryValue(int level, const char* subkey, 
                                              const char* key)
{
  if (this->GetRegistryLevel() < 0 || this->GetRegistryLevel() < level)
    {
    return 0;
    }
  float res = 0;
  char buffer[REG_KEY_VALUE_SIZE_MAX];
  if (this->GetRegistryValue(level, subkey, key, buffer))
    {
    res = atof(buffer);
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetIntRegistryValue(int level, const char* subkey, 
                                      const char* key)
{
  if (this->GetRegistryLevel() < 0 || this->GetRegistryLevel() < level)
    {
    return 0;
    }
  int res = 0;
  char buffer[REG_KEY_VALUE_SIZE_MAX];
  if (this->GetRegistryValue(level, subkey, key, buffer))
    {
    res = atoi(buffer);
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetBooleanRegistryValue(
  int level, const char* subkey, const char* key, const char* trueval)
{
  if (this->GetRegistryLevel() < 0 || this->GetRegistryLevel() < level)
    {
    return 0;
    }
  char buffer[REG_KEY_VALUE_SIZE_MAX];
  int allset = 0;
  if (this->GetRegistryValue(level, subkey, key, buffer))
    {
    if (buffer && trueval && !strncmp(buffer+1, trueval+1, strlen(trueval)-1))
      {
      allset = 1;
      }
    }
  return allset;
}

//----------------------------------------------------------------------------
int vtkKWApplication::LoadScript(const char* filename)
{
  int res = 1;
  kwsys_stl::string filename_copy(filename);
  if (Tcl_EvalFile(Et_Interp, filename_copy.c_str()) != TCL_OK)
    {
    vtkErrorMacro("\n    Script: \n" << filename_copy.c_str()
                  << "\n    Returned Error on line "
                  << this->MainInterp->errorLine << ": \n      "  
                  << Tcl_GetStringResult(this->MainInterp) << endl);
    res = 0;
    if (this->ExitAfterLoadScript)
      {
      this->SetExitStatus(1);
      }
    }
  if (this->ExitAfterLoadScript)
    {
    this->SetPromptBeforeExit(0);
    this->Exit();
    }
  return res;
}

//----------------------------------------------------------------------------
void vtkKWApplication::SetLimitedEditionMode(int v)
{
  if (this->LimitedEditionMode == v)
    {
    return;
    }

  this->LimitedEditionMode = v;

  for (int i = 0; i < this->GetNumberOfWindows(); i++)
    {
    this->GetNthWindow(i)->UpdateEnableState();
    }

  this->Modified();
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetLimitedEditionModeAndWarn(const char *feature)
{
  if (this->LimitedEditionMode)
    {
    ostrstream feature_str;
    if (feature)
      {
      feature_str << " (" << feature << ")";
      }
    feature_str << ends;

    const char *lem_name = this->GetLimitedEditionModeName() 
      ? this->GetLimitedEditionModeName() : "Limited Edition";

    ostrstream msg_str;
    msg_str << "You are running in \"" << lem_name << "\" mode. "
            << "The feature you are trying to use" << feature_str.str() 
            << " is not available in this mode. "
            << ends;

    vtkKWMessageDialog::PopupMessage(
      this, 0, this->GetPrettyName(), msg_str.str(), 
      vtkKWMessageDialog::WarningIcon);

    feature_str.rdbuf()->freeze(0);
    msg_str.rdbuf()->freeze(0);
    }

  return this->LimitedEditionMode;
}

//----------------------------------------------------------------------------
const char* vtkKWApplication::GetPrettyName()
{
  ostrstream pretty_str;
  if (this->LimitedEditionMode)
    {
    const char *lem_name = this->GetLimitedEditionModeName();
    if (lem_name)
      {
      pretty_str << lem_name << " ";
      }
    else
      {
      if (this->Name)
        {
        pretty_str << this->Name << " ";
        }
      pretty_str << "Limited Edition ";
      }
    }
  else if (this->Name)
    {
    pretty_str << this->Name << " ";
    }
  pretty_str << this->MajorVersion << "." << this->MinorVersion << ends;

  this->SetPrettyName(pretty_str.str());
  pretty_str.rdbuf()->freeze(0);

  return this->PrettyName;
}

//----------------------------------------------------------------------------
void vtkKWApplication::SetCharacterEncoding(int val)
{
  if (val == this->CharacterEncoding)
    {
    return;
    }

  if (val < VTK_ENCODING_NONE)
    {
    val = VTK_ENCODING_NONE;
    }
  else if (val > VTK_ENCODING_UNKNOWN)
    {
    val = VTK_ENCODING_UNKNOWN;
    }

  this->CharacterEncoding = val;
  
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkKWApplication::CheckForArgument(
  int argc, char* argv[], const char *arg, int &index)
{
  if (!argc || !argv || !arg)
    {
    return VTK_ERROR;
    }

  // Check each arg
  // Be careful with valued argument (should not be, but who knows)

  int i;
  for (i = 0; i < argc; i++)
    {
    if (argv[i])
      {
      const char *equal = strchr(argv[i], '=');
      if (equal)
        {
        size_t part = equal - argv[i];
        if (strlen(arg) == part && !strncmp(arg, argv[i], part))
          {
          index = i;
          return VTK_OK;
          }
        }
      else
        {
        if (!strcmp(arg, argv[i]))
          {
          index = i;
          return VTK_OK;
          }
        }
      }
    }

  return VTK_ERROR;
}

//----------------------------------------------------------------------------
int vtkKWApplication::CheckForValuedArgument(
  int argc, char* argv[], const char *arg, int &index, int &value_pos)
{
  int found = vtkKWApplication::CheckForArgument(argc, argv, arg, index);
  if (found == VTK_OK)
    {
    const char *equal = strchr(argv[index], '=');
    if (equal)
      {
      value_pos = (equal - argv[index]) + 1;
      return VTK_OK;
      }
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetCheckForUpdatesPath(ostream &
#ifdef _WIN32
                                             path
#endif
  )
{
#ifdef _WIN32
  this->FindInstallationDirectory();
  if (this->InstallationDirectory)
    {
    ostrstream upd;
    upd << this->InstallationDirectory << "/WiseUpdt.exe" << ends;
    int res = kwsys::SystemTools::FileExists(upd.str());
    upd.rdbuf()->freeze(0);
    if (res)
      {
      path << upd.str();
      }
    return res;
    }
#endif

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWApplication::HasCheckForUpdates()
{
#ifdef _WIN32
  ostrstream upd;
  int res = this->GetCheckForUpdatesPath(upd);
  upd.rdbuf()->freeze(0);
  return res;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
void vtkKWApplication::CheckForUpdates()
{
  if (!this->HasCheckForUpdates())
    {
    return;
    }

#ifdef _WIN32
  ostrstream upd;
  if (this->GetCheckForUpdatesPath(upd))
    {
    upd << ends;
    _spawnl(_P_NOWAIT, upd.str(), upd.str(), NULL);
    }
  upd.rdbuf()->freeze(0);
#endif
}

//----------------------------------------------------------------------------
int vtkKWApplication::CanEmailFeedback()
{
#ifdef _WIN32
  HMODULE g_hMAPI = ::LoadLibrary("MAPI32.DLL");
  int has_mapi = g_hMAPI ? 1 : 0;
  ::FreeLibrary(g_hMAPI);
  return (has_mapi && this->EmailFeedbackAddress) ? 1 : 0;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
void vtkKWApplication::AddEmailFeedbackBody(ostream &os)
{
  os << this->GetPrettyName() 
     << " (" 
     << this->GetVersionName() 
     << " " 
     << this->GetReleaseName()
     << ")" << endl;

  kwsys_stl::string ver = 
    kwsys::SystemTools::GetOperatingSystemNameAndVersion();
  os << ver.c_str();

#ifdef _WIN32
  SYSTEM_INFO siSysInfo;
  GetSystemInfo(&siSysInfo); 
  os << ", " << siSysInfo.dwNumberOfProcessors << " CPU(s)";
#endif
  
  os << endl;
}

//----------------------------------------------------------------------------
void vtkKWApplication::AddEmailFeedbackSubject(ostream &os)
{
  os << this->GetPrettyName() << " User Feedback";
}

//----------------------------------------------------------------------------
void vtkKWApplication::EmailFeedback()
{
  if (!this->CanEmailFeedback())
    {
    return;
    }

#ifdef _WIN32

  // Load MAPI

  HMODULE g_hMAPI = ::LoadLibrary("MAPI32.DLL");
  if (!g_hMAPI)
    {
    return;
    }

  // Recipient To: (no SMTP: for Mozilla)

  MapiRecipDesc recip_to = 
    {
      0L,
      MAPI_TO,
      NULL,
      this->EmailFeedbackAddress,
      0L,
      NULL
    };

  // Body of the message

  ostrstream body;
  this->AddEmailFeedbackBody(body);
  body << endl << ends;

  // The email itself

  ostrstream email_subject;
  this->AddEmailFeedbackSubject(email_subject);
  email_subject << ends;

  MapiMessage email = 
    {
      0, 
      email_subject.str(),
      body.str(),
      NULL, 
      NULL, 
      NULL, 
      0, 
      NULL,
      1, 
      &recip_to, 
      0, 
      NULL
    };

  // Send it

  ULONG err = ((LPMAPISENDMAIL)GetProcAddress(g_hMAPI, "MAPISendMail"))(
    0L,
    0L,
    &email,
    MAPI_DIALOG | MAPI_LOGON_UI,
    0L);

  if (err != SUCCESS_SUCCESS)
    {
    ostrstream msg;
    msg << "Sorry, an error occurred while trying to email feedback. "
        << "Please make sure that your default email client has been "
        << "configured properly. The Microsoft Simple MAPI (Messaging "
        << "Application Program Interface) is used to perform this "
        << "operation and it might not be accessible as long as your "
        << "default email client is not running simultaneously. If you "
        << "continue to have problems please use your email client to "
        << "send us feedback at " << this->EmailFeedbackAddress << "."
        << ends;

    vtkKWMessageDialog::PopupMessage(
      this, 0, email_subject.str(), msg.str(), vtkKWMessageDialog::ErrorIcon);
    msg.rdbuf()->freeze(0);
    }

  body.rdbuf()->freeze(0);
  email_subject.rdbuf()->freeze(0);

  ::FreeLibrary(g_hMAPI);
#endif
}

//----------------------------------------------------------------------------
void vtkKWApplication::RegisterDialogUp(vtkKWWidget *)
{
  this->DialogUp++;
}

//----------------------------------------------------------------------------
void vtkKWApplication::UnRegisterDialogUp(vtkKWWidget *)
{
  this->DialogUp--;
}

//----------------------------------------------------------------------------
void vtkKWApplication::FindInstallationDirectory()
{
  const char *nameofexec = Tcl_GetNameOfExecutable();
  if (nameofexec && kwsys::SystemTools::FileExists(nameofexec))
    {
    kwsys_stl::string directory = 
      kwsys::SystemTools::GetFilenamePath(nameofexec);
    // remove the /bin from the end
    // directory[strlen(directory) - 4] = '\0';
    // => do not *do* that: first it breaks all the apps 
    // relying on this method to find where the binary is installed 
    // (hello plugins ?), second this is a hard-coded assumption, what
    // about msdev path, bin/release, bin/debug, etc.
    // If you need to remove whatever dir, just copy the result of this
    // method and strip it where needed.
    kwsys::SystemTools::ConvertToUnixSlashes(directory);
    this->SetInstallationDirectory(directory.c_str());
    }
  else
    {
    char setup_key[REG_KEY_NAME_SIZE_MAX];
    sprintf(setup_key, "%s\\Setup", this->GetVersionName());
    vtkKWRegistryHelper *reg = this->GetRegistryHelper();
    reg->SetTopLevel(this->GetName());
    char installed_path[REG_KEY_VALUE_SIZE_MAX];
    if (reg && reg->ReadValue(setup_key, "InstalledPath", installed_path))
      {
      kwsys_stl::string directory(installed_path);
      kwsys::SystemTools::ConvertToUnixSlashes(directory);
      this->SetInstallationDirectory(directory.c_str());
      }
    else
      {
      reg->SetGlobalScope(1);
      if (reg && reg->ReadValue(setup_key, "InstalledPath", installed_path))
        {
        kwsys_stl::string directory(installed_path);
        kwsys::SystemTools::ConvertToUnixSlashes(directory);
        this->SetInstallationDirectory(directory.c_str());
        }
      else
        {
        this->SetInstallationDirectory(0);
        }
      reg->SetGlobalScope(0);
      }
    }
}

//----------------------------------------------------------------------------
const char* vtkKWApplication::Script(const char* format, ...)
{
  va_list var_args1, var_args2;
  va_start(var_args1, format);
  va_start(var_args2, format);
  const char* result = vtkKWTkUtilities::EvaluateStringFromArgs(
    this, format, var_args1, var_args2);
  va_end(var_args1);
  va_end(var_args2);
  return result;
}

//----------------------------------------------------------------------------
int vtkKWApplication::EvaluateBooleanExpression(const char* format, ...)
{
  va_list var_args1, var_args2;
  va_start(var_args1, format);
  va_start(var_args2, format);
  const char* result = vtkKWTkUtilities::EvaluateStringFromArgs(
    this, format, var_args1, var_args2);
  va_end(var_args1);
  va_end(var_args2);

  return (result && !strcmp(result, "1")) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Name: " << this->GetName() << endl;
  os << indent << "MajorVersion: " << this->MajorVersion << endl;
  os << indent << "MinorVersion: " << this->MinorVersion << endl;
  os << indent << "ReleaseName: " 
     << this->GetReleaseName() << endl;
  os << indent << "VersionName: " 
     << this->GetVersionName() << endl;
  os << indent << "PrettyName: " 
     << this->GetPrettyName() << endl;
  os << indent << "EmailFeedbackAddress: "
     << (this->GetEmailFeedbackAddress() ? this->GetEmailFeedbackAddress() :
         "(none)")
     << endl;
  os << indent << "HelpDialogStartingPage: "
     << (this->HelpDialogStartingPage ? this->HelpDialogStartingPage :
         "(none)")
     << endl;
  os << indent << "DialogUp: " << this->GetDialogUp() << endl;
  os << indent << "ExitStatus: " << this->GetExitStatus() << endl;
  os << indent << "RegistryLevel: " << this->GetRegistryLevel() << endl;
  os << indent << "ExitAfterLoadScript: " << (this->ExitAfterLoadScript ? "on":"off") << endl;
  os << indent << "InExit: " << (this->InExit ? "on":"off") << endl;
  if (this->SplashScreen)
    {
    os << indent << "SplashScreen: " << this->SplashScreen << endl;
    }
  else
    {
    os << indent << "SplashScreen: (none)" << endl;
    }
  if (this->BalloonHelpManager)
    {
    os << indent << "BalloonHelpManager: " << this->BalloonHelpManager << endl;
    }
  else
    {
    os << indent << "BalloonHelpManager: (none)" << endl;
    }
  os << indent << "HasSplashScreen: " << (this->HasSplashScreen ? "on":"off") << endl;
  os << indent << "ShowSplashScreen: " << (this->ShowSplashScreen ? "on":"off") << endl;
  os << indent << "PromptBeforeExit: " << (this->GetPromptBeforeExit() ? "on":"off") << endl;
  os << indent << "InstallationDirectory: " 
     << (this->InstallationDirectory ? InstallationDirectory : "None") << endl;
  os << indent << "SaveWindowGeometry: " 
     << (this->SaveWindowGeometry ? "On" : "Off") << endl;
  os << indent << "LimitedEditionMode: " 
     << (this->LimitedEditionMode ? "On" : "Off") << endl;
  os << indent << "CharacterEncoding: " << this->CharacterEncoding << "\n";
  os << indent << "LimitedEditionModeName: " 
     << (this->LimitedEditionModeName ? this->LimitedEditionModeName
         : "None") << endl;
}
