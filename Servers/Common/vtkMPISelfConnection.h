/*=========================================================================

  Program:   ParaView
  Module:    vtkMPISelfConnection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMPISelfConnection
// .SECTION Description
// This is a connection between a group of MPI processes. On a server
// (data/render) this is the default SelfConnection when MPI is enabled.
// When running ParaView with MPI (but not in client server mode), this is 
// the first Server Connection.

#ifndef __vtkMPISelfConnection_h
#define __vtkMPISelfConnection_h

#include "vtkSelfConnection.h"

class vtkProcessModuleGUIHelper;

class VTK_EXPORT vtkMPISelfConnection : public vtkSelfConnection
{
public:
  static vtkMPISelfConnection* New();
  vtkTypeMacro(vtkMPISelfConnection, vtkSelfConnection);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // Initializes the connection. This is essential to
  // intialize the controller associated with the connection etc etc.
  virtual int Initialize(int argc, char** argv, int *partitionId); 

  // Description:
  // Finalizes the connection. Triggers closing of RMI loops on 
  // satellites.
  virtual void Finalize();
  
//BTX
  // Description:
  // Gather the information about the object from the server.
  virtual void GatherInformation(vtkTypeUInt32 serverFlags, vtkPVInformation* info, 
    vtkClientServerID id);

  enum
    {
    // Send stream to satellites. 
    ROOT_SATELLITE_RMI_TAG = 397529,
    // Request satellite to gather information.
    ROOT_SATELLITE_GATHER_INFORMATION_RMI_TAG = 498797, 

    // Replies from satellites with gathered information
    ROOT_SATELLITE_INFO_LENGTH_TAG = 498798,
    ROOT_SATELLITE_INFO_TAG = 498799
    };
//ETX

  // Description:
  // Load a ClientServer wrapper module dynamically in the server
  // processes.  Returns 1 if all server nodes loaded the module and 0
  // otherwise. 
  virtual int LoadModule(const char* name, const char* directory);

  // Description:
  // Public for callback. Do not call.
  void GatherInformationSatellite(vtkClientServerStream&);
protected:
  vtkMPISelfConnection();
  ~vtkMPISelfConnection();

  // Description:
  // Given the servers that need to receive the stream, create a flag
  // that will send it to the correct places for this process module and
  // make sure it only gets sent to each server once. 
  // This decides if to send the stream to root or everyone. 
  // Sending to all processes on a single process node is 
  // same as sending to the root.
  virtual vtkTypeUInt32 CreateSendFlag(vtkTypeUInt32 );

  // send a stream to the data server
  virtual int SendStreamToDataServer(vtkClientServerStream& s);
  // send a stream to the data server root mpi process
  virtual int SendStreamToDataServerRoot(vtkClientServerStream& s);

  // Description:
  // This method gets called only on root node during Initialize().
  virtual int InitializeRoot(int argc, char** argv);

  // Description:
  // This method gets called on satellite nodes during Initialize().
  virtual int InitializeSatellite(int argc, char** argv);
  
  // Description
  // send a stream to a node of the mpi group.
  // If remoteId==-1, then the stream is sent to all processess.
  virtual void SendStreamToServerNodeInternal(int remoteId, 
    vtkClientServerStream& stream);

  // Description:
  // Internal methods to gather information.
  void GatherInformationRoot(vtkPVInformation* info, vtkClientServerID id);

  // Description:
  // Collect information from children and send it to the parent.
  void CollectInformation(vtkPVInformation* info);

  void RegisterSatelliteRMIs();
private:
  vtkMPISelfConnection(const vtkMPISelfConnection&); // Not implemented.
  void operator=(const vtkMPISelfConnection&); // Not implemented.
};

#endif

