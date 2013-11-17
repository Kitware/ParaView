/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDirectoryProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIDirectoryProxy
// .SECTION Description
// vtkSIDirectoryProxy is the server-implementation for a vtkSMDirectory
// which will customly handle server file listing for the pull request


#ifndef __vtkSIDirectoryProxy_h
#define __vtkSIDirectoryProxy_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIProxy.h"

class vtkAlgorithmOutput;
class vtkSIProperty;
class vtkPVXMLElement;
class vtkSIProxyDefinitionManager;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIDirectoryProxy : public vtkSIProxy
{
public:
  static vtkSIDirectoryProxy* New();
  vtkTypeMacro(vtkSIDirectoryProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Pull the current state of the underneath implementation
  virtual void Pull(vtkSMMessage* msg);
//ETX

//BTX
protected:
  vtkSIDirectoryProxy();
  ~vtkSIDirectoryProxy();

  // We override it to skip the fake properties (DirectoryList, FileList)
  virtual bool ReadXMLProperty(vtkPVXMLElement* property_element);

private:
  vtkSIDirectoryProxy(const vtkSIDirectoryProxy&); // Not implemented
  void operator=(const vtkSIDirectoryProxy&); // Not implemented

//ETX
};

#endif
