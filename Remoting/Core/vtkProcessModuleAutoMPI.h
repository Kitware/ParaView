/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModuleAutoMPI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkProcessModuleAutoMPI_h
#define vtkProcessModuleAutoMPI_h

#include "vtkClientServerID.h" // needed for UniqueID.
#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class vtkProcessModuleAutoMPIInternals;
class VTKREMOTINGCORE_EXPORT vtkProcessModuleAutoMPI : public vtkObject
{
public:
  static vtkProcessModuleAutoMPI* New();
  vtkTypeMacro(vtkProcessModuleAutoMPI, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static bool EnableAutoMPI;
  static int NumberOfCores;
  static void SetEnableAutoMPI(bool val);
  static void SetNumberOfCores(int val);

  vtkProcessModuleAutoMPI();
  ~vtkProcessModuleAutoMPI() override;

  // Description:
  // To determine if it is possible to use multi-core on the system.
  // It returns 1 if possible and 0 if not.
  int IsPossible();

  // Description:
  // This method is called if the system running paraview has
  // multicores. When called the systems starts N pvservers on MPI
  // where N is the total number of cores available. The method first
  // scans for an available free port and starts the server on that
  // port. The port over which the connection is made is returned for
  // the client to consequently connect to it. Returns 0 on failure.
  int ConnectToRemoteBuiltInSelf();

private:
  vtkProcessModuleAutoMPI(const vtkProcessModuleAutoMPI&) = delete;
  void operator=(const vtkProcessModuleAutoMPI&) = delete;
  vtkProcessModuleAutoMPIInternals* Internals;
};

#endif /* !vtkProcessModuleAutoMPI_h */
