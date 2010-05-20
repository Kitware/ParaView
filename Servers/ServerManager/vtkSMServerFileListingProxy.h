/*=========================================================================

  Program:   ParaView
  Module:    vtkSMServerFileListingProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMServerFileListingProxy - Proxy for vtkPVServerFileListing.
// .SECTION Description
// This is the proxy for vtkPVServerFileListing.
// This needs to be a special proxy to provide from a property interface
// suitable to query methods such as "FileIsReadable", "FileIsDirectory".

#ifndef __vtkSMServerFileListingProxy_h
#define __vtkSMServerFileListingProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMServerFileListingProxy : public vtkSMProxy
{
public:
  static vtkSMServerFileListingProxy* New();
  vtkTypeMacro(vtkSMServerFileListingProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Get/Set the file/dir name on whose information is requested.
  // All queries "FileIsDirectory" and "FileIsReadable" are against
  // this name.
  void SetActiveFileName(const char* name);
  vtkGetStringMacro(ActiveFileName);

  // Description:
  // Query if the ActiveFileName refers to a directory.
  vtkGetMacro(ActiveFileIsReadable, int);

  // Description:
  // Query is the ActiveFileName refers to a readable file.
  vtkGetMacro(ActiveFileIsDirectory, int);

  // Description:
  // Updates all property informations by calling UpdateInformation()
  // and populating the values. It also calls UpdateDependentDomains()
  // on all properties to make sure that domains that depend on the
  // information are updated.
  virtual void UpdatePropertyInformation();

  // Description:
  // Similar to UpdatePropertyInformation() but updates only the given property.
  // If the property does not belong to the proxy, the call is ignored.
  virtual void UpdatePropertyInformation(vtkSMProperty* prop)
    { this->Superclass::UpdatePropertyInformation(prop); }
    
protected:
  vtkSMServerFileListingProxy();
  ~vtkSMServerFileListingProxy();
  
  int ActiveFileIsReadable;
  int ActiveFileIsDirectory;
  char* ActiveFileName;
  
private:
  vtkSMServerFileListingProxy(const vtkSMServerFileListingProxy&); // Not implemented.
  void operator=(const vtkSMServerFileListingProxy&); // Not implemented.
};


#endif


