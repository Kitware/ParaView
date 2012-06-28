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
#ifndef   	__vtkProcessModuleAutoMPI_h
# define   	__vtkProcessModuleAutoMPI_h


#include "vtkObject.h"
#include "vtkClientServerID.h" // needed for UniqueID.

class vtkProcessModuleAutoMPIInternals;
class VTK_EXPORT vtkProcessModuleAutoMPI: public vtkObject
{
public:
  static vtkProcessModuleAutoMPI* New();
  vtkTypeMacro(vtkProcessModuleAutoMPI, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static int UseMulticoreProcessors;
  static void SetUseMulticoreProcessors(int val);
  vtkProcessModuleAutoMPI();
  ~vtkProcessModuleAutoMPI();

  // Description:
  // To determine if it is possible to use multi-core on the system.
  // It returns 1 if possible and 0 if not.
  int IsPossible();

  // Description:
  // This method is called if the system running paraview has
  // multicores. When called the systems starts N pvservers on MPI
  // where N is the total number of cores available. The method first
  // scans for an available free port and starts the server on that
  // port. The port over which the connectio is made is returned for
  // the client to consequitively connect to it.
  int ConnectToRemoteBuiltInSelf ();

private:
  vtkProcessModuleAutoMPI(const vtkProcessModuleAutoMPI&); // Not implemented.
  void operator=(const vtkProcessModuleAutoMPI&); // Not implemented.
  vtkProcessModuleAutoMPIInternals *Internals;
};


#endif 	    /* !__vktProcessModuleAutoMPI_h */
