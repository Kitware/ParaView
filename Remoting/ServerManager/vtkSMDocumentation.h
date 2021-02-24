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
/**
 * @class   vtkSMDocumentation
 * @brief   class providing access to the documentation
 * for a vtkSMProxy.
 *
 * Every proxy defined in the server manager XML can have documentation
 * associated with it. This class provides access to the various types
 * of documentation text for every proxy.
*/

#ifndef vtkSMDocumentation_h
#define vtkSMDocumentation_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkPVXMLElement;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDocumentation : public vtkSMObject
{
public:
  static vtkSMDocumentation* New();
  vtkTypeMacro(vtkSMDocumentation, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the text for long help, if any. nullptr otherwise.
   */
  const char* GetLongHelp();

  /**
   * Returns the text for short help, if any. nullptr otherwise.
   */
  const char* GetShortHelp();

  /**
   * Returns the description text, if any.
   */
  const char* GetDescription();

  //@{
  /**
   * Get/Set the documentation XML element.
   */
  void SetDocumentationElement(vtkPVXMLElement*);
  vtkGetObjectMacro(DocumentationElement, vtkPVXMLElement);

protected:
  vtkSMDocumentation();
  ~vtkSMDocumentation() override;
  //@}

  vtkPVXMLElement* DocumentationElement;

private:
  vtkSMDocumentation(const vtkSMDocumentation&) = delete;
  void operator=(const vtkSMDocumentation&) = delete;
};

#endif
