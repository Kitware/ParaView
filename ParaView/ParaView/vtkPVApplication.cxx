/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplication.cxx
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
#include "vtkPVApplication.h"

#include "vtkPVDemoPaths.h"
#include "vtkToolkits.h"
#include "vtkPVConfig.h"
#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIGroup.h"
#endif

#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCollectionIterator.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetAttributes.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInstantiator.h"
#include "vtkIntArray.h"
#include "vtkKWApplicationSettingsInterface.h"
#include "vtkKWDialog.h"
#include "vtkKWEvent.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWWindowCollection.h"
#include "vtkLongArray.h"
#include "vtkMapper.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPVApplicationSettingsInterface.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVConfig.h"
#include "vtkPVData.h"
#include "vtkPVHelpPaths.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSourceInterfaceDirectories.h"
#include "vtkPVTraceFileDialog.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProbeFilter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkShortArray.h"
#include "vtkString.h"
#include "vtkTclUtil.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerInterpreter.h"

// #include "vtkPVRenderGroupDialog.h"

#include <sys/stat.h>
#include <stdarg.h>
#include <signal.h>

#include "vtkKWDirectoryUtilities.h"

#ifdef _WIN32
#include "vtkKWRegisteryUtilities.h"

#include "ParaViewRC.h"

#include "htmlhelp.h"
#include "direct.h"
#endif

#include <vtkstd/vector>
#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVApplication);
vtkCxxRevisionMacro(vtkPVApplication, "1.221.2.6");
vtkCxxSetObjectMacro(vtkPVApplication, RenderModule, vtkPVRenderModule);


int vtkPVApplicationCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);




//----------------------------------------------------------------------------
void vtkPVApplication::SetProcessModule(vtkPVProcessModule *pm)
{
  this->ProcessModule = pm;
}

//----------------------------------------------------------------------------
extern "C" int Vtktkrenderwidget_Init(Tcl_Interp *interp);
extern "C" int Vtkkwparaviewtcl_Init(Tcl_Interp *interp);
extern "C" int Vtkpvfilterstcl_Init(Tcl_Interp *interp);

#ifdef PARAVIEW_LINK_XDMF
extern "C" int Vtkxdmftcl_Init(Tcl_Interp *interp);
#endif

vtkPVApplication* vtkPVApplication::MainApplication = 0;

static void vtkPVAppProcessMessage(vtkObject* vtkNotUsed(object),
                                   unsigned long vtkNotUsed(event), 
                                   void *clientdata, void *calldata)
{
  vtkPVApplication *self = static_cast<vtkPVApplication*>( clientdata );
  const char* message = static_cast<char*>( calldata );
  cout << "# Error or warning: " << message << endl;
  self->AddTraceEntry("# Error or warning:");
  int cc;
  ostrstream str;
  for ( cc= 0; cc < vtkString::Length(message); cc ++ )
    {
    str << message[cc];
    if ( message[cc] == '\n' )
      {
      str << "# ";
      }
    }
  str << ends;
  self->AddTraceEntry("# %s\n#", str.str());
  //cout << "# " << str.str() << endl;
  str.rdbuf()->freeze(0);
}

// initialze the class variables
int vtkPVApplication::GlobalLODFlag = 0;

// Output window which prints out the process id
// with the error or warning messages
class VTK_EXPORT vtkPVOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkPVOutputWindow,vtkOutputWindow);
  
  static vtkPVOutputWindow* New();

  void DisplayText(const char* t)
  {
    if ( this->Windows && this->Windows->GetNumberOfItems() &&
         this->Windows->GetLastKWWindow() )
      {
      vtkKWWindow *win = this->Windows->GetLastKWWindow();
      char buffer[4096];      
      const char *message = strstr(t, "): ");
      char type[1024], file[1024];
      int line;
      sscanf(t, "%[^:]: In %[^,], line %d", type, file, &line);
      if ( message )
        {
        int error = 0;
        if ( !strncmp(t, "ERROR", 5) )
          {
          error = 1;
          }
        message += 3;
        char *rmessage = vtkString::Duplicate(message);
        int last = vtkString::Length(rmessage)-1;
        while ( last > 0 && 
                (rmessage[last] == ' ' || rmessage[last] == '\n' || 
                 rmessage[last] == '\r' || rmessage[last] == '\t') )
          {
          rmessage[last] = 0;
          last--;
          }
        sprintf(buffer, "There was a VTK %s in file: %s (%d)\n %s", 
                (error ? "Error" : "Warning"),
                file, line,
                rmessage);
        if ( error )
          {
          win->ErrorMessage(buffer);
          if ( this->TestErrors )
            {
            this->ErrorOccurred = 1;
            }
          }
        else 
          {
          win->WarningMessage(buffer);
          }
        delete [] rmessage;
        }
      }
  }
  
  vtkPVOutputWindow()
  {
    this->Windows = 0;
    this->ErrorOccurred = 0;
    this->TestErrors = 1;
  }
  
  void SetWindowCollection(vtkKWWindowCollection *windows)
  {
    this->Windows = windows;
  }

  int GetErrorOccurred()
    {
    return this->ErrorOccurred;
    }

  void EnableTestErrors() { this->TestErrors = 1; }
  void DisableTestErrors() { this->TestErrors = 0; }
protected:
  vtkKWWindowCollection *Windows;
  int ErrorOccurred;
  int TestErrors;
private:
  vtkPVOutputWindow(const vtkPVOutputWindow&);
  void operator=(const vtkPVOutputWindow&);
};

vtkStandardNewMacro(vtkPVOutputWindow);

Tcl_Interp *vtkPVApplication::InitializeTcl(int argc, 
                                            char *argv[], 
                                            ostream *err)
{

  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc, argv, err);
  if (!interp)
    {
    return interp;
    }

  Vtkpvfilterstcl_Init(interp);
  //  if (Vtkparalleltcl_Init(interp) == TCL_ERROR) 
  //  {
   // cerr << "Init Parallel error\n";
   // }

  // Why is this here?  Doesn't the superclass initialize this?
  if (vtkKWApplication::GetWidgetVisibility())
    {
    Vtktkrenderwidget_Init(interp);
    }
   
  Vtkkwparaviewtcl_Init(interp);

#ifdef PARAVIEW_LINK_XDMF
  Vtkxdmftcl_Init(interp);
#endif
  
  // Create the component loader procedure in Tcl.
  char* script = vtkString::Duplicate(vtkPVApplication::LoadComponentProc);  
  if (Tcl_GlobalEval(interp, script) != TCL_OK)
    {
    if (err)
      {
      *err << Tcl_GetStringResult(interp) << endl;
      }
    // ????
    }  
  delete [] script;

  script = vtkString::Duplicate(vtkPVApplication::ExitProc);  
  if (Tcl_GlobalEval(interp, script) != TCL_OK)
    {
    if (err)
      {
      *err << Tcl_GetStringResult(interp) << endl;
      }
    // ????
    }  
  delete [] script;
  
  return interp;
}

