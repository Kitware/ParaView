/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateVersionControllerBase.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStateVersionControllerBase
// .SECTION Description
// vtkSMStateVersionControllerBase is used to convert the state XML from any
// previously supported versions to the current version.
// This class merely defines the API.
// .SECTION See Also
// vtkSMStateVersionController

#ifndef __vtkSMStateVersionControllerBase_h
#define __vtkSMStateVersionControllerBase_h

#include "vtkSMObject.h"

class vtkPVXMLElement;

class VTK_EXPORT vtkSMStateVersionControllerBase : public vtkSMObject
{
public:
  vtkTypeMacro(vtkSMStateVersionControllerBase, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called before a state is loaded. 
  // The argument must be the root element for the state being loaded.
  // eg. for server manager state, it will point to <ServerManagerState />
  // element.
  // Returns false if the conversion failed, else true.
  virtual bool Process(vtkPVXMLElement* root) = 0;

//BTX
  // Description:
  // Select all 1-level deep children of root with name=childName and
  // with attributes specified in the childAttrs (if non-null)
  // and invoke the callback for all such matches.
  void Select(vtkPVXMLElement* root,
    const char* childName,
    const char* childAttrs[],
    bool (*funcPtr)(vtkPVXMLElement*, void*),
    void* callData);

  // Description:
  // Select all 1-level deep children of the root with name=childName and with
  // attributes specified in the childAttrs (if non-null) and set the newAttrs
  // on it. Both childAttrs and newAttrs must be terminated by 0.
  void SelectAndSetAttributes(vtkPVXMLElement* root,
    const char* childName,
    const char* childAttrs[],
    const char* newAttrs[]);

protected:
  vtkSMStateVersionControllerBase();
  ~vtkSMStateVersionControllerBase();

  // Reads the version string are returns fills the argument array with
  // major, minor, patch version numbers.
  void ReadVersion(vtkPVXMLElement*, int version[3]);

  inline int GetMajor(int version[3])
    { return version[0]; }

  inline int GetMinor(int version[3])
    { return version[1]; }

  inline int GetPatch(int version[3])
    { return version[2]; }

  // Description:
  // Updates version to be atleast as big as minversion.
  void UpdateVersion(int version[3], int minversion[3])
    {
    for (int cc=0; cc < 3; cc++)
      {
      if (version[cc] < minversion[cc])
        {
        for (int kk=cc; kk<3; kk++)
          {
          version[kk] = minversion[kk];
          }
        break;
        }
      }
    }

  void SelectAndRemove(vtkPVXMLElement* root,
    const char* childName,
    const char* childAttrs[]);


private:
  vtkSMStateVersionControllerBase(const vtkSMStateVersionControllerBase&); // Not implemented
  void operator=(const vtkSMStateVersionControllerBase&); // Not implemented
//ETX
};

#endif

