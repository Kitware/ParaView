/*=========================================================================

  Program:   ParaView
  Module:    vtkMPIMToNSocketConnectionPortInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMPIMToNSocketConnectionPortInformation - holds port and host name
// information.  
// .SECTION Description This information object gets the port
// and host information from a render server.  This information is used by
// the data server to make the connections to the render server processes.

#ifndef __vtkMPIMToNSocketConnectionPortInformation_h
#define __vtkMPIMToNSocketConnectionPortInformation_h

#include "vtkPVInformation.h"

class vtkMPIMToNSocketConnectionPortInformationInternals;


class VTK_EXPORT vtkMPIMToNSocketConnectionPortInformation : public vtkPVInformation
{
public:
  static vtkMPIMToNSocketConnectionPortInformation* New();
  vtkTypeMacro(vtkMPIMToNSocketConnectionPortInformation, vtkPVInformation);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get class name of VTK object.
  vtkGetStringMacro(HostName);

  // Description:
  // Set/Get the ProcessNumber
  vtkSetMacro(ProcessNumber, int);
  vtkGetMacro(ProcessNumber, int);
  // description:
  // Set/Get the ProcessNumber
  vtkSetMacro(PortNumber, int);
  vtkGetMacro(PortNumber, int);
  
  // Description:
  // Set the port and host information for a specific process number.
  void SetPortNumber(unsigned int processNumber, int port);
  void SetHostName(unsigned int processNumber, const char* host);
  
  // Description:
  // Set/Get the number of connections.
  vtkSetMacro(NumberOfConnections, int);
  vtkGetMacro(NumberOfConnections, int);
  
  int GetProcessPort(unsigned int processNumber);
  const char* GetProcessHostName(unsigned int processNumber);
  
  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  // Description:
  // Set the host name.
  vtkSetStringMacro(HostName);
protected:
  vtkMPIMToNSocketConnectionPortInformation();
  ~vtkMPIMToNSocketConnectionPortInformation();

  char* HostName;
  int NumberOfConnections;
  int ProcessNumber;
  int PortNumber;
  vtkMPIMToNSocketConnectionPortInformationInternals* Internals;
private:
  vtkMPIMToNSocketConnectionPortInformation(const vtkMPIMToNSocketConnectionPortInformation&); // Not implemented
  void operator=(const vtkMPIMToNSocketConnectionPortInformation&); // Not implemented
};

#endif
