/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSILInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSILInformation
// .SECTION Description
// Information object used to retreived the SIL graph from a file reader or
// any compatible source.

#ifndef __vtkPVSILInformation_h
#define __vtkPVSILInformation_h

#include "vtkPVInformation.h"

class vtkGraph;

class VTK_EXPORT vtkPVSILInformation : public vtkPVInformation
{
public:
  static vtkPVSILInformation* New();
  vtkTypeMacro(vtkPVSILInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  //BTX
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  //ETX

  // Description:
  // Returns the SIL.
  vtkGetObjectMacro(SIL, vtkGraph);

//BTX
protected:
  vtkPVSILInformation();
  ~vtkPVSILInformation();

  void SetSIL(vtkGraph*);
  vtkGraph* SIL;
private:
  vtkPVSILInformation(const vtkPVSILInformation&); // Not implemented
  void operator=(const vtkPVSILInformation&); // Not implemented
//ETX
};

#endif

