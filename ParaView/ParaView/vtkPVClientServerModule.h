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
class vtkPVData;
class vtkPVApplication;
class vtkMultiProcessController;
class vtkSocketController;
class vtkMapper;
class vtkDataSet;


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
  // Temporary fix because empty VTK objects do not have arrays.
  // This will create arrays if they exist on other processes.
  virtual void CompleteArrays(vtkMapper *mapper, char *mapperTclName);
  void SendCompleteArrays(vtkMapper *mapper);
  virtual void CompleteArrays(vtkDataSet *data, char *dataTclName);
  void SendCompleteArrays(vtkDataSet *data);

  // Description:
  // Get the Partition piece. -1 means no partition assigned to this process.
  virtual int GetPartitionId();

  // Description:
  // Get the number of processes participating in sharing the data.
  virtual int GetNumberOfPartitions();
  
  // Description:
  // Get the bounds of the distributed data.
  virtual void GetPVDataBounds(vtkPVData *pvd, float bounds[6]);

  // Description:
  // Get the total number of points across all processes.
  virtual int GetPVDataNumberOfPoints(vtkPVData *pvd);

  // Description:
  // Get the total number of cells across all processes.
  virtual int GetPVDataNumberOfCells(vtkPVData *pvd);

  // Description:
  // Get the range across all processes of a component of an array.
  // If the component is -1, then the magnitude range is returned.
  virtual void GetPVDataArrayComponentRange(vtkPVData *pvd, int pointDataFlag,
                                    const char *arrayName, int component, 
                                    float *range);


  // Description:
  // When ParaView needs to query data on other procs, it needs a way to
  // get the information back (only VTK object on satellite procs).
  // These methods send the requested data to proc 0 with a tag of 1966.
  // Note:  Process 0 returns without sending.
  // These should probably be consolidated into one GetDataInfo method.
  void SendDataBounds(vtkDataSet *data);
  void SendDataNumberOfCells(vtkDataSet *data);
  void SendDataNumberOfPoints(vtkDataSet *data);
  void SendDataArrayRange(vtkDataSet *data, int pointDataFlag,
                          char *arrayName, int component);

  // Description:
  // This initializes the data object to request the correct partiaion.
  virtual void InitializePVDataPartition(vtkPVData *pvd);
  void InitializePartition(char *tclName, int updateFlag);

  // Description:
  // Flag that differentiates between clinet and server programs.
  vtkGetMacro(ClientMode, int);

  // Description:
  // This is a socket controller used to communicate
  // between the client and process 0 of the server.
  vtkGetObjectMacro(SocketController, vtkSocketController);

protected:
  vtkPVClientServerModule();
  ~vtkPVClientServerModule();

  int NumberOfServerProcesses;
  int ClientMode;
  vtkSocketController* SocketController;

  // To pass arguments through controller single method.
  int    ArgumentCount;
  char** Arguments;
  int    ReturnValue;

private:  
  vtkPVClientServerModule(const vtkPVClientServerModule&); // Not implemented
  void operator=(const vtkPVClientServerModule&); // Not implemented
};

#endif


