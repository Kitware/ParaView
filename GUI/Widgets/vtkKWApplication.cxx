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

#include "vtkArrayMap.txx"
#include "vtkCollectionIterator.h"
#include "vtkKWApplicationSettingsInterface.h"
#include "vtkKWBWidgets.h"
#include "vtkKWDirectoryUtilities.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWObject.h"
#include "vtkKWRegisteryUtilities.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWidgetsConfigure.h"
#include "vtkKWWindow.h"
#include "vtkKWWindowCollection.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkString.h"
#include "vtkKWText.h"
#include "vtkTclUtil.h"

#include <stdarg.h>

#define REG_KEY_VALUE_SIZE_MAX 8192
#define REG_KEY_NAME_SIZE_MAX 100

static Tcl_Interp *Et_Interp = 0;

#ifdef _WIN32
#include <process.h>
#include <mapi.h>
#include <htmlhelp.h>
#include "kwappicon.h"
#endif

// I need those two Tcl functions. They usually are declared in tclIntDecls.h,
// but Unix build do not have access to VTK's tkInternals include path.
// Since the signature has not changed for years (at least since 8.2),
// let's just prototype them.

EXTERN Tcl_Obj* TclGetLibraryPath _ANSI_ARGS_((void));
EXTERN void TclSetLibraryPath _ANSI_ARGS_((Tcl_Obj * pathPtr));

int vtkKWApplication::WidgetVisibility = 1;

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWApplication );
vtkCxxRevisionMacro(vtkKWApplication, "1.177");

extern "C" int Vtktcl_Init(Tcl_Interp *interp);
extern "C" int Vtkkwwidgetstcl_Init(Tcl_Interp *interp);

int vtkKWApplicationCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWApplication::vtkKWApplication()
{
  this->BalloonHelpWidget = 0;
  this->CommandFunction = vtkKWApplicationCommand;
  
  this->ApplicationName = vtkString::Duplicate("Kitware");
  this->MajorVersion = 1;
  this->MinorVersion = 0;
  this->ApplicationVersionName = vtkString::Duplicate("Kitware10");
  this->ApplicationReleaseName = vtkString::Duplicate("unknown");
  this->ApplicationPrettyName = NULL;
  this->ApplicationInstallationDirectory = NULL;

  this->LimitedEditionModeName = NULL;
  char name[1024];
  sprintf(name, "%s Limited Edition", this->ApplicationName);
  this->SetLimitedEditionModeName(name);

  this->EmailFeedbackAddress = NULL;

  this->DisplayHelpStartingPage = vtkString::Duplicate("Introduction.htm");

  this->InExit = 0;
  this->DialogUp = 0;
  this->TraceFile = NULL;
  this->LimitedEditionMode = 0;

  this->ExitStatus = 0;

  this->Registery = 0;
  this->RegisteryLevel = 10;

  this->UseMessageDialogs = 1;  

  this->CharacterEncoding = VTK_ENCODING_UNKNOWN;
  this->SetCharacterEncoding(VTK_ENCODING_ISO_8859_1);

  this->Windows = vtkKWWindowCollection::New();  

  this->AboutDialog      = 0;
  this->AboutDialogImage = 0;
  this->AboutRuntimeInfo = 0;

  // add the application as $app

  if (vtkKWApplication::WidgetVisibility)
    {
    this->BalloonHelpWindow = vtkKWWidget::New();
    this->BalloonHelpLabel = vtkKWLabel::New();
    }
  else
    {
    this->BalloonHelpWindow = 0;
    this->BalloonHelpLabel = 0;
    }
  this->BalloonHelpPending = NULL;
  this->BalloonHelpDelay = 2;

  // setup tcl stuff
  this->MainInterp = Et_Interp;
  if (!this->MainInterp)
    {
    vtkErrorMacro(
      "Interpreter not set. This probably means that Tcl was not "
      "initialized properly...");
    return;
    }

  //vtkTclGetObjectFromPointer(this->MainInterp, (void *)this, 
  //                           vtkKWApplicationCommand);

  //this->Script("set Application %s",this->MainInterp->result);  
  this->Script("set Application %s",this->GetTclName());

  if (vtkKWApplication::WidgetVisibility)
    {
    this->BalloonHelpWindow->Create(
      this, "toplevel", "-background black -bd 1 -relief flat");
    this->BalloonHelpLabel->SetParent(this->BalloonHelpWindow);    
    this->BalloonHelpLabel->Create(
      this, "-background LightYellow -foreground black -justify left "
      "-wraplength 2i");
    this->Script("pack %s", this->BalloonHelpLabel->GetWidgetName());
    this->Script("wm overrideredirect %s 1", 
                 this->BalloonHelpWindow->GetWidgetName());
    this->Script("wm withdraw %s", this->BalloonHelpWindow->GetWidgetName());
    this->SplashScreen = vtkKWSplashScreen::New();
    }
  else
    {
    this->SplashScreen = NULL;
    }

  this->ExitOnReturn = 0;

  this->HasSplashScreen = 0;

  this->ApplicationExited = 0;

  this->ShowBalloonHelp = 1;
  this->SaveWindowGeometry = 1;
  this->ShowSplashScreen = 1;
}