//----------------------------------------------------------------------------
vtkPVApplication::vtkPVApplication()
{
  this->AlwaysSSH = 0;
  this->MajorVersion = PARAVIEW_VERSION_MAJOR;
  this->MinorVersion = PARAVIEW_VERSION_MINOR;
  this->SetApplicationName("ParaView");
  char name[128];
  sprintf(name, "ParaView%d.%d", this->MajorVersion, this->MinorVersion);
  this->SetApplicationVersionName(name);
  this->SetApplicationReleaseName("1");


  this->Display3DWidgets = 0;
  this->ProcessId = 0;
  this->RunningParaViewScript = 0;

  this->ProcessModule = NULL;
  this->RenderModule = NULL;
  this->RenderModuleName = NULL;
  // Now initialized in ParaView.cxx
  //this->SetRenderModuleName("LODRenderModule");
  this->CommandFunction = vtkPVApplicationCommand;

  this->NumberOfPipes = 1;

  this->UseRenderingGroup = 0;
  this->GroupFileName = 0;

  this->UseTiledDisplay = 0;
  this->TileDimensions[0] = this->TileDimensions[1] = 1;

  this->ClientMode = 0;
  this->ServerMode = 0;
  this->HostName = NULL;
  this->SetHostName("localhost");
  this->Port = 11111;
  this->ReverseConnection = 0;
  this->Username = 0;
  this->UseSoftwareRendering = 0;
  this->UseSatelliteSoftware = 0;
  this->UseStereoRendering = 0;
  this->UseOffscreenRendering = 0;
  this->StartEmpty = 0;
  this->PlayDemo = 0;

  // GUI style & consistency

  vtkKWLabeledFrame::AllowShowHideOn();
  vtkKWLabeledFrame::SetLabelCaseToLowercaseFirst();
  vtkKWLabeledFrame::BoldLabelOn();
  
  // The following is necessary to make sure that the tcl object
  // created has the right command function. Without this,
  // the tcl object has the vtkKWApplication's command function
  // since it is first created in vtkKWApplication's constructor
  // (in vtkKWApplication's constructor GetClassName() returns
  // the wrong value because the virtual table is not setup yet)
  char* tclname = vtkString::Duplicate(this->GetTclName());
  vtkTclUpdateCommand(this->MainInterp, tclname, this);
  delete[] tclname;

  this->HasSplashScreen = 1;

  this->TraceFileName = 0;
  this->Argv0 = 0;

  this->StartGUI = 1;

  this->BatchScriptName = 0;
  this->RunBatchScript = 0;

  this->SourcesBrowserAlwaysShowName = 0;
  this->ShowSourcesLongHelp = 1;
  this->DemoPath = NULL;

  this->LogThreshold = 0.01;
}

//----------------------------------------------------------------------------
vtkPVApplication::~vtkPVApplication()
{
  this->SetProcessModule(NULL);
  this->SetRenderModule(NULL);
  this->SetRenderModuleName(NULL);
  if ( this->TraceFile )
    {
    delete this->TraceFile;
    this->TraceFile = 0;
    }
  this->SetGroupFileName(0);
  this->SetTraceFileName(0);
  this->SetArgv0(0);
  this->SetHostName(NULL);
  this->SetUsername(0);
  this->SetDemoPath(NULL);
}


  
//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVApplication::GetController()
{
  return this->ProcessModule->GetController();
}

