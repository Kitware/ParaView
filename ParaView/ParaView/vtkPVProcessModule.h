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
#include "vtkClientServerStream.h"

class vtkPolyData;
class vtkKWLoadSaveDialog;
class vtkMapper;
class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVInformation;
class vtkPVPart;
class vtkPVPartDisplay;
class vtkSource;
class vtkStringList;
class vtkClientServerInterpreter;
class vtkClientServerStream;

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
  void RemoteScript(int remoteId, const char *EventString, ...);

  // Description:
  // Can only be called by process 0.  It executes a script on every
  // process.
  void BroadcastScript(const char *EventString, ...);

  // Description:
  // Can only be called by process 0.  
  // It executes a script on every server process.
  // This is needed because we only connect pipeline on server (not client).
  void ServerScript(const char *EventString, ...);


//ETX
  virtual void RemoteSimpleScript(int remoteId, const char *str);
  virtual void BroadcastSimpleScript(const char *str);
  virtual void ServerSimpleScript(const char *str);
  
  // Description:
  // The controller is needed for filter that communicate internally.
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  //BTX
  // Description:
  // This is going to be a generic method of getting/gathering 
  // information form the server.
  virtual void GatherInformation(vtkPVInformation* info,
                                 vtkClientServerID id);
  //ETX
  virtual void GatherInformationInternal(const char* infoClassName,
                                         vtkObject* object);
  
  // Description:
  // Get the partition piece.  -1 means no assigned piece.
  virtual int GetPartitionId();

  // Description:
  // Get the number of processes participating in sharing the data.
  virtual int GetNumberOfPartitions();
  
  // Description:
  // This executes a script on process 0 of the server.
  // Used mainly for client server operation.
//BTX
  void  RootScript(const char *EventString, ...);
//ETX
  virtual void  RootSimpleScript(const char *str);
  vtkSetStringMacro(RootResult);
  virtual const char* GetRootResult();
  
  // Description:
  // Set the application instance for this class.
  virtual void SetApplication (vtkKWApplication* arg);
  
  // Description:
  // Get a directory listing for the given directory.  Returns 1 for
  // success, and 0 for failure (when the directory does not exist).
  virtual int GetDirectoryListing(const char* dir, vtkStringList* dirs,
                                  vtkStringList* files, int save);
  
  // Description:
  // Get a reference to a vtkDataObject from the server-side root node
  // given the Tcl name of the object.
  virtual int ReceiveRootPolyData(const char* tclName,
                                  vtkPolyData* output);
  
  // Description:
  // Get a file selection dialog instance.
  virtual vtkKWLoadSaveDialog* NewLoadSaveDialog();
//BTX  
  // Description:
  // Return the client server stream
  vtkClientServerStream& GetStream() 
    {
      return *this->ClientServerStream;
    }
  
  // Description: 
  // These methods construct/delete a vtk object in the vtkClientServerStream
  // owned by the process module.  The type of the object is specified by
  // string name, and the object to delete is specified by the object id
  // passed in.  To send the stream to the server call SendStreamToServer or
  // SendStreamToClientAndServer.  For construction, the unique id for the
  // new object is returned.
  vtkClientServerID NewStreamObject(const char*);
  void DeleteStreamObject(vtkClientServerID);
  
  // Description:
  // Return the vtk object associated with the given id for the client.
  // If the id is for an object on the server then 0 is returned.
  virtual vtkObjectBase* GetObjectFromID(vtkClientServerID);
  
  // Description:
  // Return a message containing the result of the last SendMessages call.
  // In client/server mode this causes a round trip to the server.
  virtual const vtkClientServerStream& GetLastServerResult();

  // Description:
  // Return a message containing the result of the last call made on
  // the client.
  virtual const vtkClientServerStream& GetLastClientResult();
//ETX
  // Description:

  // Send the current vtkClientServerStream contents to the client only.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToClient();

  // Send the current vtkClientServerStream contents to the server.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToServer();

  // Description:
  // Send current ClientServerStream data to the server and the client.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToClientAndServer();

  //BTX
  // Description:
  // Get the interpreter used on the local process.
  virtual vtkClientServerInterpreter* GetInterpreter();
  //ETX

  // Description:
  // Load a ClientServer wrapper module dynamically in the current
  // process.  Returns 1 for success and 0 for failure.
  int LoadModule(const char* name);

  vtkClientServerID GetUniqueID();
  vtkClientServerID GetApplicationID();
  vtkClientServerID GetProcessModuleID();
protected:
  vtkPVProcessModule();
  ~vtkPVProcessModule();

  virtual void InitializeInterpreter();
  virtual void FinalizeInterpreter();

  vtkMultiProcessController *Controller;
  vtkPVInformation *TemporaryInformation;

  char *RootResult;

  vtkClientServerInterpreter* Interpreter;
  vtkClientServerStream* ClientServerStream;
  vtkClientServerID UniqueID;
private:
  vtkPVProcessModule(const vtkPVProcessModule&); // Not implemented
  void operator=(const vtkPVProcessModule&); // Not implemented
};

#endif
