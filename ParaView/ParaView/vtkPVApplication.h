/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplication.h
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
// .NAME vtkPVApplication
// .SECTION Description
// A subclass of vtkKWApplication specific to this application.

#ifndef __vtkPVApplication_h
#define __vtkPVApplication_h

#include "vtkKWApplication.h"

class vtkPVProcessModule;
class vtkPVRenderModule;

class vtkDataSet;
class vtkKWMessageDialog;
class vtkMapper;
class vtkMultiProcessController;
class vtkSocketController;
class vtkPVOutputWindow;
class vtkPVSource;
class vtkPVWindow;
class vtkPVRenderView;
class vtkPolyDataMapper;
class vtkProbeFilter;

#define VTK_PV_SLAVE_SCRIPT_RMI_TAG 1150
#define VTK_PV_SLAVE_SCRIPT_COMMAND_LENGTH_TAG 1100
#define VTK_PV_SLAVE_SCRIPT_COMMAND_TAG 1120
#define VTK_PV_SLAVE_SCRIPT_RESULT_LENGTH_TAG 1130
#define VTK_PV_SLAVE_SCRIPT_RESULT_TAG 1140

class VTK_EXPORT vtkPVApplication : public vtkKWApplication
{
public:
  static vtkPVApplication* New();
  vtkTypeRevisionMacro(vtkPVApplication,vtkKWApplication);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Parses the command line arguments and modifies the applications
  // ivars appropriately.
  // Return error (1) if the arguments are not formed properly.
  // Returns 0 if all went well.
  int ParseCommandLineArguments(int argc, char*argv[]);

  // Description:
  // Process module contains all methods for managing 
  // processes and communication.
  void SetProcessModule(vtkPVProcessModule *module);
  vtkPVProcessModule* GetProcessModule() { return this->ProcessModule;}
  
  // Description:
  // RenderingModule has the rendering abstraction.  
  // It creates the render window and any composit manager.  
  // It also creates part displays which handle level of details.
  void SetRenderModule(vtkPVRenderModule *module);
  vtkPVRenderModule* GetRenderModule() { return this->RenderModule;}

  // Description:
  // Start running the main application.
  virtual void Start(int argc, char *argv[]);
  virtual void Start()
    { this->vtkKWApplication::Start(); }
  virtual void Start(char* arg)
    { this->vtkKWApplication::Start(arg); }

  
//BTX
  // Description:
  // Script which is executed in the remot processes.
  // If a result string is passed in, the results are place in it. 
  void RemoteScript(int remoteId, char *EventString, ...);

  // Description:
  // Can only be called by process 0.  It executes a script on every other
  // process.
  void BroadcastScript(char *EventString, ...);
//ETX
  void RemoteSimpleScript(int remoteId, const char *str);
  void BroadcastSimpleScript(const char *str);
  
  // Description:
  // We need to keep the controller in a prominent spot because there is no
  // more "RegisterAndGetGlobalController" method.
  vtkMultiProcessController *GetController();
  
  // Description:
  // If ParaView is running in client server mode, then this returns
  // the socket controller used for client server communication.
  // It will only be set on the client and process 0 of the server.
  vtkSocketController *GetSocketController();

  // Description:
  // No licence required.
  int AcceptLicense();
  int AcceptEvaluation();

  // Description:
  // This method is invoked when a window closes
  virtual void Close(vtkKWWindow *);

  // Description:
  // We need to kill the slave processes
  virtual void Exit();
  
  // Description:
  // Initialize Tcl/Tk
  // Return NULL on error (eventually provides an ostream where detailed
  // error messages will be stored).
  //BTX
  static Tcl_Interp *InitializeTcl(int argc, char *argv[], ostream *err = 0);
  //ETX

//BTX
#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
  // Description:
  // Setup traps for signals that may kill ParaView.
  void SetupTrapsForSignals(int nodeid);
  static void TrapsForSignals(int signal);

  // Description:
  // When error happens, try to exit as gracefully as you can.
  static void ErrorExit();
#endif // PV_HAVE_TRAPS_FOR_SIGNALS
//ETX

  // Description:
  // This constructs a vtk object (type specified by class name) and
  // uses the tclName for the tcl instance command.  The user must
  // cast to the correct type, and is responsible for deleting the
  // object.
  vtkObject *MakeTclObject(const char *className,
                           const char *tclName);

  // Description:
  // This constructs a vtk object (type specified by class name) on the
  // server processes and uses the tclName for the tcl instance command.
  // The user must cast to the correct type, and is responsible for deleting
  // the object.
  void MakeServerTclObject(const char *className, const char *tclName);

  // Description:
  // This method returns pointer to the object specified as a tcl
  // name.
  vtkObject *TclToVTKObject(const char *tclName);
    
  // Description:
  // A start at recording macros in ParaView.  Create a custom trace file
  // that can be loaded back into paraview.  Window variables get
  // initialized when the file is opened.
  // Note: The trace entries get diverted to this file.  This is only used
  // for testing at the moment.  It is restricted to using sources created
  // after the recording is started.  The macro also cannot use the glyph
  // sources.  To make mocro recording available to the user, then there
  // must be a way of setting arguments (existing sources) to the macro,
  // and a way of prompting the user to set the arguments when the
  // macro/script is loaded.
  void StartRecordingScript(char *filename);
  void StopRecordingScript();

