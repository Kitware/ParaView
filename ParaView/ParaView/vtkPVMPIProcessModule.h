/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPIProcessModule.h
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
// .NAME vtkPVMPIProcessModule
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

#ifndef __vtkPVMPIProcessModule_h
#define __vtkPVMPIProcessModule_h

#include "vtkPVProcessModule.h"
class vtkPVPart;
class vtkPVApplication;
class vtkMultiProcessController;
class vtkMapper;
class vtkDataSet;

class VTK_EXPORT vtkPVMPIProcessModule : public vtkPVProcessModule
{
public:
  static vtkPVMPIProcessModule* New();
  vtkTypeRevisionMacro(vtkPVMPIProcessModule,vtkPVProcessModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // ParaView.cxx (main) calls this method to setup the processes.
  // It currently creates the application, but I will try to pass
  // the application as an argument.  Start calls Initialize
  // which calls vtkPVApplication::Start();
  virtual int Start(int argc, char **argv);
  void Initialize();
  
  // Description:
  // This breaks rmi loops and cleans up processes.`                
  virtual void Exit();

  // Send the current vtkClientServerStream contents to the client.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToClient();

  // Send the current vtkClientServerStream contents to the server.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToServer();

  // Send the current vtkClientServerStream contents to the server
  // root node.  Also reset the vtkClientServerStream object.
  virtual void SendStreamToServerRoot();

  // Description:
  // Send current ClientServerStream data to the server and the client.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToClientAndServer();

  // Description:
  // Send current ClientServerStream data to the server root and the client.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToClientAndServerRoot();

  // Description:
  // A method for getting generic information from the server.
  virtual void GatherInformationInternal(const char* infoClassName,
                                         vtkObject* object);
    
  // Description:
  // Get the partition number. -1 means no assigned partition.
  virtual int GetPartitionId();

  // Description:
  // Get the number of processes participating in sharing the data.
  virtual int GetNumberOfPartitions();

  // Description:
  // Used internally.  Do not call.  Use LoadModule instead.
  virtual int LoadModuleInternal(const char* name);
protected:
  vtkPVMPIProcessModule();
  ~vtkPVMPIProcessModule();

  virtual void SendStreamToServerNodeInternal(int remoteId);
  virtual void SendStreamToServerInternal();

  // To pass arguments through controller single method.
  int    ArgumentCount;
  char** Arguments;
  int    ReturnValue;

private:  
  vtkPVMPIProcessModule(const vtkPVMPIProcessModule&); // Not implemented
  void operator=(const vtkPVMPIProcessModule&); // Not implemented
};

#endif
