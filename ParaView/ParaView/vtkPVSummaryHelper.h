/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSummaryHelper.h
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
