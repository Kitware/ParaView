/*=========================================================================

  Program:   ParaView
  Module:    vtkMPIMToNSocketConnection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMPIMToNSocketConnection - class to create socket connections between two servers 
//
// .SECTION Description vtkMPIMToNSocketConnection is a
// class used to create socket connections between the render and data
// servers.  This used for example when data is on a super computer (SGI,
// IBM, etc) and Rendering on a Linux cluster with hardware graphics support.
// This problem is known as the "M" to "N" geometry load redistribution
// problem.  It addresses the common case where there is a significante
// mismatch in the size of large parallel computing resources and the often
// smaller parallel hardward-accelerated rendering resources. The larger
// number of processors on the compute servers are called M, and the smaller
// number of rendering processors are call N.  This class is used to create N
// vtkSocketCommunicator's that connect the first N of the M processes on the
// data server to the N processes on the render server.

#ifndef __vtkMPIMToNSocketConnection_h
#define __vtkMPIMToNSocketConnection_h

#include "vtkObject.h"
class vtkMultiProcessController;
class vtkServerSocket;
class vtkSocketCommunicator;
class vtkMPIMToNSocketConnectionPortInformation;
class vtkMPIMToNSocketConnectionInternals;

class VTK_EXPORT vtkMPIMToNSocketConnection : public vtkObject
{
public:
  static vtkMPIMToNSocketConnection* New();
  vtkTypeMacro(vtkMPIMToNSocketConnection,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  void Initialize(int waiting_process_type);

  // Description:
  // Setup  the connection.
  void ConnectMtoN();

  // Description:
  // Set the number of connections to be made.
  void SetNumberOfConnections(int);
  vtkGetMacro(NumberOfConnections, int);

  // Description:
  // Set up information about the remote connection.
  void SetPortInformation(unsigned int processNumber, int portNumber, const char* hostName);
  
  // Description:
  // Return the socket communicator for this process.
  vtkGetObjectMacro(SocketCommunicator, vtkSocketCommunicator);
  
  // Description:
  // Fill the port information values into the port information object.
  void GetPortInformation(vtkMPIMToNSocketConnectionPortInformation*);
  
  // Description:
  // Set port to use, if the value is 0, then the system will pick the port.
  vtkGetMacro(PortNumber,int);

protected:
  vtkSetMacro(PortNumber,int);

  // Description:
  // Add a machine name.
  void SetMachineName(unsigned int idx, const char* name);

  // Description:
  // Setup the wait for connection, but do not wait yet.
  // This should determine the network to be used and the port to be used.
  void SetupWaitForConnection();

  // Description:
  // SetupStartWaitForConnection must be called first.  This
  // method will start waiting for a connection to be made to it.
  void WaitForConnection();

  // Description:
  // Connect to remote server.
  void Connect();


  virtual void SetController(vtkMultiProcessController*);
  virtual void SetSocketCommunicator(vtkSocketCommunicator*);
  vtkMPIMToNSocketConnection();
  ~vtkMPIMToNSocketConnection();
private:
  int PortNumber;
  int Socket;
  vtkServerSocket* ServerSocket;
  char* HostName;
  vtkSetStringMacro(HostName);
  int NumberOfConnections;
  vtkMPIMToNSocketConnectionInternals* Internals;
  vtkMultiProcessController *Controller;
  vtkSocketCommunicator* SocketCommunicator;
  vtkMPIMToNSocketConnection(const vtkMPIMToNSocketConnection&); // Not implemented
  void operator=(const vtkMPIMToNSocketConnection&); // Not implemented
  bool IsWaiting;
};

#endif
