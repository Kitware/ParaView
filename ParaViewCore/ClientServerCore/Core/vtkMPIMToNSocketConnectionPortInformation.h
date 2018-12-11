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
/**
 * @class   vtkMPIMToNSocketConnectionPortInformation
 * @brief   holds port and host name
 * information.
 *
 * and host information from a render server.  This information is used by
 * the data server to make the connections to the render server processes.
*/

#ifndef vtkMPIMToNSocketConnectionPortInformation_h
#define vtkMPIMToNSocketConnectionPortInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkMPIMToNSocketConnectionPortInformationInternals;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkMPIMToNSocketConnectionPortInformation
  : public vtkPVInformation
{
public:
  static vtkMPIMToNSocketConnectionPortInformation* New();
  vtkTypeMacro(vtkMPIMToNSocketConnectionPortInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the port and host information for a specific process number.
   */
  void SetConnectionInformation(unsigned int processNumber, int portNumber, const char* hostname);

  /**
   * Set/Get the number of connections.
   */
  int GetNumberOfConnections();

  //@{
  /**
   * Access information about a particular process.
   */
  int GetProcessPort(unsigned int processNumber);
  const char* GetProcessHostName(unsigned int processNumber);
  //@}

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

protected:
  vtkMPIMToNSocketConnectionPortInformation();
  ~vtkMPIMToNSocketConnectionPortInformation() override;

  int NumberOfConnections;
  vtkMPIMToNSocketConnectionPortInformationInternals* Internals;

private:
  vtkMPIMToNSocketConnectionPortInformation(
    const vtkMPIMToNSocketConnectionPortInformation&) = delete;
  void operator=(const vtkMPIMToNSocketConnectionPortInformation&) = delete;
};

#endif
