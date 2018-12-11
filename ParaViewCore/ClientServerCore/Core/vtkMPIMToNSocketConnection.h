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
/**
 * @class   vtkMPIMToNSocketConnection
 * @brief   class to create socket connections between two servers
 *
 *
 * class used to create socket connections between the render and data
 * servers.  This used for example when data is on a super computer (SGI,
 * IBM, etc) and Rendering on a Linux cluster with hardware graphics support.
 * This problem is known as the "M" to "N" geometry load redistribution
 * problem.  It addresses the common case where there is a significante
 * mismatch in the size of large parallel computing resources and the often
 * smaller parallel hardward-accelerated rendering resources. The larger
 * number of processors on the compute servers are called M, and the smaller
 * number of rendering processors are call N.  This class is used to create N
 * vtkSocketCommunicator's that connect the first N of the M processes on the
 * data server to the N processes on the render server.
*/

#ifndef vtkMPIMToNSocketConnection_h
#define vtkMPIMToNSocketConnection_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
class vtkMultiProcessController;
class vtkServerSocket;
class vtkSocketCommunicator;
class vtkMPIMToNSocketConnectionPortInformation;
class vtkMPIMToNSocketConnectionInternals;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkMPIMToNSocketConnection : public vtkObject
{
public:
  static vtkMPIMToNSocketConnection* New();
  vtkTypeMacro(vtkMPIMToNSocketConnection, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(int waiting_process_type);

  /**
   * Setup  the connection.
   */
  void ConnectMtoN();

  //@{
  /**
   * Set the number of connections to be made.
   */
  void SetNumberOfConnections(int);
  vtkGetMacro(NumberOfConnections, int);
  //@}

  /**
   * Set up information about the remote connection.
   */
  void SetPortInformation(unsigned int processNumber, int portNumber, const char* hostName);

  //@{
  /**
   * Return the socket communicator for this process.
   */
  vtkGetObjectMacro(SocketCommunicator, vtkSocketCommunicator);
  //@}

  /**
   * Fill the port information values into the port information object.
   */
  void GetPortInformation(vtkMPIMToNSocketConnectionPortInformation*);

  //@{
  /**
   * Set port to use, if the value is 0, then the system will pick the port.
   */
  vtkGetMacro(PortNumber, int);
  //@}

protected:
  vtkSetMacro(PortNumber, int);

  /**
   * Setup the wait for connection, but do not wait yet.
   * This should determine the network to be used and the port to be used.
   */
  void SetupWaitForConnection();

  /**
   * SetupStartWaitForConnection must be called first.  This
   * method will start waiting for a connection to be made to it.
   */
  void WaitForConnection();

  /**
   * Connect to remote server.
   */
  void Connect();

  virtual void SetController(vtkMultiProcessController*);
  virtual void SetSocketCommunicator(vtkSocketCommunicator*);
  vtkMPIMToNSocketConnection();
  ~vtkMPIMToNSocketConnection() override;

private:
  int PortNumber;
  int Socket;
  vtkServerSocket* ServerSocket;
  int NumberOfConnections;
  vtkMPIMToNSocketConnectionInternals* Internals;
  vtkMultiProcessController* Controller;
  vtkSocketCommunicator* SocketCommunicator;
  vtkMPIMToNSocketConnection(const vtkMPIMToNSocketConnection&) = delete;
  void operator=(const vtkMPIMToNSocketConnection&) = delete;
  bool IsWaiting;
};

#endif