//----------------------------------------------------------------------------
vtkSocketController* vtkPVApplication::GetSocketController()
{
  vtkPVClientServerModule *csm;
  csm = vtkPVClientServerModule::SafeDownCast(this->ProcessModule);
  if (csm)
    {
    return csm->GetSocketController();
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkPVWindow *vtkPVApplication::GetMainWindow()
{
  this->Windows->InitTraversal();
  return (vtkPVWindow*)(this->Windows->GetNextItemAsObject());
}

//----------------------------------------------------------------------------
vtkPVRenderView *vtkPVApplication::GetMainView()
{
  return this->GetMainWindow()->GetMainView();
}


//----------------------------------------------------------------------------
void vtkPVApplication::BroadcastScript(char *format, ...)
{
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
  
  this->BroadcastSimpleScript(buffer);
  
  if(buffer != event)
    {
    delete [] buffer;
    }
}

//----------------------------------------------------------------------------
void vtkPVApplication::RemoteScript(int id, char *format, ...)
{
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
  
  this->RemoteSimpleScript(id, buffer);
  
  if(buffer != event)
    {
    delete [] buffer;
    }
}


//----------------------------------------------------------------------------
int vtkPVApplication::AcceptLicense()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::AcceptEvaluation()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::PromptRegistration(char* vtkNotUsed(name), 
                                         char* vtkNotUsed(IDS))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::CheckRegistration()
{
  return 1;
}

const char vtkPVApplication::ArgumentList[vtkPVApplication::NUM_ARGS][128] = 
{ "--client" , "-c", 
  "Run ParaView as client (MPI run, 1 process) (ParaView Server must be started first).", 
  "--server" , "-v", 
  "Start ParaView as a server (use MPI run).",
  "--host", "-h",
  "Tell the client where to look for the server (default: localhost). Used with --client option or --server -rc options.", 
  "--user", "",
  "Tell the client what username to send to server when establishing SSH connection.",
  "--always-ssh", "",
  "",
  "--port", "",
  "Specify the port client and server will use (--port=11111).  Client and servers ports must match.", 
  "--reverse-connection", "-rc",
  "Have the server connect to the client.", 
  "--stereo", "",
  "Tell the application to enable stero rendering (only when running on a single process).",
  "--render-module", "",
  "User specified rendering module (--render-module=...)",
  "--start-empty" , "-e", 
  "Start ParaView without any default modules.", 
  "--disable-registry", "-dr", 
  "Do not use registry when running ParaView (for testing).", 
  "--batch", "-b", 
  "Load and run the batch script specified.", 
#ifdef VTK_USE_MPI
/* Temporarily disabling - for release
  "--use-rendering-group", "-p",
  "Use a subset of processes to render.",
  "--group-file", "-gf",
  "--group-file=fname where fname is the name of the input file listing number of processors to render on.",
*/
  "--use-tiled-display", "-td",
  "Duplicate the final data to all nodes and tile node displays 1-N into one large display.",
  "--tile-dimensions-x", "-tdx",
  "-tdx=X where X is number of displays in each row of the display.",
  "--tile-dimensions-y", "-tdy",
  "-tdy=Y where Y is number of displays in each column of the display.",
#endif
#ifdef VTK_USE_MANGLED_MESA
  "--use-software-rendering", "-r", 
  "Use software (Mesa) rendering (supports off-screen rendering).", 
  "--use-satellite-software", "-s", 
  "Use software (Mesa) rendering (supports off-screen rendering) only on satellite processes.", 
#endif
  "--use-offscreen-rendering", "-os", 
  "Render offscreen on the satellite processes. This option only works with software rendering or mangled mesa on Unix", 
  "--play-demo", "-pd",
  "Run the ParaView demo.",
  "--help", "",
  "Displays available command line arguments.",
  "" 
};

char* vtkPVApplication::CreateHelpString()
{
  ostrstream error;
  error << "Valid arguments are: " << endl;

  int j=0;
  const char* argument1 = vtkPVApplication::ArgumentList[j];
  const char* argument2 = vtkPVApplication::ArgumentList[j+1];
  const char* help = vtkPVApplication::ArgumentList[j+2];
  while (argument1 && argument1[0])
    {
    if ( strlen(help) > 0 )
      {
      error << argument1;
      if (argument2[0])
        {
        error << ", " << argument2;
        }
      error << " : " << help << endl;
      }
    j += 3;
    argument1 = vtkPVApplication::ArgumentList[j];
    if (argument1 && argument1[0]) 
      {
      argument2 = vtkPVApplication::ArgumentList[j+1];
      help = vtkPVApplication::ArgumentList[j+2];
      }
    }
  error << ends;
  return error.str();
  
}

//----------------------------------------------------------------------------
int vtkPVApplication::CheckForExtension(const char* arg,
                                        const char* ext)
{
  if (!ext)
    {
    return 0;
    }
  int extLen = static_cast<int>(strlen(ext));
  if (extLen <= 0)
    {
    return 0;
    }
  if (!arg || static_cast<int>(strlen(arg)) < extLen)
    {
    return 0;
    }
  if (strcmp(arg + strlen(arg) - extLen, ext) == 0)
    {
    return 1;
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVApplication::SetEnvironmentVariable(const char* str)
{ 
  char* envstr = vtkString::Duplicate(str);
  putenv(envstr);
}

//----------------------------------------------------------------------------
void vtkPVApplication::DeleteTraceFiles(char* name, int all)
{
  if ( !all )
    {
    unlink(name);
    return;
    }
  char buf[256];
  if ( vtkDirectory::GetCurrentWorkingDirectory(buf, 256) )
    {
    vtkDirectory* dir = vtkDirectory::New();
    if ( !dir->Open(buf) )
      {
      dir->Delete();
      return;
      }
    int numFiles = dir->GetNumberOfFiles();
    int len = (int)(strlen("ParaViewTrace"));
    for(int i=0; i<numFiles; i++)
      {
      const char* fname = dir->GetFile(i);
      if ( strncmp(fname, "ParaViewTrace", len) == 0 )
        {
        unlink(fname);
        }
      }
    dir->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkPVApplication::CheckForTraceFile(char* name, unsigned int maxlen)
{
  if ( this->GetRegisteryValue(2,"RunTime", VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY, 0) &&
    !this->GetIntRegisteryValue(2,"RunTime", VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY) )
    {
    return 0;
    }
  char buf[256];
  if ( vtkDirectory::GetCurrentWorkingDirectory(buf, 256) )
    {
    vtkDirectory* dir = vtkDirectory::New();
    if ( !dir->Open(buf) )
      {
      dir->Delete();
      return 0;
      }
    int retVal = 0;
    int numFiles = dir->GetNumberOfFiles();
    int len = (int)(strlen("ParaViewTrace"));
    for(int i=0; i<numFiles; i++)
      {
      const char* fname = dir->GetFile(i);
      if ( strncmp(fname, "ParaViewTrace", len) == 0 )
        {
        if ( retVal == 0 )
          {
          strncpy(name, fname, maxlen);
          }
        retVal++;
        }
      }
    dir->Delete();
    return retVal;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVApplication::SaveTraceFile(const char* fname)
{
  vtkKWLoadSaveDialog* exportDialog = vtkKWLoadSaveDialog::New();
  this->GetMainWindow()->RetrieveLastPath(exportDialog, "SaveTracePath");
  exportDialog->SetParent(this->GetMainWindow());
  exportDialog->Create(this, 0);
  exportDialog->SaveDialogOn();
  exportDialog->SetTitle("Save ParaView Trace");
  exportDialog->SetDefaultExtension(".pvs");
  exportDialog->SetFileTypes("{{ParaView Scripts} {.pvs}} {{All Files} {*}}");
  if ( exportDialog->Invoke() && 
       vtkString::Length(exportDialog->GetFileName())>0 )
    {
    if (rename(fname, exportDialog->GetFileName()) != 0)
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetApplication(), this->GetMainWindow(),
        "Error Saving", "Could not save trace file.",
        vtkKWMessageDialog::ErrorIcon);
      }
    else
      {
      this->GetMainWindow()->SaveLastPath(exportDialog, "SaveTracePath");
      }
    }
  exportDialog->Delete();
}

//----------------------------------------------------------------------------
int vtkPVApplication::ParseCommandLineArguments(int argc, char*argv[])
{
  int i;
  int index=-1;

  // This should really be part of Parsing !!!!!!
  if (argv)
    {
    this->SetArgv0(argv[0]);
    }

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--batch",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-b",
                                          index) == VTK_OK )
    {
    for (i=1; i < argc; i++)
      {
      if (vtkPVApplication::CheckForExtension(argv[i], ".pvb"))
        {
        this->RunBatchScript = 1;
        this->SetBatchScriptName(argv[i]);
        break;
        }
      }
    return 0;
    }

  for (i=1; i < argc; i++)
    {
    if ( vtkPVApplication::CheckForExtension(argv[i], ".pvs") )
      {
      this->RunningParaViewScript = 1;
      break;
      }
    }

  if (!this->RunningParaViewScript)
    {
    for (i=1; i < argc; i++)
      {
      int valid=0;
      if (argv[i])
        {
        int  j=0;
        const char* argument1 = vtkPVApplication::ArgumentList[j];
        const char* argument2 = vtkPVApplication::ArgumentList[j+1];
        while (argument1 && argument1[0])
          {

          char* newarg = vtkString::Duplicate(argv[i]);
          int len = (int)(strlen(newarg));
          for (int ik=0; ik<len; ik++)
            {
            if (newarg[ik] == '=')
              {
              newarg[ik] = '\0';
              }
            }

          if ( strcmp(newarg, argument1) == 0 || 
               strcmp(newarg, argument2) == 0)
            {
            valid = 1;
            }
          delete[] newarg;
          j += 3;
          argument1 = vtkPVApplication::ArgumentList[j];
          if (argument1 && argument1[0]) 
            {
            argument2 = vtkPVApplication::ArgumentList[j+1];
            }
          }
        }
      if (!valid)
        {
        char* error = this->CreateHelpString();
        vtkErrorMacro("Unrecognized argument " << argv[i] << "." << endl
                      << error);
        delete[] error;
        return 1;
        }
      }
    }

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--help",
                                          index) == VTK_OK )
    {
    char* error = this->CreateHelpString();
    vtkWarningMacro(<<error);
    delete[] error;
    return 1;
    }

  this->PlayDemo = 0;
  if ( vtkPVApplication::CheckForArgument(argc, argv, "--play-demo",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-pd",
                                          index) == VTK_OK )
    {
    this->PlayDemo = 1;
    }

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--disable-registry",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-dr",
                                          index) == VTK_OK )
    {
    this->RegisteryLevel = 0;
    }

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--stereo",
                                          index) == VTK_OK)
    {
    this->UseStereoRendering = 1;
    }

#ifdef VTK_USE_MPI
// Temporarily removing this (for the release - it has bugs)
//    if ( vtkPVApplication::CheckForArgument(argc, argv, "--use-rendering-group",
//                                            index) == VTK_OK ||
//         vtkPVApplication::CheckForArgument(argc, argv, "-p",
//                                            index) == VTK_OK )
//      {
//      this->UseRenderingGroup = 1;
//      }

