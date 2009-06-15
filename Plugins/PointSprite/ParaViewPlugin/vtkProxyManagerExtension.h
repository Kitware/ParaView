/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProxyManagerExtension.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkProxyManagerExtension
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#ifndef __vtkProxyManagerExtension_h
#define __vtkProxyManagerExtension_h

#include "vtkSMProxyManagerExtension.h"

class vtkProxyManagerExtension : public vtkSMProxyManagerExtension
{
public:
  static vtkProxyManagerExtension* New();
  vtkTypeRevisionMacro(vtkProxyManagerExtension, vtkSMProxyManagerExtension);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is called when an extension is registered with the proxy manager to
  // ensure that the extension is compatible with the proxy manager version.
  // Incompatible extensions are not registered with the proxy manager.
  virtual bool CheckCompatibility(int major, int minor, int patch);

  // Description:
  // Given the proxy name and group name, returns the XML element for
  // the proxy.
  virtual vtkPVXMLElement* GetProxyElement(const char* groupName,
    const char* proxyName, vtkPVXMLElement* currentElement);

//BTX
protected:
  vtkProxyManagerExtension();
  ~vtkProxyManagerExtension();

private:
  vtkProxyManagerExtension(const vtkProxyManagerExtension&); // Not implemented.
  void operator=(const vtkProxyManagerExtension&); // Not implemented.

  class vtkMapOfElements;
  vtkMapOfElements* MapOfElements;
//ETX
};

#endif