//----------------------------------------------------------------------------
vtkKWApplication::~vtkKWApplication()
{
  this->SetLimitedEditionModeName(NULL);
  this->SetBalloonHelpPending(NULL);

  if (this->BalloonHelpWindow)
    {
    this->BalloonHelpWindow->Delete();
    this->BalloonHelpWindow = NULL;
    }

  if (this->BalloonHelpLabel)
    {
    this->BalloonHelpLabel->Delete();
    this->BalloonHelpLabel = NULL;
    }

  this->SetBalloonHelpWidget(0);

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

  if (this->Windows)
    {
    this->Windows->Delete();
    this->Windows = NULL;
    this->MainInterp = NULL;
    vtkObjectFactory::UnRegisterAllFactories();
    }

  this->SetApplicationName(NULL);
  this->SetApplicationVersionName(NULL);
  this->SetApplicationReleaseName(NULL);
  this->SetApplicationPrettyName(NULL);
  this->SetApplicationInstallationDirectory(NULL);

  this->SetEmailFeedbackAddress(NULL);

  this->SetDisplayHelpStartingPage(NULL);

  if (this->TraceFile)
    {
    this->TraceFile->close();
    delete this->TraceFile;
    this->TraceFile = NULL;
    }

  if (this->Registery )
    {
    this->Registery->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkKWApplication::SetApplication(vtkKWApplication*) 
{ 
  vtkErrorMacro( << "Do not set the Application on an Application" << endl); 
}

//----------------------------------------------------------------------------
void vtkKWApplication::FindApplicationInstallationDirectory()
{
  const char *nameofexec = Tcl_GetNameOfExecutable();
  if (nameofexec && vtkKWDirectoryUtilities::FileExists(nameofexec))
    {
    char directory[1024];
    vtkKWDirectoryUtilities::GetFilenamePath(nameofexec, directory);
    // remove the /bin from the end
    // What the h??? no, do not *do* that: first it breaks all the apps 
    // relying on this method to find where the binary is installed 
    // (hello plugins ?), second this is completely hard-coded, what
    // about msdev path, bin/release, bin/debug, etc.!
    // If you need to remove whatever dir, just copy the result of this
    // method and strip it yourself.
    // directory[strlen(directory) - 4] = '\0';
    vtkKWDirectoryUtilities *util = vtkKWDirectoryUtilities::New();
    this->SetApplicationInstallationDirectory(
      util->ConvertToUnixSlashes(directory));
    util->Delete();
    }
  else
    {
    char setup_key[REG_KEY_NAME_SIZE_MAX];
    sprintf(setup_key, "%s\\Setup", this->GetApplicationVersionName());
    vtkKWRegisteryUtilities *reg 
      = this->GetRegistery(this->GetApplicationName());
    char installed_path[REG_KEY_VALUE_SIZE_MAX];
    if (reg && reg->ReadValue(setup_key, "InstalledPath", installed_path))
      {
      vtkKWDirectoryUtilities *util = vtkKWDirectoryUtilities::New();
      this->SetApplicationInstallationDirectory(
        util->ConvertToUnixSlashes(installed_path));
      util->Delete();
      }
    else
      {
      reg->SetGlobalScope(1);
      if (reg && reg->ReadValue(setup_key, "InstalledPath", installed_path))
        {
        vtkKWDirectoryUtilities *util = vtkKWDirectoryUtilities::New();
        this->SetApplicationInstallationDirectory(
          util->ConvertToUnixSlashes(installed_path));
        util->Delete();
        }
      else
        {
      this->SetApplicationInstallationDirectory(0);
        }
      reg->SetGlobalScope(0);
      }
    }
}

//----------------------------------------------------------------------------
const char* vtkKWApplication::EvaluateString(const char* format, ...)
{  
  ostrstream str;
  str << "eval set vtkKWApplicationEvaluateStringTemporaryString " 
      << format << ends;
  va_list var_args1, var_args2;
  va_start(var_args1, format);
  va_start(var_args2, format);
  const char* result = this->ScriptInternal(str.str(), var_args1, var_args2);
  va_end(var_args1);
  va_end(var_args2);
  str.rdbuf()->freeze(0);
  return result;
}

//----------------------------------------------------------------------------
int vtkKWApplication::EvaluateBooleanExpression(const char* format, ...)
{
  va_list var_args1, var_args2;
  va_start(var_args1, format);
  va_start(var_args2, format);
  const char* result = this->ScriptInternal(format, var_args1, var_args2);
  va_end(var_args1);
  va_end(var_args2);
  if(vtkString::Equals(result, "1" ))
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWApplication::ExpandFileName(const char* format, ...)
{
  ostrstream str;
  str << "eval file join {\"" << format << "\"}" << ends;
  va_list var_args1, var_args2;
  va_start(var_args1, format);
  va_start(var_args2, format);
  const char* result = this->ScriptInternal(str.str(), var_args1, var_args2);  
  va_end(var_args1);
  va_end(var_args2);
  str.rdbuf()->freeze(0);
  return result;
}

//----------------------------------------------------------------------------
const char* vtkKWApplication::Script(const char* format, ...)
{
  va_list var_args1, var_args2;
  va_start(var_args1, format);
  va_start(var_args2, format);
  const char* result = this->ScriptInternal(format, var_args1, var_args2);
  va_end(var_args1);
  va_end(var_args2);
  return result;
}

//----------------------------------------------------------------------------
const char* vtkKWApplication::ScriptInternal(const char* format,
                                             va_list var_args1,
                                             va_list var_args2)
{
  // We need a place to construct the script.
  char event[1600];
  char* buffer = event;
  
  // Estimate the length of the result string.  Never underestimates.
  int length = this->EstimateFormatLength(format, var_args1);
  
  // If our stack-allocated buffer is too small, allocate on one on
  // the heap that will be large enough.
  if(length > 1599)
    {
    buffer = new char[length+1];
    }
  
  // Print to the string.
  vsprintf(buffer, format, var_args2);
  
  // Evaluate the string in Tcl.
  if(Tcl_GlobalEval(this->MainInterp, buffer) != TCL_OK)
    {
    vtkErrorMacro("\n    Script: \n" << buffer
                  << "\n    Returned Error on line "
                  << this->MainInterp->errorLine << ": \n"  
                  << Tcl_GetStringResult(this->MainInterp) << endl);
    }
  
  // Free the buffer from the heap if we allocated it.
  if(buffer != event)
    {
    delete [] buffer;
    }
  
  // Convert the Tcl result to its string representation.
  return Tcl_GetStringResult(this->MainInterp);
}

//----------------------------------------------------------------------------
const char* vtkKWApplication::SimpleScript(const char* script)
{
  // Tcl might modify the script in-place.  We need a temporary copy.
  char event[1600];
  char* buffer = event;  
  
  // Make sure we have a script.
  int length = vtkString::Length(script);
  if(length < 1)
    {
    return 0;
    }
  
  // If our stack-allocated buffer is too small, allocate on one on
  // the heap that will be large enough.
  if(length > 1599)
    {
    buffer = new char[length+1];
    }
  
  // Copy the string to our buffer.
  strcpy(buffer, script);
  
  // Evaluate the string in Tcl.
  if(Tcl_GlobalEval(this->MainInterp, buffer) != TCL_OK)
    {
    vtkErrorMacro("\n    Script: \n" << buffer
                  << "\n    Returned Error on line "
                  << this->MainInterp->errorLine << ": \n"  
                  << Tcl_GetStringResult(this->MainInterp) << endl);
    }
  
  // Free the buffer from the heap if we allocated it.
  if(buffer != event)
    {
    delete [] buffer;
    }
  
  // Convert the Tcl result to its string representation.
  return Tcl_GetStringResult(this->MainInterp);
}

//----------------------------------------------------------------------------
void vtkKWApplication::Close(vtkKWWindow *win)
{
  if ( this->Windows )
    {
    win->PrepareForDelete();
    this->Windows->RemoveItem(win);
    if (this->Windows->GetNumberOfItems() < 1)
      {
      this->Exit();
      }
    }
}

//----------------------------------------------------------------------------
vtkKWWindowCollection *vtkKWApplication::GetWindows()
{
  return this->Windows;
}


//----------------------------------------------------------------------------
void vtkKWApplication::AddWindow(vtkKWWindow *w)
{
  this->Windows->AddItem(w);
}

//----------------------------------------------------------------------------
void vtkKWApplication::Exit()
{
  // Avoid a recursive exit.

  if (this->InExit)
    {
    return;
    }
  this->InExit = 1;

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

  vtkKWWindow* win = 0;
  this->Windows->InitTraversal();
  
  while (this->Windows && (win = this->Windows->GetNextKWWindow()))
    {
    win->SetPromptBeforeClose(0);
    win->Close();
    if (this->Windows)
      {
      this->Windows->InitTraversal();
      }
    }
  
  this->SetBalloonHelpPending(NULL);

  if (this->BalloonHelpWindow)
    {
    this->BalloonHelpWindow->Delete();
    this->BalloonHelpWindow = NULL;
    }

  if (this->BalloonHelpLabel)
    {
    this->BalloonHelpLabel->Delete();
    this->BalloonHelpLabel = NULL;
    }

  if (this->SplashScreen)
    {
    this->SplashScreen->Delete();
    this->SplashScreen = NULL;
    }

  this->Cleanup();

  this->ApplicationExited = 1;
  return;
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
    if (nameofexec && vtkKWDirectoryUtilities::FileExists(nameofexec))
      {
      char dir[1024], dir_unix[1024], buffer[1024];
      vtkKWDirectoryUtilities *util = vtkKWDirectoryUtilities::New();
      util->GetFilenamePath(nameofexec, dir);
      strcpy(dir_unix, util->ConvertToUnixSlashes(dir));

      // Installed KW application, otherwise build tree/windows
      sprintf(buffer, "%s/..%s/TclTk", dir_unix, KW_INSTALL_LIB_DIR);
      int exists = vtkKWDirectoryUtilities::FileExists(buffer);
      if (!exists)
        {
        sprintf(buffer, "%s/TclTk", dir_unix);
        exists = vtkKWDirectoryUtilities::FileExists(buffer);
        }
      sprintf(buffer, util->CollapseDirectory(buffer));
      util->Delete();
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
        if (vtkKWDirectoryUtilities::FileExists(tcl_library))
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
          if (vtkKWDirectoryUtilities::FileExists(tk_library))
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

  if (vtkKWApplication::WidgetVisibility)
    {
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
    }
    
  // create the SetApplicationIcon command
#ifdef _WIN32
  ApplicationIcon_DoInit(interp);
#endif

  // Initialize VTK

  Vtktcl_Init(interp);

  // Initialize Widgets

  if (vtkKWApplication::WidgetVisibility)
    {
    Vtkkwwidgetstcl_Init(interp);

    vtkKWBWidgets::Initialize(interp);
    }

  return interp;
}

//----------------------------------------------------------------------------
void vtkKWApplication::Start()
{ 
  int i;
  
  // look at Tcl for any args
  this->Script("set argc");
  int argc = vtkKWObject::GetIntegerResult(this) + 1;
  char **argv = new char *[argc];
  argv[0] = NULL;
  for (i = 1; i < argc; i++)
    {
    this->Script("lindex $argv %d",i-1);
    argv[i] = strdup(this->GetMainInterp()->result);
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
void vtkKWApplication::Start(char *arg)
{ 
  this->Start(1,&arg);
}

//----------------------------------------------------------------------------
void vtkKWApplication::Start(int /*argc*/, char ** /*argv*/)
{ 
  while (this->Windows && this->Windows->GetNumberOfItems())
    {
    this->DoOneTclEvent();
    }
  
  //Tk_MainLoop();
}

//----------------------------------------------------------------------------
void vtkKWApplication::GetApplicationSettingsFromRegistery()
{ 
  // Show balloon help ?

  if (this->HasRegisteryValue(
    2, "RunTime", VTK_KW_SHOW_TOOLTIPS_REG_KEY))
    {
    this->ShowBalloonHelp = this->GetIntRegisteryValue(
      2, "RunTime", VTK_KW_SHOW_TOOLTIPS_REG_KEY);
    }

  // Save window geometry ?

  if (this->HasRegisteryValue(
    2, "Geometry", VTK_KW_SAVE_WINDOW_GEOMETRY_REG_KEY))
    {
    this->SaveWindowGeometry = this->GetIntRegisteryValue(
      2, "Geometry", VTK_KW_SAVE_WINDOW_GEOMETRY_REG_KEY);
    }

  // Show splash screen ?

  if (this->HasRegisteryValue(
    2, "RunTime", VTK_KW_SHOW_SPLASH_SCREEN_REG_KEY))
    {
    this->ShowSplashScreen = this->GetIntRegisteryValue(
      2, "RunTime", VTK_KW_SHOW_SPLASH_SCREEN_REG_KEY);
    }

  if (this->RegisteryLevel <= 0)
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
void vtkKWApplication::DisplayHelp(vtkKWWindow* master)
{
#ifdef _WIN32
  ostrstream temp;
  if (this->ApplicationInstallationDirectory)
    {
    temp << this->ApplicationInstallationDirectory << "/";
    }
  temp << this->ApplicationName << ".chm";
  if (this->DisplayHelpStartingPage)
    {
    temp << "::/" << this->DisplayHelpStartingPage;
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
void vtkKWApplication::BalloonHelpTrigger(vtkKWWidget *widget)
{
  if ( this->InExit )
    {
    return;
    }
  if ( !widget->IsCreated() )
    {
    return;
    }
  char *result;

  // If there is no help string, return

  if (!this->ShowBalloonHelp || 
      !widget->GetBalloonHelpString() || 
      this->BalloonHelpDelay <= 0)
    {
    this->SetBalloonHelpPending(NULL);
    return;
    }
  
  this->BalloonHelpCancel();
  this->SetBalloonHelpWidget(widget);
  this->Script("after %d {catch {%s BalloonHelpDisplay %s}}", 
               this->BalloonHelpDelay * 1000,
               this->GetTclName(), widget->GetTclName());
  result = this->GetMainInterp()->result;
  this->SetBalloonHelpPending(result);
}

//----------------------------------------------------------------------------
void vtkKWApplication::BalloonHelpDisplay(vtkKWWidget *widget)
{
  if ( this->InExit )
    {
    return;
    }
  if (!this->ShowBalloonHelp ||
      !this->BalloonHelpLabel || 
      !this->BalloonHelpWindow ||
      !widget->GetParent())
    {
    return;
    }
  int x, y;

  // If there is no help string, return
  if ( !widget->GetBalloonHelpString() )
    {
    this->SetBalloonHelpPending(NULL);
    return;
    }

  // make sure it is really pending
  this->BalloonHelpLabel->SetLabel(widget->GetBalloonHelpString());

  // Get the position of the mouse in the renderer.
  this->Script( "winfo pointerx %s", widget->GetWidgetName());
  x = vtkKWObject::GetIntegerResult(this);
  this->Script( "winfo pointery %s", widget->GetWidgetName());
  y = vtkKWObject::GetIntegerResult(this);

  // Get the position of the parent widget of the one needing help
  this->Script( "winfo rootx %s", widget->GetParent()->GetWidgetName());
  int xw = vtkKWObject::GetIntegerResult(this);
  this->Script( "winfo rooty %s", widget->GetParent()->GetWidgetName());

  // get the size of the balloon window
  this->Script( "winfo reqwidth %s", this->BalloonHelpLabel->GetWidgetName());
  int dx = vtkKWObject::GetIntegerResult(this);
  this->Script( "winfo reqheight %s", this->BalloonHelpLabel->GetWidgetName());
  
  // get the size of the parent window of the one needing help
  this->Script( "winfo width %s", widget->GetParent()->GetWidgetName());
  int dxw = vtkKWObject::GetIntegerResult(this);
  this->Script( "winfo height %s", widget->GetParent()->GetWidgetName());
  
  // Set the position of the window relative to the mouse.
  int just = widget->GetBalloonHelpJustification();

  // just 0 == left just 2 == right
  if (just)
    {
    if (x + dx > xw + dxw)
      {
      x = xw + dxw - dx;
      }
    }
  // with left justification (default) still try to keep the 
  // help from going past the right edge of the widget
  else
    {
     // if it goes too far right
    if (x + dx > xw + dxw)
      {
      // move it to the left
      x = xw + dxw - dx;
      // but not past the left edge of the parent widget
      if (x < xw)
        {
        x = xw;
        }
      }
    }
  
  this->Script("wm geometry %s +%d+%d",
               this->BalloonHelpWindow->GetWidgetName(), x, y+15);
  this->Script("update");

  // map the window
  if (this->BalloonHelpPending)
    {
    this->Script("wm deiconify %s", this->BalloonHelpWindow->GetWidgetName());
    this->Script("raise %s", this->BalloonHelpWindow->GetWidgetName());
    }
  
  this->SetBalloonHelpPending(NULL);

}

//----------------------------------------------------------------------------
void vtkKWApplication::BalloonHelpCancel()
{
  if ( this->InExit )
    {
    return;
    }
  if (this->BalloonHelpPending)
    {
    this->Script("after cancel %s", this->BalloonHelpPending);
    this->SetBalloonHelpPending(NULL);
    }
  if ( this->BalloonHelpWindow )
    {
    this->Script("wm withdraw %s",this->BalloonHelpWindow->GetWidgetName());
    }
  this->SetBalloonHelpWidget(0);
}

//----------------------------------------------------------------------------
void vtkKWApplication::BalloonHelpWithdraw()
{
  if ( this->InExit )
    {
    return;
    }
  if ( !this->BalloonHelpLabel || !this->BalloonHelpWindow )
    {
    return;
    }
  this->Script("wm withdraw %s",this->BalloonHelpWindow->GetWidgetName());
  if ( this->BalloonHelpWidget )
    {
    this->BalloonHelpTrigger(this->BalloonHelpWidget);
    }
}

//----------------------------------------------------------------------------
void vtkKWApplication::SetShowBalloonHelp(int v)
{
  if (this->ShowBalloonHelp == v)
    {
    return;
    }
  this->ShowBalloonHelp = v;
  this->Modified();

  if (!this->ShowBalloonHelp)
    {
    this->BalloonHelpCancel();
    }
}

//----------------------------------------------------------------------------
void vtkKWApplication::SetWidgetVisibility(int v)
{
  vtkKWApplication::WidgetVisibility = v;
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetWidgetVisibility() 
{
  return vtkKWApplication::WidgetVisibility;
}

//----------------------------------------------------------------------------
void vtkKWApplication::DisplayAbout(vtkKWWindow* master)
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
    this->AboutDialog->SetOptions(
      this->AboutDialog->GetOptions() | vtkKWMessageDialog::NoDecoration);
    this->AboutDialog->Create(this, "-bd 1 -relief solid");
    }

  this->ConfigureAbout();

  this->AboutDialog->Invoke();
}

//----------------------------------------------------------------------------
void vtkKWApplication::ConfigureAbout()
{
  if (this->HasSplashScreen && 
      this->SplashScreen)
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

      this->Script("pack %s -side left -padx 2 -expand 1 -fill both",
                   this->AboutRuntimeInfo->GetWidgetName());
      this->Script("pack %s -side bottom",  // -expand 1 -fill both
                   this->AboutDialog->GetMessageDialogFrame()->GetWidgetName());
      }
    }

  ostrstream title;
  title << "About " << this->GetApplicationPrettyName() << ends;
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
  os << this->GetApplicationPrettyName();
  const char *app_ver_name = this->GetApplicationVersionName();
  const char *app_rel_name = this->GetApplicationReleaseName();
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
void vtkKWApplication::AddSimpleTraceEntry(const char *trace)
{
  this->AddTraceEntry(trace);
}

//----------------------------------------------------------------------------
void vtkKWApplication::AddTraceEntry(const char *format, ...)
{
  if (this->TraceFile == NULL)
    {
    return;
    }
  
  char event[1600];
  char* buffer = event;
  
  va_list ap;
  va_start(ap, format);
  int length = this->EstimateFormatLength(format, ap);
  va_end(ap);
  
  if(length > 1599)
    {
    buffer = new char[length+1];
    }

  va_list var_args;
  va_start(var_args, format);
  vsprintf(buffer, format, var_args);
  va_end(var_args);

  *(this->TraceFile) << buffer << endl;
  
  if(buffer != event)
    {
    delete [] buffer;
    }
}

//----------------------------------------------------------------------------
vtkKWRegisteryUtilities *vtkKWApplication::GetRegistery( const char*toplevel )
{
  this->GetRegistery();
  this->Registery->SetTopLevel( toplevel );
  return this->Registery;
}

//----------------------------------------------------------------------------
vtkKWRegisteryUtilities *vtkKWApplication::GetRegistery()
{
  if ( !this->Registery )
    {
    this->Registery = vtkKWRegisteryUtilities::New();
    }
  return this->Registery;
}

//----------------------------------------------------------------------------
void vtkKWApplication::SetBalloonHelpWidget( vtkKWWidget *widget )
{
  if ( this->InExit && widget )
    {
    return;
    }
  if ( this->BalloonHelpWidget )
    {
    this->BalloonHelpWidget->UnRegister(this);
    this->BalloonHelpWidget = 0;
    }
  if ( widget )
    {
    this->BalloonHelpWidget = widget;
    this->BalloonHelpWidget->Register(this);
    }  
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetMessageDialogResponse(const char* dialogname)
{
  char buffer[REG_KEY_VALUE_SIZE_MAX];
  int retval = 0;
  if ( this->GetRegisteryValue(3, "Dialogs", dialogname, buffer) )
    {
    retval = atoi(buffer);
    }
  return retval;
}

//----------------------------------------------------------------------------
void vtkKWApplication::SetMessageDialogResponse(const char* dialogname, 
                                               int response)
{
  this->SetRegisteryValue(3, "Dialogs", dialogname, "%d", response);
}


//----------------------------------------------------------------------------
int vtkKWApplication::SetRegisteryValue(int level, const char* subkey, 
                                        const char* key, 
                                        const char* format, ...)
{
  if ( this->GetRegisteryLevel() < 0 ||
       this->GetRegisteryLevel() < level )
    {
    return 0;
    }
  int res = 0;
  char buffer[REG_KEY_NAME_SIZE_MAX];
  char value[REG_KEY_VALUE_SIZE_MAX];
  sprintf(buffer, "%s\\%s", 
          this->GetApplication()->GetApplicationVersionName(),
          subkey);
  va_list var_args;
  va_start(var_args, format);
  vsprintf(value, format, var_args);
  va_end(var_args);
  
  vtkKWRegisteryUtilities *reg 
    = this->GetRegistery(this->GetApplicationName());
  res = reg->SetValue(buffer, key, value);
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetRegisteryValue(int level, const char* subkey, 
                                        const char* key, char* value)
{
  int res = 0;
  char buff[REG_KEY_VALUE_SIZE_MAX];
  if ( !this->GetApplication() ||
       this->GetRegisteryLevel() < 0 ||
       this->GetRegisteryLevel() < level )
    {
    return 0;
    }
  char buffer[REG_KEY_NAME_SIZE_MAX];
  sprintf(buffer, "%s\\%s", 
          this->GetApplicationVersionName(),
          subkey);

  vtkKWRegisteryUtilities *reg 
    = this->GetRegistery(this->GetApplicationName());
  res = reg->ReadValue(buffer, key, buff);
  if ( *buff && value )
    {
    *value = 0;
    strcpy(value, buff);
    }  
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::DeleteRegisteryValue(int level, const char* subkey, 
                                      const char* key)
{
  if ( this->GetRegisteryLevel() < 0 ||
       this->GetRegisteryLevel() < level )
    {
    return 0;
    }
  int res = 0;
  char buffer[REG_KEY_NAME_SIZE_MAX];
  sprintf(buffer, "%s\\%s", 
          this->GetApplicationVersionName(),
          subkey);
  
  vtkKWRegisteryUtilities *reg 
    = this->GetRegistery(this->GetApplicationName());
  res = reg->DeleteValue(buffer, key);
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::HasRegisteryValue(int level, const char* subkey, 
                                        const char* key)
{
  char buffer[REG_KEY_VALUE_SIZE_MAX];
  return this->GetRegisteryValue(level, subkey, key, buffer);
}

//----------------------------------------------------------------------------
int vtkKWApplication::SelfTest()
{
  int res = 0;
  this->EvaluateString("foo");
  res += (!this->EvaluateBooleanExpression("proc a {} { return 1; }; a"));
  res += this->EvaluateBooleanExpression("proc a {} { return 0; }; a");

  return (res == 0);
}

//----------------------------------------------------------------------------
int vtkKWApplication::LoadTclScript(const char* filename)
{
  int res = 1;
  char* file = vtkString::Duplicate(filename);
  // add this window as a variable
  if ( Tcl_EvalFile(Et_Interp, file) != TCL_OK )
    {
    res = 0;
    }
  delete [] file;
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::LoadScript(const char* filename)
{
  int res = 1;
  if ( !vtkKWApplication::LoadTclScript(filename) )
    {
    vtkErrorMacro("\n    Script: \n" << filename 
                  << "\n    Returned Error on line "
                  << this->MainInterp->errorLine << ": \n      "  
                  << Tcl_GetStringResult(this->MainInterp) << endl);
    res = 0;
    this->SetExitStatus(1);
    }
  if ( this->ExitOnReturn )
    {
    this->Exit();
    }
  return res;
}

//----------------------------------------------------------------------------
float vtkKWApplication::GetFloatRegisteryValue(int level, const char* subkey, 
                                               const char* key)
{
  if ( this->GetRegisteryLevel() < 0 ||
       this->GetRegisteryLevel() < level )
    {
    return 0;
    }
  float res = 0;
  char buffer[REG_KEY_VALUE_SIZE_MAX];
  if ( this->GetRegisteryValue( 
         level, subkey, key, buffer ) )
    {
    res = atof(buffer);
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetIntRegisteryValue(int level, const char* subkey, 
                                      const char* key)
{
  if ( this->GetRegisteryLevel() < 0 ||
       this->GetRegisteryLevel() < level )
    {
    return 0;
    }

  int res = 0;
  char buffer[REG_KEY_VALUE_SIZE_MAX];
  if ( this->GetRegisteryValue( 
         level, subkey, key, buffer ) )
    {
    res = atoi(buffer);
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWApplication::BooleanRegisteryCheck(int level, 
                                            const char* subkey,
                                            const char* key, 
                                            const char* trueval)
{
  if ( this->GetRegisteryLevel() < 0 ||
       this->GetRegisteryLevel() < level )
    {
    return 0;
    }
  char buffer[REG_KEY_VALUE_SIZE_MAX];
  int allset = 0;
  if ( this->GetRegisteryValue(level, subkey, key, buffer) )
    {
    if ( !strncmp(buffer+1, trueval+1, vtkString::Length(trueval)-1) )
      {
      allset = 1;
      }
    }
  return allset;
}

//----------------------------------------------------------------------------
void vtkKWApplication::SetLimitedEditionMode(int v)
{
  if (this->LimitedEditionMode == v)
    {
    return;
    }

  this->LimitedEditionMode = v;

  this->UpdateEnableStateForAllWindows();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWApplication::UpdateEnableStateForAllWindows()
{
  vtkCollectionIterator *it = this->Windows->NewIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWWindow* win = vtkKWWindow::SafeDownCast(it->GetObject());
    if (win)
      {
      win->UpdateEnableState();
      }
    }
  it->Delete();
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
      this, 0, this->GetApplicationPrettyName(), msg_str.str(), 
      vtkKWMessageDialog::WarningIcon);

    feature_str.rdbuf()->freeze(0);
    msg_str.rdbuf()->freeze(0);
    }

  return this->LimitedEditionMode;
}

//----------------------------------------------------------------------------
const char* vtkKWApplication::GetApplicationPrettyName()
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
      if (this->ApplicationName)
        {
        pretty_str << this->ApplicationName << " ";
        }
      pretty_str << "Limited Edition ";
      }
    }
  else if (this->ApplicationName)
    {
    pretty_str << this->ApplicationName << " ";
    }
  pretty_str << this->MajorVersion << "." << this->MinorVersion << ends;

  this->SetApplicationPrettyName(pretty_str.str());
  pretty_str.rdbuf()->freeze(0);

  return this->ApplicationPrettyName;
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
  this->FindApplicationInstallationDirectory();
  if (this->ApplicationInstallationDirectory)
    {
    ostrstream upd;
    upd << this->ApplicationInstallationDirectory << "/WiseUpdt.exe" << ends;
    int res = vtkKWDirectoryUtilities::FileExists(upd.str());
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
int vtkKWApplication::GetSystemVersion(ostream &
#ifdef _WIN32
                                       os
#endif
  )
{
#ifdef _WIN32
  OSVERSIONINFOEX osvi;
  BOOL bOsVersionInfoEx;

  // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
  // If that fails, try using the OSVERSIONINFO structure.

  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

  if (!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *)&osvi)))
    {
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx((OSVERSIONINFO *)&osvi)) 
      {
      return 0;
      }
    }
  
  switch (osvi.dwPlatformId)
    {
    // Test for the Windows NT product family.

    case VER_PLATFORM_WIN32_NT:
      
      // Test for the specific product family.

      if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
        {
        os << "Microsoft Windows Server 2003 family";
        }

      if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
        {
        os << "Microsoft Windows XP";
        }

      if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
        {
        os << "Microsoft Windows 2000";
        }

      if (osvi.dwMajorVersion <= 4)
        {
        os << "Microsoft Windows NT";
        }

      // Test for specific product on Windows NT 4.0 SP6 and later.

      if (bOsVersionInfoEx)
        {
        // Test for the workstation type.

#if (_MSC_VER >= 1300) 
        if (osvi.wProductType == VER_NT_WORKSTATION)
          {
          if (osvi.dwMajorVersion == 4)
            {
            os << " Workstation 4.0";
            }
          else if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
            {
            os << " Home Edition";
            }
          else
            {
            os << " Professional";
            }
          }
            
        // Test for the server type.

        else if (osvi.wProductType == VER_NT_SERVER)
          {
          if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
            {
            if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
              {
              os << " Datacenter Edition";
              }
            else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
              {
              os << " Enterprise Edition";
              }
            else if (osvi.wSuiteMask == VER_SUITE_BLADE)
              {
              os << " Web Edition";
              }
            else
              {
              os << " Standard Edition";
              }
            }
          
          else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
            {
            if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
              {
              os << " Datacenter Server";
              }
            else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
              {
              os << " Advanced Server";
              }
            else
              {
              os << " Server";
              }
            }

          else  // Windows NT 4.0 
            {
            if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
              {
              os << " Server 4.0, Enterprise Edition";
              }
            else
              {
              os << " Server 4.0";
              }
            }
          }
#endif // Visual Studio 7 and up
        }

      // Test for specific product on Windows NT 4.0 SP5 and earlier

      else  
        {
        HKEY hKey;
        #define BUFSIZE 80
        char szProductType[BUFSIZE];
        DWORD dwBufLen=BUFSIZE;
        LONG lRet;

        lRet = RegOpenKeyEx(
          HKEY_LOCAL_MACHINE,
          "SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
          0, KEY_QUERY_VALUE, &hKey);
        if (lRet != ERROR_SUCCESS)
          {
          return 0;
          }

        lRet = RegQueryValueEx(hKey, "ProductType", NULL, NULL,
                               (LPBYTE) szProductType, &dwBufLen);

        if ((lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE))
          {
          return 0;
          }

        RegCloseKey(hKey);

        if (lstrcmpi("WINNT", szProductType) == 0)
          {
          os << " Workstation";
          }
        if (lstrcmpi("LANMANNT", szProductType) == 0)
          {
          os << " Server";
          }
        if (lstrcmpi("SERVERNT", szProductType) == 0)
          {
          os << " Advanced Server";
          }

        os << " " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion;
        }

      // Display service pack (if any) and build number.

      if (osvi.dwMajorVersion == 4 && 
          lstrcmpi(osvi.szCSDVersion, "Service Pack 6") == 0)
        {
        HKEY hKey;
        LONG lRet;

        // Test for SP6 versus SP6a.

        lRet = RegOpenKeyEx(
          HKEY_LOCAL_MACHINE,
          "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009",
          0, KEY_QUERY_VALUE, &hKey);

        if (lRet == ERROR_SUCCESS)
          {
          os << " Service Pack 6a (Build " 
             << (osvi.dwBuildNumber & 0xFFFF) << ")";
          }
        else // Windows NT 4.0 prior to SP6a
          {
          os << " " << osvi.szCSDVersion << " (Build " 
             << (osvi.dwBuildNumber & 0xFFFF) << ")";
          }
        
        RegCloseKey(hKey);
        }
      else // Windows NT 3.51 and earlier or Windows 2000 and later
        {
        os << " " << osvi.szCSDVersion << " (Build " 
           << (osvi.dwBuildNumber & 0xFFFF) << ")";
        }

      break;

      // Test for the Windows 95 product family.

    case VER_PLATFORM_WIN32_WINDOWS:

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
        {
        os << "Microsoft Windows 95";
        if (osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B')
          {
          os << " OSR2";
          }
        }

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
        {
        os << "Microsoft Windows 98";
        if (osvi.szCSDVersion[1] == 'A')
          {
          os << " SE";
          }
        }

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
        {
        os << "Microsoft Windows Millennium Edition";
        } 
      break;

    case VER_PLATFORM_WIN32s:
      
      os <<  "Microsoft Win32s";
      break;
    }
#endif

  return 1;
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
  os << this->GetApplicationPrettyName() 
     << " (" 
     << this->GetApplicationVersionName() 
     << " " 
     << this->GetApplicationReleaseName()
     << ")" << endl;

  this->GetSystemVersion(os);

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
  os << this->GetApplicationPrettyName() << " User Feedback";
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
void vtkKWApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ApplicationName: " << this->GetApplicationName() << endl;
  os << indent << "MajorVersion: " << this->MajorVersion << endl;
  os << indent << "MinorVersion: " << this->MinorVersion << endl;
  os << indent << "ApplicationReleaseName: " 
     << this->GetApplicationReleaseName() << endl;
  os << indent << "ApplicationVersionName: " 
     << this->GetApplicationVersionName() << endl;
  os << indent << "ApplicationPrettyName: " 
     << this->GetApplicationPrettyName() << endl;
  os << indent << "EmailFeedbackAddress: "
     << (this->GetEmailFeedbackAddress() ? this->GetEmailFeedbackAddress() :
         "(none)")
     << endl;
  os << indent << "ShowBalloonHelp: " << (this->ShowBalloonHelp ? "on":"off") << endl;
  os << indent << "BalloonHelpDelay: " << this->GetBalloonHelpDelay() << endl;
  os << indent << "DialogUp: " << this->GetDialogUp() << endl;
  os << indent << "ExitStatus: " << this->GetExitStatus() << endl;
  os << indent << "RegisteryLevel: " << this->GetRegisteryLevel() << endl;
  os << indent << "UseMessageDialogs: " << this->GetUseMessageDialogs() 
     << endl;
  os << indent << "ExitOnReturn: " << (this->ExitOnReturn ? "on":"off") << endl;
  os << indent << "InExit: " << (this->InExit ? "on":"off") << endl;
  if (this->SplashScreen)
    {
    os << indent << "SplashScreen: " << this->SplashScreen << endl;
    }
  else
    {
    os << indent << "SplashScreen: (none)" << endl;
    }
  os << indent << "HasSplashScreen: " << (this->HasSplashScreen ? "on":"off") << endl;
  os << indent << "ShowSplashScreen: " << (this->ShowSplashScreen ? "on":"off") << endl;
  os << indent << "ApplicationExited: " << this->ApplicationExited << endl;
  os << indent << "ApplicationInstallationDirectory: " 
     << (this->ApplicationInstallationDirectory ? ApplicationInstallationDirectory : "None") << endl;
  os << indent << "SaveWindowGeometry: " 
     << (this->SaveWindowGeometry ? "On" : "Off") << endl;
  os << indent << "LimitedEditionMode: " 
     << (this->LimitedEditionMode ? "On" : "Off") << endl;
  os << indent << "CharacterEncoding: " << this->CharacterEncoding << "\n";
  os << indent << "LimitedEditionModeName: " 
     << (this->LimitedEditionModeName ? this->LimitedEditionModeName
         : "None") << endl;
}
