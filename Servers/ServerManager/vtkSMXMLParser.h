/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXMLParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMXMLParser - parser of server manager configuration files
// .SECTION Description
// vtkSMXMLParser parses configuration files (or strings) and stores
// the resulting elements in a proxy manager.
// .SECTION See Also
// vtkSMProxyManager

#ifndef __vtkSMXMLParser_h
#define __vtkSMXMLParser_h

#include "vtkPVXMLParser.h"

class vtkSMProxy;
class vtkSMProperty;
class vtkSMProxyManager;

class VTK_EXPORT vtkSMXMLParser : public vtkPVXMLParser
{
public:
  vtkTypeMacro(vtkSMXMLParser,vtkPVXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSMXMLParser* New();

  // Description:
  // Processes the xml data structures and creates the appropriate
  // map entries (based on group name and proxy name) in the proxy manager.
  // Should be called after Parse().
  void ProcessConfiguration(vtkSMProxyManager* manager);

protected:
  vtkSMXMLParser();
  ~vtkSMXMLParser();

  // Called by ProcessConfiguration(), processes all proxies in one
  // group and passes them to the proxy manager.
  void ProcessGroup(vtkPVXMLElement* group, vtkSMProxyManager* manager);

private:
  vtkSMXMLParser(const vtkSMXMLParser&);  // Not implemented.
  void operator=(const vtkSMXMLParser&);  // Not implemented.
};

#endif
