/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerModule.h
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
// .NAME vtkPVClientServerModule
// .SECTION Description
// A class to encapaulate all of the process initialization,
// distributed data model and duplication  of the pipeline.
// Filters and compositers will still need a controller, 
// but every thing else should be handled here.  This class 
// sets up the default MPI processes with the user interface
// running on process 0.  I plan to make an alternative module
// for client server mode, where the client running the UI 
// is not in the MPI group but links to the MPI group through 
// a socket connection.

#ifndef __vtkPVClientServerModule_h
#define __vtkPVClientServerModule_h

#include "vtkPVProcessModule.h"

class vtkKWRemoteExecute;
class vtkMapper;
class vtkMapper;
class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVPart;
class vtkSocketController;
class vtkSource;

class VTK_EXPORT vtkPVClientServerModule : public vtkPVProcessModule
{
public:
  static vtkPVClientServerModule* New();
  vtkTypeRevisionMacro(vtkPVClientServerModule,vtkPVProcessModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This starts the whole application.
  // This method initializes the MPI controller, then passes control
  // onto the init method.
  virtual int Start(int argc, char **argv);

  // Description:  
  // Start calls this method to continue initialization.
  // This method initializes the sockets and then calls
  // vtkPVApplication::Start(argc, argv);
  void Initialize();

  // Description:
  // This breaks rmi loops and cleans up processes.`                
  virtual void Exit();

  // Description:
  // The primary method for building pipelines on remote proceses
  // is to use tcl.
  virtual void RemoteSimpleScript(int remoteId, const char *str);
  virtual void BroadcastSimpleScript(const char *str);
  void BroadcastScriptRMI(const char *str);
  void RelayScriptRMI(const char *str);
    
  // Description:
  // Execute on all server procs (but not client).
  virtual void ServerSimpleScript(const char *str);

  // Description:
  // Get the Partition piece. -1 means no partition assigned to this process.
  virtual int GetPartitionId();

  // Description:
  // Get the number of processes participating in sharing the data.
  virtual int GetNumberOfPartitions();
  
  // Description:
  // Flag that differentiates between clinet and server programs.
  vtkGetMacro(ClientMode, int);

  // Description:
  // This is a socket controller used to communicate
  // between the client and process 0 of the server.
  vtkGetObjectMacro(SocketController, vtkSocketController);

  //BTX
  // Description:
  // Module dependant method for collecting data information from all procs.
  virtual void GatherInformation(vtkPVInformation* info,
                                 vtkClientServerID id);
  //ETX
  virtual void GatherInformationInternal(const char* infoClassName,
                                         vtkObject* object);

  // Description:
  // This executes a script on process 0 of the server.
  // Used mainly for client server operation.
  virtual void  RootSimpleScript(const char *str);
  virtual const char* GetRootResult();
  
  // Description:
  // Get a directory listing for the given directory.  This
  // implementation will always give a listing on the server side.
  virtual int GetDirectoryListing(const char* dir, vtkStringList* dirs,
                                  vtkStringList* files, int save);
  
  // Description:
  // Get a file selection dialog instance.
  virtual vtkKWLoadSaveDialog* NewLoadSaveDialog();
  
  // Description:
  // Get a reference to a vtkDataObject from the server-side root node
  // given the Tcl name of the object.
  virtual int ReceiveRootPolyData(const char* tclName,
                                  vtkPolyData* output);

//BTX
  enum 
    {
    SINGLE_PROCESS_MODE = 0,
    MPI_MODE
    };
//ETX
  
  static void ErrorCallback(vtkObject *caller, unsigned long eid, void *clientdata, void *calldata);

  // Description:
  // Process a client server message on the server.
  void ProcessMessage(unsigned char* arg, size_t len);
  
  // Description:
  // Send current ClientServerStream data to the client
  virtual void SendStreamToClient();
  
  // Description:
  // Send current ClientServerStream data to the server
  virtual void SendStreamToServer();
  
  // Send the current vtkClientServerStream contents to the server
  // root node.  Also reset the vtkClientServerStream object.
  virtual void SendStreamToServerRoot();

  // Description:
  // Send current ClientServerStream data to the server and the client.
  virtual void SendStreamToClientAndServer();

  //BTX
  // Description:
  // Return a message containing the result of the last SendMessages call.
  // In client/server mode this causes a round trip to the server.
  virtual const vtkClientServerStream& GetLastServerResult();

  // Description:
  // Return a message containing the result of the last call made on
  // the client.
  virtual const vtkClientServerStream& GetLastClientResult();
  friend void vtkPVClientServerLastResultRMI(  void *, void* , int ,int );
  //ETX

  // Description:
  // Used internally.  Do not call.  Use LoadModule instead.
  int LoadModuleInternal(const char* name);
protected:
  vtkPVClientServerModule();
  ~vtkPVClientServerModule();

  // Description:
  // Send the last client server result to the client called from an RMI
  void SendLastClientServerResult();

  void SendStreamToServerInternal();
  void Connect();

  int NumberOfServerProcesses;
  int ClientMode;
  vtkSocketController* SocketController;

  // To pass arguments through controller single method.
  int    ArgumentCount;
  char** Arguments;
  int    ReturnValue;

  vtkSetStringMacro(Hostname);
  vtkSetStringMacro(Username);
  char* Hostname;
  char* Username;
  int Port;
  int MultiProcessMode;
  int NumberOfProcesses;
  
  vtkClientServerStream* LastServerResultStream;
  
  vtkKWRemoteExecute* RemoteExecution;
private:  
  vtkPVClientServerModule(const vtkPVClientServerModule&); // Not implemented
  void operator=(const vtkPVClientServerModule&); // Not implemented
};

#endif
