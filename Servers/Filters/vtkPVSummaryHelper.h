/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSummaryHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSummaryHelper - Synchronize writing of summary files.
// .SECTION Description
// When a parallel XML file is written, we must make sure every disk
// will have exactly one process write the summary file.  This helper
// class is used by PV writer modules to set the WriteSummaryFile or
// WriteCollectionFile flags correctly on the writer.

#ifndef __vtkPVSummaryHelper_h
#define __vtkPVSummaryHelper_h

#include "vtkObject.h"

class vtkMultiProcessController;
class vtkXMLWriter;

class VTK_EXPORT vtkPVSummaryHelper : public vtkObject
{
public:
  static vtkPVSummaryHelper* New();
  vtkTypeRevisionMacro(vtkPVSummaryHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This class uses MPI communication mechanisms to verify the
  // integrity of all case files in the master file.  The get method
  // interface must use vtkMultiProcessController instead of
  // vtkMPIController because Tcl wrapping requires the class's
  // wrapper to be defined, but it is not defined if MPI is not on.
  // In client-server mode, we may still need to create an instance of
  // this class on the client process even if MPI is not compiled in.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  
  // Description:
  // Get/Set the writer instance to be configured.
  virtual void SetWriter(vtkXMLWriter*);
  vtkGetObjectMacro(Writer, vtkXMLWriter);
  
  // Description:
  // Synchronize writing of summary files across multiple processes
  // according to the sharing of disks.  Only one process will write
  // the summary file to each disk.  Return value on process 0 is 1
  // for success and 0 for failure.  Return value on other processes
  // is not meaningful.  Failure can occur if the directory is not
  // writable on all processes' disks.
  int SynchronizeSummaryFiles();
  
protected:
  vtkPVSummaryHelper();
  ~vtkPVSummaryHelper();
  
  // The controller used to communicate with the instances in other
  // processes.
  vtkMultiProcessController* Controller;
  
  vtkXMLWriter* Writer;
  
  int CheckDummyFile();
  void DeleteDummyFiles();
  void SetWriteSummaryFile(int value);
  
private:
  vtkPVSummaryHelper(const vtkPVSummaryHelper&); // Not implemented
  void operator=(const vtkPVSummaryHelper&); // Not implemented
};

#endif
