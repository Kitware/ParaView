/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonInterpretor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPythonInterpretor - Encapsulates a single instance of a python
// interpretor.
// .SECTION Description
// Encapsulates a python interpretor. It also initializes the interpretor
// with the paths to the paraview libraries and modules. This object can 
// represent the main interpretor or any sub-interpretor.
// .SECTION Caveat
// Since this class uses a static variable to keep track of interpretor lock
// state, it is not safe to use vtkPVPythonInterpretor instances in different
// threads. 

#ifndef __vtkPVPythonInterpretor_h
#define __vtkPVPythonInterpretor_h

#include "vtkObject.h"

class vtkStdString;
class vtkPVPythonInterpretorInternal;
class VTK_EXPORT vtkPVPythonInterpretor : public vtkObject
{
public:
  static vtkPVPythonInterpretor* New();
  vtkTypeMacro(vtkPVPythonInterpretor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initializes python and starts the interprestor's event loop
  // i.e. call // Py_Main().
  int PyMain(int argc, char** argv);
  
  // Description:
  // Initializes python and create a sub-interpretor context.
  // Subinterpretors don't get argc/argv, however, argv[0] is
  // used to set the executable path if not already set. The
  // executable path is used to locate paraview python modules.
  // This method calls ReleaseControl() at the end of the call, hence the newly
  // created interpretor is not the active interpretor when the control returns.
  // Use MakeCurrent() to make it active.
  int InitializeSubInterpretor(int argc, char** argv);

  // Description:
  // This method will make the sub-interpretor represented by this object
  // the active one. A MakeCurrent() call must have a corresponding
  // ReleaseControl() to avoid deadlocks.
  void MakeCurrent();

  // Description:
  // Helper function that executes a script using PyRun_SimpleString() - handles
  // some pesky details with DOS line endings.
  // This method calls MakeCurrent() at the start and ReleaseControl() at the
  // end, hence the interpretor will not be the active one when the control
  // returns from this call. 
  void RunSimpleString(const char* const script);

  // Description:
  // Helper function that calls execfile().
  void RunSimpleFile(const char* const filename);

  // Description:
  // Call in a subinterpretter to pause it and return control to the 
  // main interpretor. 
  // A MakeCurrent() call must have a corresponding
  // ReleaseControl() to avoid deadlocks.
  void ReleaseControl();

  // Description:
  // In some cases, the application may want to capture the output/error streams
  // dumped by the python interpretor. When enabled, the streams are captured
  // and output/error is collected which can be flushed by FlushMessages.
  // vtkCommand::ErrorEvent is fired when data is received on stderr and
  // vtkCommand::WarningEvent is fired when data is received on stdout.
  // from the python interpretor. Event data for both the events is the text
  // received.  This flag can be changed only before the interpretor is 
  // initialized. Changing it afterwards has no effect.
  vtkSetMacro(CaptureStreams, bool);
  vtkGetMacro(CaptureStreams, bool);

  // Description:
  // Flush any errors received from the python interpretor to the
  // vtkOutputWindow. Applicable only if CaptureStreams was true when the
  // interpretor was initialized.
  void FlushMessages();

  // Description:
  // Clears all received messages. Unlike FlushMessages, this call does not dump
  // it on the vtkOutputWindow.
  // Applicable only if CaptureStreams was true when the
  // interpretor was initialized.
  void ClearMessages();

  // Description:
  // Add a directory to python path.
  // Can be called only after InitializeSubInterpretor().
  void AddPythonPath(const char*);

  // Description:
  // Call this method to initialize the interpretor for ParaView GUI
  // applications. This is typically called right after
  // InitializeSubInterpretor().
  void ExecuteInitFromGUI();

protected:
  vtkPVPythonInterpretor();
  ~vtkPVPythonInterpretor();

  // Description:
  // Initialize the interpretor.
  virtual void InitializeInternal();

  void AddPythonPathInternal(const char* path);

  char* ExecutablePath;
  vtkSetStringMacro(ExecutablePath);

  bool CaptureStreams;

  friend struct vtkPVPythonInterpretorWrapper;

  void DumpError(const char* string);
  void DumpOutput(const char* string);
  vtkStdString GetInputLine();

  // Need to be called before releasing the interpretor, otherwise
  // active session VTK callback will be trigger while the interpretor
  // is shutting down which will make the application crash.
  // This is only required when ActiveSessionObserverAttached == true
  // meaning when python has been initialized by the UI
  // meaning that method has been called: paraview.servermanager.InitFromGUI()
  void DetachActiveSessionObserver();

  // Description:
  // Declare that DetachActiveSessionObserver() should be called at close time.
  // CAUTION: This must be set to true when paraview.servermanager.InitFromGUI()
  //          get called.
  bool ActiveSessionObserverAttached;
  
private:
  vtkPVPythonInterpretor(const vtkPVPythonInterpretor&); // Not implemented.
  void operator=(const vtkPVPythonInterpretor&); // Not implemented.

  vtkPVPythonInterpretorInternal* Internal;
};

#endif

