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
// .NAME vtkMPIMToNSocketConnectionPortInformation - Holds class name
// .SECTION Description
// This information object gets the class name of the input VTK object.  This
// is separate from vtkPVDataInformation because it can be determined before
// Update is called and because it operates on any VTK object.

#ifndef __vtkMPIMToNSocketConnectionPortInformation_h
#define __vtkMPIMToNSocketConnectionPortInformation_h

#include "vtkPVInformation.h"

class vtkMPIMToNSocketConnectionPortInformationInternals;


class VTK_EXPORT vtkMPIMToNSocketConnectionPortInformation : public vtkPVInformation
{
public:
  static vtkMPIMToNSocketConnectionPortInformation* New();
  vtkTypeRevisionMacro(vtkMPIMToNSocketConnectionPortInformation, vtkPVInformation);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get class name of VTK object.
  vtkGetStringMacro(HostName);

  // Description:
  // Set/Get the ProcessNumber
  vtkSetMacro(ProcessNumber, int);
  vtkGetMacro(ProcessNumber, int);
  // Description:
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
  virtual void CopyToStream(vtkClientServerStream*) const;
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
