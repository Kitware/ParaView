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
#include "vtkMultiProcessController.h"

class vtkPVSource;
class vtkPolyDataMapper;
class vtkProbeFilter;
class vtkMapper;

#define VTK_PV_SLAVE_SCRIPT_RMI_TAG 1150
#define VTK_PV_SLAVE_SCRIPT_COMMAND_LENGTH_TAG 1100
#define VTK_PV_SLAVE_SCRIPT_COMMAND_TAG 1120
#define VTK_PV_SLAVE_SCRIPT_RESULT_LENGTH_TAG 1130
#define VTK_PV_SLAVE_SCRIPT_RESULT_TAG 1140


class VTK_EXPORT vtkPVApplication : public vtkKWApplication
{
public:
  static vtkPVApplication* New();
  vtkTypeMacro(vtkPVApplication,vtkKWApplication);
  
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
  // Can only be called by process 0.  It executes a script on every other process.
  void BroadcastScript(char *EventString, ...);
//ETX
  void RemoteSimpleScript(int remoteId, char *str);
  void RemoteSimpleScript(int remoteId, const char *str);
  void BroadcastSimpleScript(char *str);
  void BroadcastSimpleScript(const char *str);
  
  // Description:
  // We need to keep the controller in a prominent spot because there is no more 
  // "RegisterAndGetGlobalController" method.
  void SetController(vtkMultiProcessController *c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  
  // Description:
  // Make sure the user accepts the license before running.
  int AcceptLicense();
  int AcceptEvaluation();

  // Description:
  // We need to kill the slave processes
  virtual void Exit();
  
  // Description:
  // class static method to initialize Tcl/Tk
  static Tcl_Interp *InitializeTcl(int argc, char *argv[]);  

  // This constructs a vtk object (type specified by class name)
  // and uses the tclName for the tcl instance command.
  // The user must cast to the correct type, and is responsible
  // for deleting the object.
  vtkObject *MakeTclObject(const char *className,
                           const char *tclName);

  // Description:
  // When ParaView needs to query data on other procs, it needs a way to
  // get the information back (only VTK object on satellite procs).
  // These methods send the requested data to proc 0 with a tag of 1966.
  // Note:  Process 0 returns without sending.
  // These should probably be consolidated into one GetDataInfo method.
  void SendDataBounds(vtkDataSet *data);
  void SendDataNumberOfCells(vtkDataSet *data);
  void SendDataNumberOfPoints(vtkDataSet *data);
  void SendMapperColorRange(vtkPolyDataMapper *mapper);
  void SendProbeData(vtkProbeFilter *source);
  void SendDataArrayRange(vtkDataSet *data, char *arrayName);
  
  // Description:
  // A method that should probably be in the mapper.
  void GetMapperColorRange(float range[2], vtkPolyDataMapper *mapper);
  
  // Description:
  // Methods that will create a text log file.
  // I do not know what kind of login I will be doing, (multi threading ...)
  // So I will keep the interface generic.
  void StartLog(char *filename);
  void StopLog();
  void AddLogEntry(char *tag, float value);

  // Description:
  // A start at recording macros in ParaView.  Create a custom trace file
  // that can be loaded back into paraview.  
  // Window variables get initialized when the file is opened.
  // Note: The trace entries get diverted to this file.
  // This is only used for testing at the moment.  It is restricted to using
  // sources created after the recording is started.  The macro also cannot use
  // the glyph sources.  To make mocro recording available to the user,
  // then there must be a way of setting arguments (existing sources) to the 
  // macro, and a way of prompting the user to set the arguments when the
  // macro/script is loaded.
  void StartRecordingScript(char *filename);
  void StopRecordingScript();

  // Description:
  // Temporary fix because empty VTK objects do not have arrays.
  // This will create arrays if they exist on other processes.
  void CompleteArrays(vtkMapper *mapper, char *mapperTclName);
  void SendCompleteArrays(vtkMapper *mapper);


  // Description:
  // This value will be returned by ParaView at exit.
  // Use this from scripts if you want ParaView exit with an
  // error status (for example to indicate that a regression test 
  // failed)
  vtkSetMacro(ExitStatus, int);
  vtkGetMacro(ExitStatus, int);

  // Description:
  // ParaView version is always MajorVersion.MinorVersion.
  // Change these in the constructor.
  vtkGetMacro(MajorVersion, int);
  vtkGetMacro(MinorVersion, int);


  // Description:
  // Display the on-line help and about dialog for this application.
  // Over-writing vtkKWApplication defaults.
  void DisplayHelp();
  virtual void DisplayAbout(vtkKWWindow *);

  // For locating help (.chm) on Windows.
  virtual int GetApplicationKey() 
    {
      return 15;
    };

protected:
  vtkPVApplication();
  ~vtkPVApplication();
  vtkPVApplication(const vtkPVApplication&) {};
  void operator=(const vtkPVApplication&) {};

  
  void *Log;
  char *LogFileName;
  vtkSetStringMacro(LogFileName);

  void CreateButtonPhotos();
  void CreatePhoto(char *name, unsigned char *data, int width, int height);
  int CheckRegistration();
  int PromptRegistration(char *,char *);

  int ExitStatus;
  int MajorVersion;
  int MinorVersion;

  vtkMultiProcessController *Controller;
};

#endif