//    if ( vtkPVApplication::CheckForArgument(argc, argv, "--group-file",
//                                            index) == VTK_OK ||
//         vtkPVApplication::CheckForArgument(argc, argv, "-gf",
//                                            index) == VTK_OK )
//      {
//      const char* newarg=0;
//      int len = (int)(strlen(argv[index]));
//      for (int i=0; i<len; i++)
//        {
//        if (argv[index][i] == '=')
//          {
//          newarg = &(argv[index][i+1]);
//          }
//        }
//      this->SetGroupFileName(newarg);
//      }

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--use-tiled-display",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-td",
                                          index) == VTK_OK )
    {
    this->UseTiledDisplay = 1;

    if ( vtkPVApplication::CheckForArgument(argc, argv, "--tile-dimensions-x",
                                            index) == VTK_OK ||
         vtkPVApplication::CheckForArgument(argc, argv, "-tdx",
                                          index) == VTK_OK )
      {
      // Strip string to equals sign.
      const char* newarg=0;
      int len = (int)(strlen(argv[index]));
      for (i=0; i<len; i++)
        {
        if (argv[index][i] == '=')
          {
          newarg = &(argv[index][i+1]);
          }
        }
      this->TileDimensions[0] = atoi(newarg);
      }
    if ( vtkPVApplication::CheckForArgument(argc, argv, "--tile-dimensions-y",
                                            index) == VTK_OK ||
         vtkPVApplication::CheckForArgument(argc, argv, "-tdy",
                                          index) == VTK_OK )
      {
      // Strip string to equals sign.
      const char* newarg=0;
      int len = (int)(strlen(argv[index]));
      for (i=0; i<len; i++)
        {
        if (argv[index][i] == '=')
          {
          newarg = &(argv[index][i+1]);
          }
        }
      this->TileDimensions[1] = atoi(newarg);
      }
    }
#endif


  if ( vtkPVApplication::CheckForArgument(argc, argv, "--client",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-c",
                                          index) == VTK_OK )
    {
    this->ClientMode = 1;

    if ( vtkPVApplication::CheckForArgument(argc, argv, "--user",
                                            index) == VTK_OK )
      {
      // Strip string to equals sign.
      const char* newarg=0;
      int len = (int)(strlen(argv[index]));
      for (i=0; i<len; i++)
        {
        if (argv[index][i] == '=')
          {
          newarg = &(argv[index][i+1]);
          }
        }
      this->SetUsername(newarg);
      }
    }

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--server",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-v",
                                          index) == VTK_OK )
    {
    this->ServerMode = 1;
    }

  if (this->ServerMode || this->ClientMode)
    {
    if ( vtkPVApplication::CheckForArgument(argc, argv, "--host",
                                            index) == VTK_OK ||
         vtkPVApplication::CheckForArgument(argc, argv, "-h",
                                          index) == VTK_OK )
      {
      // Strip string to equals sign.
      const char* newarg=0;
      int len = (int)(strlen(argv[index]));
      for (i=0; i<len; i++)
        {
        if (argv[index][i] == '=')
          {
          newarg = &(argv[index][i+1]);
          }
        }
      this->SetHostName(newarg);
      }

    if ( vtkPVApplication::CheckForArgument(argc, argv, "--port",
                                            index) == VTK_OK)
      {
      // Strip string to equals sign.
      const char* newarg=0;
      int len = (int)(strlen(argv[index]));
      for (i=0; i<len; i++)
        {
        if (argv[index][i] == '=')
          {
          newarg = &(argv[index][i+1]);
          }
        }
      this->Port = atoi(newarg);
      }
    // Change behavior so server connects to the client.
    if ( vtkPVApplication::CheckForArgument(argc, argv, "--reverse-connection",
                                            index) == VTK_OK ||
         vtkPVApplication::CheckForArgument(argc, argv, "-rc",
                                            index) == VTK_OK )
      {
      this->ReverseConnection = 1;
      }
    }

  if (this->ClientMode)
    {
    if ( vtkPVApplication::CheckForArgument(argc, argv, "--always-ssh",
                                            index) == VTK_OK)
      {
      this->AlwaysSSH = 1;
      }
    }

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--start-empty", index) 
       == VTK_OK || 
       vtkPVApplication::CheckForArgument(argc, argv, "-e", index) 
       == VTK_OK)
    {
    this->StartEmpty = 1;
    }

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--render-module",
                                            index) == VTK_OK)
    {
    // Strip string to equals sign.
    const char* newarg=0;
    int len = (int)(strlen(argv[index]));
    for (i=0; i<len; i++)
      {
      if (argv[index][i] == '=')
        {
        newarg = &(argv[index][i+1]);
        }
      }
    this->SetRenderModuleName(newarg);
    }
    
#ifdef VTK_USE_MANGLED_MESA
  
  if ( vtkPVApplication::CheckForArgument(argc, argv, "--use-software-rendering",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-r",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "--use-satellite-software",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-s",
                                          index) == VTK_OK ||
       getenv("PV_SOFTWARE_RENDERING") )
    {
    this->UseSoftwareRendering = 1;
    if ( getenv("PV_SOFTWARE_RENDERING") ||
         vtkPVApplication::CheckForArgument(
           argc, argv, "--use-satellite-software", index) == VTK_OK ||
         vtkPVApplication::CheckForArgument(argc, argv, "-s",
                                            index) == VTK_OK)
      {
      this->UseSatelliteSoftware = 1;
      }
    }
#endif

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--offscreen-rendering",
                                          index) == VTK_OK ||
       vtkPVApplication::CheckForArgument(argc, argv, "-os",
                                          index) == VTK_OK ||
       getenv("PV_OFFSCREEN") )
    {
    this->UseOffscreenRendering = 1;
    }

  return 0;
}


