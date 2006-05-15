/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerIdCollectionInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPropCollectionInformation - A gatherable list of ClientServerIds.
// .SECTION Description
// This class is used to gather a list of vtkClientServerIds. The resulting 
// list will contain one copy of every id that is contained in any 
// of the input lists. The class is used by SMRenderModuleProxy to gather a
// list of ClientServerIds corresponding to 

#ifndef __vtkPVClientServerIdCollectionInformation_h
#define __vtkPVClientServerIdCollectionInformation_h

#include "vtkPVInformation.h"

class vtkClientServerIdSetType;
class vtkClientServerID;

class VTK_EXPORT vtkPVClientServerIdCollectionInformation 
  : public vtkPVInformation
{
public:
  static vtkPVClientServerIdCollectionInformation* New();
  vtkTypeRevisionMacro(vtkPVClientServerIdCollectionInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object. Calls AddInformation(info, 0).
  virtual void AddInformation(vtkPVInformation* info);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*) const;
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // For debugging.
  int Contains(vtkClientServerID *id);

protected:
  vtkPVClientServerIdCollectionInformation();
  ~vtkPVClientServerIdCollectionInformation();

  vtkClientServerIdSetType *ClientServerIdIds;

private:
  vtkPVClientServerIdCollectionInformation
    (const vtkPVClientServerIdCollectionInformation&); // Not implemented
  void operator=
    (const vtkPVClientServerIdCollectionInformation&); // Not implemented
};

#endif
