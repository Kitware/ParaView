/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDocumentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDocumentation - class providing access to the documentation
// for a vtkSMProxy.
// .SECTION Description
// Every proxy defined in the server manager XML can have documentation
// associated with it. This class provides access to the various types
// of documentation text for every proxy.

#ifndef __vtkSMDocumentation_h
#define __vtkSMDocumentation_h

#include "vtkSMObject.h"

class vtkPVXMLElement;

class VTK_EXPORT vtkSMDocumentation : public vtkSMObject
{
public:
  static vtkSMDocumentation* New();
  vtkTypeMacro(vtkSMDocumentation, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the text for long help, if any. NULL otherwise.
  const char* GetLongHelp();

  // Description:
  // Returns the text for short help, if any. NULL otherwise.
  const char* GetShortHelp();

  // Description:
  // Returns the description text, if any.
  const char* GetDescription();
 
  // Description:
  // Get/Set the documentation XML element.
  void SetDocumentationElement(vtkPVXMLElement*);
  vtkGetObjectMacro(DocumentationElement, vtkPVXMLElement);
protected:
  vtkSMDocumentation();
  ~vtkSMDocumentation();
  
  vtkPVXMLElement* DocumentationElement;
private:
  vtkSMDocumentation(const vtkSMDocumentation&); // Not implemented.
  void operator=(const vtkSMDocumentation&); // Not implemented.
};


#endif