//----------------------------------------------------------------------------
void vtkPVApplication::Start(int argc, char*argv[])
{

  // Find the installation directory (now that we have the app name)
  this->FindApplicationInstallationDirectory();

  if (this->RunBatchScript)
    {
    this->BroadcastScript("$Application LoadScript {%s}", 
                          this->GetBatchScriptName());
    this->Exit();
    return;
    }

  // set the font size to be small
#ifdef _WIN32
  this->Script("option add *font {{Tahoma} 8}");
#else
  // Specify a font only if there isn't one in the database
  this->Script(
    "toplevel .tmppvwindow -class ParaView;"
    "if {[option get .tmppvwindow font ParaView] == \"\"} {"
    "option add *font "
    "-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1};"
    "destroy .tmppvwindow");
  this->Script("option add *highlightThickness 0");
  this->Script("option add *highlightBackground #ccc");
  this->Script("option add *activeBackground #eee");
  this->Script("option add *activeForeground #000");
  this->Script("option add *background #ccc");
  this->Script("option add *foreground #000");
  this->Script("option add *Entry.background #ffffff");
  this->Script("option add *Text.background #ffffff");
  this->Script("option add *Button.padX 6");
  this->Script("option add *Button.padY 3");
  this->Script("option add *selectColor #666");
#endif

#ifdef VTK_USE_MANGLED_MESA
  if (this->UseSoftwareRendering)
    {
    this->BroadcastScript("vtkGraphicsFactory _graphics_fact\n"
                          "_graphics_fact SetUseMesaClasses 1\n"
                          "_graphics_fact Delete");
    this->BroadcastScript("vtkImagingFactory _imaging_fact\n"
                          "_imaging_fact SetUseMesaClasses 1\n"
                          "_imaging_fact Delete");
    if (this->UseSatelliteSoftware)
      {
      this->Script("vtkGraphicsFactory _graphics_fact\n"
                   "_graphics_fact SetUseMesaClasses 0\n"
                   "_graphics_fact Delete");
      this->Script("vtkImagingFactory _imaging_fact\n"
                   "_imaging_fact SetUseMesaClasses 0\n"
                   "_imaging_fact Delete");
      }
    }
#endif


// Temporarily removing this (for the release - it has bugs)
//    // Handle setting up the SGI pipes.
//    if (this->UseRenderingGroup)
//      {
//      int numProcs = this->ProcessModule->GetNumberOfPartitions();
//      int numPipes = 1;
//      int id;
//      int fileFound=0;
//      // Until I add a user interface to set the number of pipes,
//      // just read it from a file.
//      if (this->GroupFileName)
//        {
//        ifstream ifs;
//        ifs.open(this->GroupFileName,ios::in);
//        if (ifs.fail())
//          {
//          vtkErrorMacro("Could not find the file " << this->GroupFileName);
//          }
//        else
//          {
//          char bfr[1024];
//          ifs.getline(bfr, 1024);
//          numPipes = atoi(bfr);
//          if (numPipes > numProcs) { numPipes = numProcs; }
//          if (numPipes < 1) { numPipes = 1; }
//          this->BroadcastScript("$Application SetNumberOfPipes %d", numPipes);   
        
//          for (id = 0; id < numPipes; ++id)
//            {
//            ifs.getline(bfr, 1024);
//            this->RemoteScript(
//              id, "$Application SetEnvironmentVariable {DISPLAY=%s}", 
//              bfr);
//            }
//          fileFound = 1;
//          }
//        }

    
//      if (!fileFound)
//        {
//        numPipes = numProcs;
//        vtkPVRenderGroupDialog *rgDialog = vtkPVRenderGroupDialog::New();
//        const char *displayString;
//        displayString = getenv("DISPLAY");
//        if (displayString)
//          {
//          rgDialog->SetDisplayString(0, displayString);
//          }
//        rgDialog->SetNumberOfProcessesInGroup(numPipes);
//        rgDialog->Create(this);
      
//        rgDialog->Invoke();
//        numPipes = rgDialog->GetNumberOfProcessesInGroup();
      
//        this->BroadcastScript("$Application SetNumberOfPipes %d", numPipes);    
      
//        if (displayString)
//          {    
//          for (id = 1; id < numPipes; ++id)
//            {
//            // Format a new display string based on process.
//            displayString = rgDialog->GetDisplayString(id);
//            this->RemoteScript(
//              id, "$Application SetEnvironmentVariable {DISPLAY=%s}", 
//              displayString);
//            }
//          }
//        rgDialog->Delete();
//        }
//      }

  // Create the rendering module here.
  char* rmClassName;
  rmClassName = new char[strlen(this->RenderModuleName) + 20];
  sprintf(rmClassName, "vtkPV%s", this->RenderModuleName);
  vtkObject* o = vtkInstantiator::CreateInstance(rmClassName);
  vtkPVRenderModule* rm = vtkPVRenderModule::SafeDownCast(o);
  if (rm == NULL)
    {
    vtkErrorMacro("Could not create render module " << rmClassName);
    this->SetRenderModuleName("RenderModule");
    o = vtkInstantiator::CreateInstance("vtkPVRenderModule");
    rm = vtkPVRenderModule::SafeDownCast(o);
    }
  this->SetRenderModule(rm);
  rm->SetPVApplication(this);
  o->Delete();
  o = NULL;
  rm = NULL;

  delete [] rmClassName;
  rmClassName = NULL;
  if ( !this->RenderModule )
    {
    return;
    }

  vtkstd::vector<vtkstd::string> open_files;
  // If any of the arguments has a .pvs extension, load it as a script.
  int i;
  for (i=1; i < argc; i++)
    {
    if (vtkPVApplication::CheckForExtension(argv[i], ".pvs"))
      {
      if ( !vtkKWDirectoryUtilities::FileExists(argv[i]) )
        {
        vtkErrorMacro("Could not find the script file: " << argv[i]);
        this->SetRenderModule(NULL);
        this->SetExitStatus(1);
        this->Exit();
        return;
        }
      else
        {
        open_files.push_back(argv[i]);
        }
      }
    }

  vtkOutputWindow::GetInstance()->PromptUserOn();

  // Splash screen ?

  if (this->ShowSplashScreen)
    {
    this->CreateSplashScreen();
    this->SplashScreen->SetProgressMessage("Initializing application...");
    }

  // Application Icon 
#ifdef _WIN32
  this->Script("SetApplicationIcon {} %d big",
               IDI_PARAVIEWICO32);
  // No, we can't set the same icon, even if it has both 32x32 and 16x16
  this->Script("SetApplicationIcon {} %d small",
               IDI_PARAVIEWICO16);
#endif

  vtkPVWindow *ui = vtkPVWindow::New();
  this->AddWindow(ui);

  vtkCallbackCommand *ccm = vtkCallbackCommand::New();
  ccm->SetClientData(this);
  ccm->SetCallback(::vtkPVAppProcessMessage);  
  ui->AddObserver(vtkKWEvent::WarningMessageEvent, ccm);
  ui->AddObserver(vtkKWEvent::ErrorMessageEvent, ccm);
  ccm->Delete();

  if (this->ShowSplashScreen)
    {
    this->SplashScreen->SetProgressMessage("Creating icons...");
    }

  this->CreateButtonPhotos();

  if (this->StartEmpty)
    {
    ui->InitializeDefaultInterfacesOff();
    }

  if (this->ShowSplashScreen)
    {
    this->SplashScreen->SetProgressMessage("Creating UI...");
    }

  ui->Create(this,"");

  // ui has ref. count of at least 1 because of AddItem() above
  ui->Delete();

  this->Script("proc bgerror { m } "
               "{ global Application; $Application DisplayTCLError $m }");
  vtkPVOutputWindow *window = vtkPVOutputWindow::New();
  window->SetWindowCollection( this->Windows );
  this->OutputWindow = window;
  vtkOutputWindow::SetInstance(this->OutputWindow);

  // Check if there is an existing ParaViewTrace file.
  if (this->ShowSplashScreen)
    {
    this->SplashScreen->SetProgressMessage("Looking for old trace files...");
    }
  char traceName[128];
  int foundTrace = this->CheckForTraceFile(traceName, 128);

  if (this->ShowSplashScreen)
    {
    this->SplashScreen->Hide();
    }

  // If there is already an existing trace file, ask the
  // user what she wants to do with it.
  if (foundTrace && ! this->RunningParaViewScript) 
    {
    vtkPVTraceFileDialog *dlg2 = vtkPVTraceFileDialog::New();
    dlg2->SetMasterWindow(ui);
    dlg2->Create(this,"");
    ostrstream str;
    str << "Do you want to save the existing tracefile?\n\n"
        << "A tracefile called " << traceName << " was found in "
        << "the current directory. This might mean that there is "
        << "another instance of ParaView running or ParaView crashed "
        << "previously and failed to delete it.\n"
        << "If ParaView crashed previously, this tracefile can "
        << "be useful in tracing the problem and you should consider "
        << "saving it.\n"
        << "If there is another instance of ParaView running, select "
        << "\"Do Nothing\" to avoid potential problems."
        << ends;
    dlg2->SetText(str.str());
    str.rdbuf()->freeze(0);
    dlg2->SetTitle("Tracefile found");
    dlg2->SetIcon();
    int shouldSave = dlg2->Invoke();
    dlg2->Delete();
    if (shouldSave == 2)
      {
      this->SaveTraceFile(traceName);
      }
    else if (shouldSave == 1)
      {
      if ( foundTrace > 1 )
        {
        char buffer[1024];
        sprintf(buffer, "Do you want to delete all the tracefiles? Answering No will only delete \"%s\".",
          traceName);
        int all = vtkKWMessageDialog::PopupYesNo(
          this, ui, "Delete Trace Files",
          buffer,
          vtkKWMessageDialog::QuestionIcon);
        this->DeleteTraceFiles(traceName, all);
        }
      else
        {
        this->DeleteTraceFiles(traceName, 0);
        }
      }
    // Having trouble with tiled display.
    // Not all tiles rendered properly after this dialoge at startup.
    // It must have been a race condition.
    this->GetMainView()->EventuallyRender();
    }

  // Open the trace file.
  int count = 0;
  struct stat fs;
  while(1)
    {
    ostrstream str;
    str << "ParaViewTrace" << count++ << ".pvs" << ends;
    if ( stat(str.str(), &fs) != 0 || count >= 10 )
      {
      this->SetTraceFileName(str.str());
      str.rdbuf()->freeze(0);
      break;
      }
    str.rdbuf()->freeze(0);
    }

  this->TraceFile = new ofstream(this->TraceFileName, ios::out);
    
  // Initialize a couple of variables in the trace file.
  this->AddTraceEntry("set kw(%s) [$Application GetMainWindow]",
                      ui->GetTclName());
  ui->SetTraceInitialized(1);
  // We have to set this variable after the window variable is set,
  // so it has to be done here.
  this->AddTraceEntry("set kw(%s) [$kw(%s) GetMainView]",
                      ui->GetMainView()->GetTclName(), ui->GetTclName());
  ui->GetMainView()->SetTraceInitialized(1);

  vtkstd::vector<vtkstd::string>::size_type cc;
  for ( cc = 0; cc < open_files.size(); cc ++ )
    {
    this->RunningParaViewScript = 1;
    ui->LoadScript(open_files[cc].c_str());
    this->RunningParaViewScript = 0;
    }
  
  if (this->PlayDemo)
    {
    this->Script("set pvDemoCommandLine 1");
    ui->PlayDemo();
    }
  else
    {
    this->vtkKWApplication::Start(argc,argv);
    }
  vtkOutputWindow::SetInstance(0);
  this->OutputWindow->Delete();

  // Circular reference.
  this->SetRenderModule(NULL);
}

