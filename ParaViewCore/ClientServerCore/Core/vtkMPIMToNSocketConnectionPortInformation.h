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

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkMPIMToNSocketConnectionPortInformationInternals;


class VTKPVCLIENTSERVERCORECORE_EXPORT vtkMPIMToNSocketConnectionPortInformation : public vtkPVInformation
{
public:
  static vtkMPIMToNSocketConnectionPortInformation* New();
  vtkTypeMacro(vtkMPIMToNSocketConnectionPortInformation, vtkPVInformation);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set the port and host information for a specific process number.
  void SetConnectionInformation(
    unsigned int processNumber, int portNumber, const char* hostname);

  // Description:
  // Set/Get the number of connections.
  int GetNumberOfConnections();
 
  // Description:
  // Access information about a particular process.
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

protected:
  vtkMPIMToNSocketConnectionPortInformation();
  ~vtkMPIMToNSocketConnectionPortInformation();

  int NumberOfConnections;
  vtkMPIMToNSocketConnectionPortInformationInternals* Internals;
private:
  vtkMPIMToNSocketConnectionPortInformation(const vtkMPIMToNSocketConnectionPortInformation&); // Not implemented
  void operator=(const vtkMPIMToNSocketConnectionPortInformation&); // Not implemented
};

#endif