  // Description:
  // Since ParaView has only one window, we might as well provide access to it.
  vtkPVWindow *GetMainWindow();
  vtkPVRenderView *GetMainView();

  // Description:
  // Display the on-line help and about dialog for this application.
  // Over-writing vtkKWApplication defaults.
  void DisplayHelp(vtkKWWindow* master);

  // For locating help (.chm) on Windows.
  virtual int GetApplicationKey() 
    {
      return 15;
    };

  // Description:
  // Need to put a global flag that indicates interactive rendering.  All
  // process must be consistent in choosing LODs because of the
  // vtkCollectPolydata filter.  This has to be in vtkPVApplication
  // because we do not create a render module on remote processes.
  void SetGlobalLODFlag(int val);
  static int GetGlobalLODFlag();
  void SetGlobalLODFlagInternal(int val);

  // Description:
  // For loggin from Tcl start and end execute events.  We do not have c
  // pointers to all filters.
  void LogStartEvent(char* str);
  void LogEndEvent(char* str);

  // Description:
  // More timer log access methods.  Static methods are not accessible 
  // from tcl.  We need a timer object on all procs.
  void SetLogBufferLength(int length);
  void ResetLog();
  void SetEnableLog(int flag);

  // Description:
  // Time threshold for event (start-end) when getting the log with indents.
  // We do not have a timer object on all procs.  Statics do not work with Tcl.
  vtkSetMacro(LogThreshold, float);
  vtkGetMacro(LogThreshold, float);

  // Description:
  // Flag showing whether the commands are being executed from
  // a ParaView script.
  vtkSetMacro(RunningParaViewScript, int);
  vtkGetMacro(RunningParaViewScript, int);

  // Description:
  // Tells the process modules whether to start the main
  // event loop. Mainly used by command line argument parsing code
  // when an argument requires not starting the GUI
  vtkSetMacro(StartGUI, int);
  vtkGetMacro(StartGUI, int);

  //BTX
  static const char* const LoadComponentProc;
  static const char* const ExitProc;
  //ETX

  void DisplayTCLError(const char* message);

  // Description:
  // A method used to set environment variables in the satellite
  // processes. This method leaks memory and for now should be called
  // only once.
  void SetEnvironmentVariable(const char* string);

  // Description: 
  // Set or get the display 3D widgets flag.  When this flag is set,
  // the 3D widgets will be displayed when they are created. Otherwise
  // user has to enable them. User will still be able to disable the
  // 3D widget.
  vtkSetClampMacro(Display3DWidgets, int, 0, 1);
  vtkBooleanMacro(Display3DWidgets, int);
  vtkGetMacro(Display3DWidgets, int);

  // Description: 
  // Are we using a subset to render?
  vtkGetMacro(UseRenderingGroup, int);

  // Description:
  // Varibles (command line argurments) set to render to a tiled display.
  vtkGetMacro(UseTiledDisplay, int);
  vtkGetVector2Macro(TileDimensions, int);

  // Description:
  // Variable set by command line arguments --client or -c
  // Client mode tries to connect to a server through a socket.
  // The client does not have any local partitioned data.
  vtkGetMacro(ClientMode,int);

  // Description:
  // Variable set by command line arguments --server or -v
  // Server can have many processes, but has no UI.
  vtkGetMacro(ServerMode,int);

  // Description:
  // Get the host command line option. (--host=localhost).
  vtkGetStringMacro(HostName);
  vtkGetStringMacro(Username);

  // Description:
  // The the port for the client/server socket connection.
  vtkGetMacro(Port,int);

  // Description:
  // The default behavior is for the server to wait and for the client 
  // to connect to the server.  When this flag is set by the command line 
  // arguments.  The server tries to connect to the client instead.
  vtkGetMacro(ReverseConnection,int);

  // Description:
  // Variable set by command line arguments --client or -c
  // Client mode tries to connect to a server through a socket.
  // The client does not have any local partitioned data.
  vtkGetMacro(UseStereoRendering,int);

  // Description:
  // Set by the command line arguments --use-software-rendering or -r
  // Requires ParaView is linked with mangled mesa.
  // Supports off screen rendering.
  vtkGetMacro(UseSoftwareRendering,int);

  // Description:
  // Set by the command line arguments --use-satellite-software or -s
  // Requires ParaView is linked with mangled mesa.
  // Satellite processes use mesa (supports offscreen) while root
  // can still use hardware acceleration.
  vtkGetMacro(UseSatelliteSoftware,int);

  // Description:
  // Set by the command line arguments --use-offscreen-rendering or -os
  // Requires that ParaView is linked with mangled mesa or that sofware
  // rendering is enabled on Unix.
  // Satellite processes render offscreen.
  vtkGetMacro(UseOffscreenRendering,int);

