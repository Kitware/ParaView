/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerXDMFParameters.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerXDMFParameters - Server-side helper for vtkPVArraySelection.
// .SECTION Description

#ifndef __vtkPVServerXDMFParameters_h
#define __vtkPVServerXDMFParameters_h

#include "vtkPVServerObject.h"

class vtkClientServerStream;
class vtkPVServerXDMFParametersInternals;
class vtkXdmfReader;

class VTK_EXPORT vtkPVServerXDMFParameters : public vtkPVServerObject
{
public:
  static vtkPVServerXDMFParameters* New();
  vtkTypeRevisionMacro(vtkPVServerXDMFParameters, vtkPVServerObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the set of parameters that may be specified for the given
  // XDMF reader.
  const vtkClientServerStream& GetParameters(vtkXdmfReader*);

protected:
  vtkPVServerXDMFParameters();
  ~vtkPVServerXDMFParameters();

  vtkPVServerXDMFParametersInternals* Internal;
private:
  vtkPVServerXDMFParameters(const vtkPVServerXDMFParameters&); // Not implemented
  void operator=(const vtkPVServerXDMFParameters&); // Not implemented
};

#endif
