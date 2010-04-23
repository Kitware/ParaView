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
// of the input lists. The class was created to merge vtkPropCollections inside
// vtkAreaPicker's for ParaView. The CopyObject method tries to cast the 
// given pointer into a vtkAreaPicker and then gets the IDs for each prop in 
// its collection.

#ifndef __vtkPVClientServerIdCollectionInformation_h
#define __vtkPVClientServerIdCollectionInformation_h

#include "vtkPVInformation.h"
#include "vtkClientServerID.h" // for vtkClientServerID

class vtkClientServerIdSetType;

class VTK_EXPORT vtkPVClientServerIdCollectionInformation 
  : public vtkPVInformation
{
public:
  static vtkPVClientServerIdCollectionInformation* New();
  vtkTypeMacro(vtkPVClientServerIdCollectionInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object. Calls AddInformation(info, 0).
  virtual void AddInformation(vtkPVInformation* info);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

//BTX
  // Description:
  // Returns the number of IDs held.
  int GetLength();

  // Description:
  // Returns the i'th ID in my set.
  vtkClientServerID GetID(int i);

  // Description:
  // Returns true if the given ID is in my set.
  int Contains(vtkClientServerID id);
//ETX

protected:
  vtkPVClientServerIdCollectionInformation();
  ~vtkPVClientServerIdCollectionInformation();

  vtkClientServerIdSetType *ClientServerIds;

private:
  vtkPVClientServerIdCollectionInformation
    (const vtkPVClientServerIdCollectionInformation&); // Not implemented
  void operator=
    (const vtkPVClientServerIdCollectionInformation&); // Not implemented
};

#endif