//----------------------------------------------------------------------------
void vtkPVApplication::GetApplicationSettingsFromRegistery()
{ 
  this->Superclass::GetApplicationSettingsFromRegistery();

  // Show sources description ?

  if (this->HasRegisteryValue(
    2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY))
    {
    this->ShowSourcesLongHelp = this->GetIntRegisteryValue(
      2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY);
    }

  // Show name in sources description browser

  if (this->HasRegisteryValue(
    2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY))
    {
    this->SourcesBrowserAlwaysShowName = this->GetIntRegisteryValue(
      2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY);
    }
}

//-----------------------------------------------------------------------------
void vtkPVApplication::SetShowSourcesLongHelp(int v)
{
  if (this->ShowSourcesLongHelp == v)
    {
    return;
    }

  this->ShowSourcesLongHelp = v;
  this->Modified();

  // Update the properties of all the sources previously created
  // so that the Description label can be removed/brought back.
  
  if (this->GetMainWindow())
    {
    vtkPVSource *pvs;
    vtkPVSourceCollection *col = 
      this->GetMainWindow()->GetSourceList("Sources");
    vtkCollectionIterator *cit = col->NewIterator();
    cit->InitTraversal();
    while (!cit->IsDoneWithTraversal())
      {
      pvs = static_cast<vtkPVSource*>(cit->GetObject()); 
      pvs->UpdateProperties();
      cit->GoToNextItem();
      }
    cit->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVApplication::SetSourcesBrowserAlwaysShowName(int v)
{
  if (this->SourcesBrowserAlwaysShowName == v)
    {
    return;
    }

  this->SourcesBrowserAlwaysShowName = v;
  this->Modified();

  if (this->GetMainView())
    {
    this->GetMainView()->SetSourcesBrowserAlwaysShowName(v);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplication::Exit()
{  
  // Avoid recursive exit calls during window destruction.
  if (this->InExit)
    {
    return;
    }
  
  // If errors were reported to the output window, return a bad
  // status.
  if(vtkPVOutputWindow* w =
     vtkPVOutputWindow::SafeDownCast(vtkOutputWindow::GetInstance()))
    {
    if(w->GetErrorOccurred())
      {
      this->SetExitStatus(1);
      }
    }
  
  // Remove the ParaView output window so errors during exit will
  // still be displayed.
  vtkOutputWindow::SetInstance(0);

  // Try to get the render window to destruct before the render widget.
  this->SetRenderModule(NULL);
  
  this->vtkKWApplication::Exit();

  this->ProcessModule->Exit();

  // This is a normal exit.  Close trace file here and delete it.
  if (this->TraceFile)
    {
    this->TraceFile->close();
    delete this->TraceFile;
    this->TraceFile = NULL;
    }
  if (this->TraceFileName)
    {
    unlink(this->TraceFileName);
    }
}


//----------------------------------------------------------------------------
void vtkPVApplication::StartRecordingScript(char *filename)
{
  if (this->TraceFile)
    {
    *this->TraceFile << "$Application StartRecordingScript " << filename << endl;
    this->StopRecordingScript();
    }

  this->TraceFile = new ofstream(filename, ios::out);
  if (this->TraceFile && this->TraceFile->fail())
    {
    vtkErrorMacro("Could not open trace file " << filename);
    delete this->TraceFile;
    this->TraceFile = NULL;
    return;
    }

  // Initialize a couple of variables in the trace file.
  this->AddTraceEntry("set kw(%s) [$Application GetMainWindow]",
                      this->GetMainWindow()->GetTclName());
  this->GetMainWindow()->SetTraceInitialized(1);
  this->SetTraceFileName(filename);
}

//----------------------------------------------------------------------------
void vtkPVApplication::StopRecordingScript()
{
  if (this->TraceFile)
    {
    this->TraceFile->close();
    delete this->TraceFile;
    this->TraceFile = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkPVApplication::SetGlobalLODFlag(int val)
{
  if (vtkPVApplication::GlobalLODFlag == val)
    {
    return;
    }

  this->ProcessModule->BroadcastScript(
                        "$Application SetGlobalLODFlagInternal %d",val);
}

 
//----------------------------------------------------------------------------
void vtkPVApplication::SetGlobalLODFlagInternal(int val)
{
  vtkPVApplication::GlobalLODFlag = val;
}





//----------------------------------------------------------------------------
int vtkPVApplication::GetGlobalLODFlag()
{
  return vtkPVApplication::GlobalLODFlag;
}


//============================================================================
// Make instances of sources.
//============================================================================

//----------------------------------------------------------------------------
vtkObject *vtkPVApplication::MakeTclObject(const char *className,
                                           const char *tclName)
{
  this->BroadcastScript("%s %s", className, tclName);
  return this->TclToVTKObject(tclName);
}

//----------------------------------------------------------------------------
void vtkPVApplication::MakeServerTclObject(const char *className,
                                           const char *tclName)
{
  this->GetProcessModule()->ServerScript("%s %s", className, tclName);
}


//----------------------------------------------------------------------------
vtkObject *vtkPVApplication::TclToVTKObject(const char *tclName)
{
  vtkObject *o;
  int error;

  o = (vtkObject *)(vtkTclGetPointerFromObject(
                      tclName, "vtkObject", this->GetMainInterp(), error));
  
  if (o == NULL)
    {
    vtkErrorMacro("Could not get pointer from object \""
                  << tclName << "\"");
    }
  
  return o;
}

void vtkPVApplication::DisplayHelp(vtkKWWindow* master)
{
#ifdef _WIN32
  ostrstream temp;
  if (this->ApplicationInstallationDirectory)
    {
    temp << this->ApplicationInstallationDirectory << "/";
    }
  temp << this->ApplicationName << ".chm" << ends;

  struct stat fs;
  if (stat(temp.str(), &fs) == 0) 
    {
    HtmlHelp(NULL, temp.str(), HH_DISPLAY_TOPIC, 0);
    temp.rdbuf()->freeze(0);
    return;
    }
  temp.rdbuf()->freeze(0);

  const char** dir;
  for(dir=VTK_PV_HELP_PATHS; *dir; ++dir)
    {
    ostrstream temp2;
    temp2 << *dir << "/" << this->ApplicationName << ".chm";
    if (stat(temp2.str(), &fs) == 0) 
      {
      HtmlHelp(NULL, temp2.str(), HH_DISPLAY_TOPIC, 0);
      temp2.rdbuf()->freeze(0);
      return;
      }
    temp2.rdbuf()->freeze(0);
    }

  vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
  dlg->SetTitle("ParaView Help");
  dlg->SetMasterWindow(master);
  dlg->Create(this,"");
  dlg->SetText(
    "The help file could not be found. Please make sure that ParaView "
    "is installed properly.");
  dlg->Invoke();  
  dlg->Delete();
  return;
    
#else

  vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
  dlg->SetTitle("ParaView Help");
  dlg->SetMasterWindow(master);
  dlg->Create(this,"");
  dlg->SetText(
    "HTML help is included in the Documentation/HTML subdirectory of"
    "this application. You can view this help using a standard web browser.");
  dlg->Invoke();  
  dlg->Delete();


#endif
}

//----------------------------------------------------------------------------
void vtkPVApplication::LogStartEvent(char* str)
{
  vtkTimerLog::MarkStartEvent(str);
}

//----------------------------------------------------------------------------
void vtkPVApplication::LogEndEvent(char* str)
{
  vtkTimerLog::MarkEndEvent(str);
}

#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
//----------------------------------------------------------------------------
void vtkPVApplication::SetupTrapsForSignals(int nodeid)
{
  vtkPVApplication::MainApplication = this;
#ifndef _WIN32
  signal(SIGHUP, vtkPVApplication::TrapsForSignals);
#endif
  signal(SIGINT, vtkPVApplication::TrapsForSignals);
  if ( nodeid == 0 )
    {
    signal(SIGILL,  vtkPVApplication::TrapsForSignals);
    signal(SIGABRT, vtkPVApplication::TrapsForSignals);
    signal(SIGSEGV, vtkPVApplication::TrapsForSignals);

#ifndef _WIN32
    signal(SIGQUIT, vtkPVApplication::TrapsForSignals);
    signal(SIGPIPE, vtkPVApplication::TrapsForSignals);
    signal(SIGBUS,  vtkPVApplication::TrapsForSignals);
#endif
    }
}

//----------------------------------------------------------------------------
void vtkPVApplication::TrapsForSignals(int signal)
{
  if ( !vtkPVApplication::MainApplication )
    {
    exit(1);
    }
  
  switch ( signal )
    {
#ifndef _WIN32
    case SIGHUP:
      return;
      break;
#endif
    case SIGINT:
#ifndef _WIN32
    case SIGQUIT: 
#endif
      break;
    case SIGILL:
    case SIGABRT: 
    case SIGSEGV:
#ifndef _WIN32
    case SIGPIPE:
    case SIGBUS:
#endif
      vtkPVApplication::ErrorExit(); 
      break;      
    }
  
  if ( vtkPVApplication::MainApplication->GetProcessId() )
    {
    return;
    }
  vtkPVWindow *win = vtkPVApplication::MainApplication->GetMainWindow();
  if ( !win )
    {
    cout << "Call exit on application" << endl;
    vtkPVApplication::MainApplication->Exit();
    }
  cout << "Call exit on window" << endl;
  win->Exit();
}

//----------------------------------------------------------------------------
void vtkPVApplication::ErrorExit()
{
  // This { is here because compiler is smart enough to know that exit
  // exits the code without calling destructors. By adding this,
  // destructors are called before the exit.
  {
  cout << "There was a major error! Trying to exit..." << endl;
  char name[] = "ErrorApplication";
  char *n = name;
  char** args = &n;
  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(1, args);
  ostrstream str;
  char buffer[1024];
#ifdef _WIN32
  _getcwd(buffer, 1023);
#else
  getcwd(buffer, 1023);
#endif

  Tcl_GlobalEval(interp, "wm withdraw .");
#ifdef _WIN32
  str << "option add *font {{MS Sans Serif} 8}" << endl;
#else
  str << "option add *font "
    "-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1" << endl;
  str << "option add *highlightThickness 0" << endl;
  str << "option add *highlightBackground #ccc" << endl;
  str << "option add *activeBackground #eee" << endl;
  str << "option add *activeForeground #000" << endl;
  str << "option add *background #ccc" << endl;
  str << "option add *foreground #000" << endl;
  str << "option add *Entry.background #ffffff" << endl;
  str << "option add *Text.background #ffffff" << endl;
  str << "option add *Button.padX 6" << endl;
  str << "option add *Button.padY 3" << endl;
#endif
  str << "tk_messageBox -type ok -message {It looks like ParaView "
      << "or one of its libraries performed an illegal opeartion and "
      << "it will be terminated. Please report this error to "
      << "bug-report@kitware.com. You may want to include a small "
      << "description of what you did when this happened and your "
      << "ParaView trace file: " << buffer
      << "/ParaViewTrace.pvs} -icon error"
      << ends;
  Tcl_GlobalEval(interp, str.str());
  str.rdbuf()->freeze(0);
  }
  exit(1);
}
#endif // PV_HAVE_TRAPS_FOR_SIGNALS

//----------------------------------------------------------------------------
void vtkPVApplication::SetLogBufferLength(int length)
{
  vtkTimerLog::SetMaxEntries(length);
}

//----------------------------------------------------------------------------
void vtkPVApplication::ResetLog()
{
  vtkTimerLog::ResetLog();
}

//----------------------------------------------------------------------------
void vtkPVApplication::SetEnableLog(int flag)
{
  vtkTimerLog::SetLogging(flag);
}

//----------------------------------------------------------------------------
void vtkPVApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ProcessModule: " << this->ProcessModule << endl;;
  os << indent << "RunningParaViewScript: " 
     << ( this->RunningParaViewScript ? "on" : " off" ) << endl;
  os << indent << "Current Process Id: " << this->ProcessId << endl;
  os << indent << "NumberOfPipes: " << this->NumberOfPipes << endl;
  os << indent << "UseRenderingGroup: " << (this->UseRenderingGroup?"on":"off")
     << endl; 
  os << indent << "UseOffscreenRendering: " << (this->UseOffscreenRendering?"on":"off")
     << endl; 
  os << indent << "StartGUI: " << this->StartGUI << endl;
  if (this->UseTiledDisplay)
    { 
    os << indent << "UseTiledDisplay: On\n";
    os << indent << "TileDimensions: " << this->TileDimensions[0]
       << ", " << this->TileDimensions[1] << endl;
    }

  os << indent << "UseStereoRendering: " << this->UseStereoRendering << endl;
  os << indent << "RenderModuleName: " 
     << (this->RenderModuleName?this->RenderModuleName:"(none)") << endl;

  if (this->ClientMode)
    {
    os << indent << "Running as a client\n";
    os << indent << "Port: " << this->Port << endl;
    os << indent << "Host: " << (this->HostName?this->HostName:"(none)") << endl;
    os << indent << "Username: " 
       << (this->Username?this->Username:"(none)") << endl;
    os << indent << "AlwaysSSH: " << this->AlwaysSSH << endl;
    os << indent << "ReverseConnection: " << this->ReverseConnection << endl;
    }
  if (this->ServerMode)
    {
    os << indent << "Running as a server\n";
    os << indent << "Port: " << this->Port << endl;
    os << indent << "ReverseConnection: " << this->ReverseConnection << endl;
    }
  if (this->UseSoftwareRendering)
    {
    os << indent << "UseSoftwareRendering: Enabled\n";
    }
  if (this->UseSatelliteSoftware)
    {
    os << indent << "UseSatelliteSoftware: Enabled\n";
    }
  if (this->StartEmpty)
    {
    os << indent << "ParaView was started with no default modules.\n";
    }
  os << indent << "Display3DWidgets: " << (this->Display3DWidgets?"on":"off") 
     << endl;
  os << indent << "TraceFileName: " 
     << (this->TraceFileName ? this->TraceFileName : "(none)") << endl;
  os << indent << "Argv0: " 
     << (this->Argv0 ? this->Argv0 : "(none)") << endl;

  os << indent << "RenderModuleName: " 
    << (this->RenderModuleName?this->RenderModuleName:"(null)") << endl;
  os << indent << "ShowSourcesLongHelp: " 
     << (this->ShowSourcesLongHelp?"on":"off") << endl;
  os << indent << "SourcesBrowserAlwaysShowName: " 
     << (this->SourcesBrowserAlwaysShowName?"on":"off") << endl;

  os << indent << "LogThreshold: " << this->LogThreshold << endl;
}

void vtkPVApplication::DisplayTCLError(const char* message)
{
  vtkErrorMacro("TclTk error: "<<message);
}

//----------------------------------------------------------------------------
const char* const vtkPVApplication::LoadComponentProc =
"namespace eval ::paraview {\n"
"    proc load_component {name {optional_paths {}}} {\n"
"        \n"
"        global tcl_platform auto_path env\n"
"        \n"
"        # First dir is empty, to let Tcl try in the current dir\n"
"        \n"
"        set dirs $optional_paths\n"
"        set dirs [concat $dirs {\"\"}]\n"
"        set ext [info sharedlibextension]\n"
"        if {$tcl_platform(platform) == \"unix\"} {\n"
"            set prefix \"lib\"\n"
"            # Help Unix a bit by browsing into $auto_path and /usr/lib...\n"
"            set dirs [concat $dirs /usr/local/lib /usr/local/lib/vtk $auto_path]\n"
"            if {[info exists env(LD_LIBRARY_PATH)]} {\n"
"                set dirs [concat $dirs [split $env(LD_LIBRARY_PATH) \":\"]]\n"
"            }\n"
"            if {[info exists env(PATH)]} {\n"
"                set dirs [concat $dirs [split $env(PATH) \":\"]]\n"
"            }\n"
"        } else {\n"
"            set prefix \"\"\n"
"            if {$tcl_platform(platform) == \"windows\"} {\n"
"                if {[info exists env(PATH)]} {\n"
"                    set dirs [concat $dirs [split $env(PATH) \";\"]]\n"
"                }\n"
"            }\n"
"        }\n"
"        \n"
"        foreach dir $dirs {\n"
"            set libname [file join $dir ${prefix}${name}${ext}]\n"
"            if {[file exists $libname]} {\n"
"                if {![catch {load $libname} errormsg]} {\n"
"                    # WARNING: it HAS to be \"\" so that pkg_mkIndex work (since\n"
"                    # while evaluating a package ::paraview::load_component won't\n"
"                    # exist and will default to the unknown() proc that \n"
"                    # returns \"\"\n"
"                    return \"\"\n"
"                } else {\n"
"                    # If not loaded but file was found, oops\n"
"                    error $errormsg\n"
"                }\n"
"            }\n"
"        }\n"
"        \n"
"        error \"::paraview::load_component: $name could not be found.\"\n"
"        \n"
"        return 1\n"
"    }\n"
"    namespace export load_component\n"
"}\n";



//----------------------------------------------------------------------------
const char* const vtkPVApplication::ExitProc =
"proc exit {} { global Application; $Application Exit }";



//============================================================================
// Stuff that is a part of render-process module.


//----------------------------------------------------------------------------
void vtkPVApplication::RemoteSimpleScript(int remoteId, const char *str)
{
  if (this->ProcessModule)
    {
    this->ProcessModule->RemoteSimpleScript(remoteId, str);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplication::BroadcastSimpleScript(const char *str)
{
  if (this->ProcessModule)
    {
    this->ProcessModule->BroadcastSimpleScript(str);
    }
  else
    {
    this->SimpleScript(str);
    }
}


//-----------------------------------------------------------------------------
char* vtkPVApplication::GetDemoPath()
{
  int found=0;
  char temp1[1024];
  struct stat fs;

  this->SetDemoPath(NULL);

#ifdef _WIN32  

  if (this->GetApplicationInstallationDirectory())
    {
    sprintf(temp1, "%s/Demos/Demo1.pvs",
            this->GetApplicationInstallationDirectory());
    if (stat(temp1, &fs) == 0) 
      {
      sprintf(temp1, "%s/Demos",
              this->GetApplicationInstallationDirectory());
      this->SetDemoPath(temp1);
      found=1;
      }
    }

#else

  vtkKWDirectoryUtilities* util = vtkKWDirectoryUtilities::New();
  const char* selfPath = util->FindSelfPath(
    this->GetArgv0());
  const char* relPath = "../share/paraview-" PARAVIEW_VERSION "/Demos";
  char* newPath = new char[strlen(selfPath)+strlen(relPath)+2];
  sprintf(newPath, "%s/%s", selfPath, relPath);

  char* demoFile = new char[strlen(newPath)+strlen("/Demo1.pvs")+1];
  sprintf(demoFile, "%s/Demo1.pvs", newPath);

  if (stat(demoFile, &fs) == 0) 
    {
    this->SetDemoPath(newPath);
    found = 1;
    }

  delete[] demoFile;
  delete[] newPath;
  util->Delete();

#endif // _WIN32  

  if (!found)
    {
    // Look in binary and installation directories
    const char** dir;
    for(dir=VTK_PV_DEMO_PATHS; !found && *dir; ++dir)
      {
      sprintf(temp1, "%s/Demo1.pvs", *dir);
      if (stat(temp1, &fs) == 0) 
        {
        this->SetDemoPath(*dir);
        found = 1;
        }
      }
    }

  return this->DemoPath;
}

void vtkPVApplication::EnableTestErrors()
{
  this->OutputWindow->EnableTestErrors();
}

void vtkPVApplication::DisableTestErrors()
{
  this->OutputWindow->DisableTestErrors();
}