  // Description:
  // Set by the command line arguments --start-empty or -e
  // This flag is set when ParaView was started without the default modules.
  vtkGetMacro(StartEmpty,int);

  // Description:
  // This is used internally for specifying how many pipes
  // to use for rendering when UseRenderingGroup is defined.
  // All processes have this set to the same value.
  vtkSetMacro(NumberOfPipes, int);
  vtkGetMacro(NumberOfPipes, int);

  // Description:
  // This root class name will eventually be replaced
  // with an XML specification of rendering module classes.
  vtkGetStringMacro(RenderModuleName);

  // Description:
  // I have ParaView.cxx set the proper default render module.
  vtkSetStringMacro(RenderModuleName);  

  // Description:
  // This is used (Unix only) to obtain the path of the executable.
  // This path is used to locate demos etc.
  vtkGetStringMacro(Argv0);

  // Description:
  // The name of the trace file.
  vtkGetStringMacro(TraceFileName);

  vtkSetClampMacro(AlwaysSSH, int, 0, 1);
  vtkBooleanMacro(AlwaysSSH, int);
  vtkGetMacro(AlwaysSSH, int);

  // Descrition:
  // Show/Hide the sources long help.
  virtual void SetShowSourcesLongHelp(int);
  vtkGetMacro(ShowSourcesLongHelp, int);
  vtkBooleanMacro(ShowSourcesLongHelp, int);

  // Descrition:
  // Show/Hide the sources long help.
  virtual void SetSourcesBrowserAlwaysShowName(int);
  vtkGetMacro(SourcesBrowserAlwaysShowName, int);
  vtkBooleanMacro(SourcesBrowserAlwaysShowName, int);

  // Descrition:
  // Get those application settings that are stored in the registery
  // Should be called once the application name is known (and the registery
  // level set).
  virtual void GetApplicationSettingsFromRegistery();

  // Description:
  // We need to get the data path for the demo on the server.
  char* GetDemoPath();

  // Description:
  // Enable or disable test errors. This refers to wether errors make test fail
  // or not.
  void EnableTestErrors();
  void DisableTestErrors();

  // Description:
  // This is a debug feature of ParaView. If this is set, ParaView will crash on errors.
  vtkSetClampMacro(CrashOnErrors, int, 0, 1);
  vtkBooleanMacro(CrashOnErrors, int);
  vtkGetMacro(CrashOnErrors, int);

protected:
  vtkPVApplication();
  ~vtkPVApplication();

  virtual void CreateSplashScreen();
  virtual void AddAboutText(ostream &);

  void CreateButtonPhotos();
  void CreatePhoto(char *name, 
                   unsigned char *data, 
                   int width, int height, int pixel_size, 
                   unsigned long buffer_length = 0,
                   char *filename = 0);
  int CheckRegistration();
  int PromptRegistration(char *,char *);

  vtkPVProcessModule *ProcessModule;
  vtkPVRenderModule *RenderModule;
  char* RenderModuleName;

  // For running with SGI pipes.
  int NumberOfPipes;

  int ProcessId;

  int Display3DWidgets;

  int StartGUI;

  int RunBatchScript;

  char* BatchScriptName;
  vtkSetStringMacro(BatchScriptName);
  vtkGetStringMacro(BatchScriptName);

  // Command line arguments.
  int ClientMode;
  int ServerMode;
  char* HostName;
  vtkSetStringMacro(HostName);
  char* Username;
  vtkSetStringMacro(Username);
  int Port;
  int AlwaysSSH;
  int ReverseConnection;
  int UseSoftwareRendering;
  int UseSatelliteSoftware;
  int UseStereoRendering;
  int StartEmpty;
  int PlayDemo;
  int UseRenderingGroup;
  int UseOffscreenRendering;
  char* GroupFileName;
  vtkSetStringMacro(GroupFileName);
  int UseTiledDisplay;
  int TileDimensions[2];

  // Need to put a global flag that indicates interactive rendering.
  // All process must be consistent in choosing LODs because
  // of the vtkCollectPolydata filter.
  static int GlobalLODFlag;

  int RunningParaViewScript;
  
  vtkPVOutputWindow *OutputWindow;

  static int CheckForExtension(const char* arg, const char* ext);
  char* CreateHelpString();
  int CheckForTraceFile(char* name, unsigned int len);
  void DeleteTraceFiles(char* name, int all);
  void SaveTraceFile(const char* fname);

  vtkSetStringMacro(TraceFileName);
  char* TraceFileName;
  char* Argv0;
  vtkSetStringMacro(Argv0);

  //BTX
  enum
  {
    NUM_ARGS=100
  };
  static const char ArgumentList[vtkPVApplication::NUM_ARGS][128];
  //ETX

  static vtkPVApplication* MainApplication;  

  int ShowSourcesLongHelp;
  int SourcesBrowserAlwaysShowName;

  char* DemoPath;
  vtkSetStringMacro(DemoPath);

  float LogThreshold;

  int CrashOnErrors;

private:  
  vtkPVApplication(const vtkPVApplication&); // Not implemented
  void operator=(const vtkPVApplication&); // Not implemented
};

#endif


