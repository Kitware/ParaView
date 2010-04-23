/*=========================================================================

  Program:   ParaView
  Module:    vtkSynchronousMPISelfConnection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSynchronousMPISelfConnection
// .SECTION Description
// vtkSynchronousMPISelfConnection is a specialization of vtkMPISelfConnection
// to be used in cases where each process has an active servermanager. Thus for
// a python-based batch client, each process will execute the python script. For
// such cases, each process acts as the root node (as far as the server manager
// is concerned) for SendStream and GatherInformation requests. There's rarely
// any need for parallel communication (except when collecting data information)
// since each process is expected to be executing identical python code.

#ifndef __vtkSynchronousMPISelfConnection_h
#define __vtkSynchronousMPISelfConnection_h

#include "vtkMPISelfConnection.h"

class VTK_EXPORT vtkSynchronousMPISelfConnection : public vtkMPISelfConnection
{
public:
  static vtkSynchronousMPISelfConnection* New();
  vtkTypeMacro(vtkSynchronousMPISelfConnection, vtkMPISelfConnection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Finalizes the connection. Triggers closing of RMI loops on 
  // satellites.
  virtual void Finalize();

//BTX
  // Description:
  // Gather the information about the object from the server.
  virtual void GatherInformation(vtkTypeUInt32 serverFlags, vtkPVInformation* info, 
    vtkClientServerID id);
//ETX

  // Description:
  // Initializes the connection. This sets the progress handler's connection
  // to 0 to turn of progress reporting and calls superclass.
  virtual int Initialize(int argc, char** argv, int *partitionId);

//BTX
protected:
  vtkSynchronousMPISelfConnection();
  ~vtkSynchronousMPISelfConnection();

  // Description:
  // This method gets called on satellite nodes during Initialize().
  virtual int InitializeSatellite(int argc, char** argv);

  // Description
  // send a stream to a node of the mpi group.
  // If remoteId==-1, then the stream is sent to all processess.
  virtual void SendStreamToServerNodeInternal(int remoteId, 
    vtkClientServerStream& stream);

private:
  vtkSynchronousMPISelfConnection(const vtkSynchronousMPISelfConnection&); // Not implemented
  void operator=(const vtkSynchronousMPISelfConnection&); // Not implemented
//ETX
};

#endif

