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
// .NAME vtkSMXMLParser
// .SECTION Description
#ifndef __vtkSMXMLParser_h
#define __vtkSMXMLParser_h

#include "vtkPVXMLParser.h"

class vtkSMProxy;
class vtkSMProperty;
class vtkSMProxyManager;

class VTK_EXPORT vtkSMXMLParser : public vtkPVXMLParser
{
public:
  vtkTypeRevisionMacro(vtkSMXMLParser,vtkPVXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSMXMLParser* New();

  // Description:
  void ProcessConfiguration(vtkSMProxyManager* manager);

protected:
  vtkSMXMLParser();
  ~vtkSMXMLParser();

  void ProcessGroup(vtkPVXMLElement* group, vtkSMProxyManager* manager);

private:
  vtkSMXMLParser(const vtkSMXMLParser&);  // Not implemented.
  void operator=(const vtkSMXMLParser&);  // Not implemented.
};

#endif
