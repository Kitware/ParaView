/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModule.h
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
// .NAME vtkPVProcessModule
// .SECTION Description
// A class to encapaulate all of the process initialization,
// This super class assumes the application is running all in one process
// with no MPI.

#ifndef __vtkPVProcessModule_h
#define __vtkPVProcessModule_h

#include "vtkKWObject.h"

class vtkDataObject;
class vtkKWLoadSaveDialog;
class vtkMapper;
class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVDataInformation;
class vtkPVPart;
class vtkPVPartDisplay;
class vtkSource;
class vtkStringList;

class VTK_EXPORT vtkPVProcessModule : public vtkKWObject
{
public:
  static vtkPVProcessModule* New();
  vtkTypeRevisionMacro(vtkPVProcessModule,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // ParaView.cxx (main) calls this method to setup the processes.
  // It currently creates the application, but I will try to pass
  // the application as an argument.
  virtual int Start(int argc, char **argv);
  
  // Description:
  // This breaks rmi loops and cleans up processes.`                
  virtual void Exit();

  // Description:
  // Access to the subclass PVApplication store in the superclass
  // as a generic vtkKWApplication.
  vtkPVApplication *GetPVApplication();

//BTX
  // Description:
  // Script which is executed in the remot processes.
  // If a result string is passed in, the results are place in it. 
  void RemoteScript(int remoteId, char *EventString, ...);

  // Description:
  // Can only be called by process 0.  It executes a script on every
  // process.
  void BroadcastScript(char *EventString, ...);

  // Description:
  // Can only be called by process 0.  
  // It executes a script on every server process.
  // This is needed because we only connect pipeline on server (not client).
  void ServerScript(char *EventString, ...);


//ETX
  virtual void RemoteSimpleScript(int remoteId, const char *str);
  virtual void BroadcastSimpleScript(const char *str);
  virtual void ServerSimpleScript(const char *str);
  
  // Description:
  // The controller is needed for filter that communicate internally.
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // This has migrated into get PVPartInformation.
  // If there were PVParts on all processes, the API would be simpler.
  // User calls the first method passing info object.
  // Second method gets broadcasted to all procs.
  // I dislike this, but the info get temporarily stored as an ivar.
  // If vtkPVPart existed on all processes, 
  // it would make this method cleaner.
  void GatherDataInformation(vtkPVDataInformation *info, 
                             char* deciTclName); 
  virtual void GatherDataInformation(vtkSource* deci);
  
  // Description:
  // Get the partition piece.  -1 means no assigned piece.
  virtual int GetPartitionId();

  // Description:
  // Get the number of processes participating in sharing the data.
  virtual int GetNumberOfPartitions();
  
  // Description:
  // This initializes the data object to request the correct partiaion.
  virtual void InitializePartition(vtkPVPartDisplay* partDisplay);

  // Description:
  // This executes a script on process 0 of the server.
  // Used mainly for client server operation.
//BTX
  void  RootScript(char *EventString, ...);
//ETX
  virtual void  RootSimpleScript(const char *str);
  virtual const char* GetRootResult();
  
  // Description:
  // Set the application instance for this class.
  virtual void SetApplication (vtkKWApplication* arg);
  
  // Description:
  // Get a directory listing for the given directory.  Returns 1 for
  // success, and 0 for failure (when the directory does not exist).
  int GetDirectoryListing(const char* dir, vtkStringList* dirs,
                          vtkStringList* files);
  virtual int GetDirectoryListing(const char* dir, vtkStringList* dirs,
                                  vtkStringList* files, const char* perm);
  
  // Description:
  // Get a reference to a vtkDataObject from the server-side root node
  // given the Tcl name of the object.
  virtual vtkDataObject* ReceiveRootDataObject(const char* tclName);
  
  // Description:
  // Get a file selection dialog instance.
  virtual vtkKWLoadSaveDialog* NewLoadSaveDialog();
  
protected:
  vtkPVProcessModule();
  ~vtkPVProcessModule();

  void InitializeTclMethodImplementations();
  
  vtkMultiProcessController *Controller;
  vtkPVDataInformation *TemporaryInformation;

private:  
  vtkPVProcessModule(const vtkPVProcessModule&); // Not implemented
  void operator=(const vtkPVProcessModule&); // Not implemented
};

#endif


