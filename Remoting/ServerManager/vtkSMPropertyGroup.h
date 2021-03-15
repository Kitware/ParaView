/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyGroup.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMPropertyGroup_h
#define vtkSMPropertyGroup_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMDocumentation;
class vtkSMProperty;
class vtkSMPropertyGroupInternals;
class vtkSMProxy;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMPropertyGroup : public vtkSMObject
{
public:
  static vtkSMPropertyGroup* New();
  vtkTypeMacro(vtkSMPropertyGroup, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Sets the name of the property group to \p name.
  vtkSetStringMacro(Name)

    // Description:
    // Returns the name of the property group.
    vtkGetStringMacro(Name)

    // Description:
    // Sets the name of the property group to \p name.
    vtkSetStringMacro(XMLLabel)

    // Description:
    // Returns the name of the property group.
    vtkGetStringMacro(XMLLabel)

    // Description:
    // Sets the name of the panel widget to use for the property group.
    vtkSetStringMacro(PanelWidget)

    // Description:
    // Gets the name of the panel widget to use for the property group.
    vtkGetStringMacro(PanelWidget)

    // Description:
    // Sets the panel visibility for the property group.
    //
    // \see vtkSMProperty::SetPanelVisibility()
    vtkSetStringMacro(PanelVisibility)

    // Description:
    // Returns the panel visibility for the property group.
    vtkGetStringMacro(PanelVisibility)

    // Description:
    // Returns true if the property group contains zero properties.
    bool IsEmpty() const;

  // Description:
  // Adds \p property to the group. function can be nullptr.
  void AddProperty(const char* function, vtkSMProperty* property, const char* name = nullptr);

  /**
   * Returns the name for a property at the given index.
   */
  const char* GetPropertyName(unsigned int index) const;

  // Description:
  // Returns the property at \p index.
  vtkSMProperty* GetProperty(unsigned int index) const;

  // Description:
  // Returns the property associated with a given function, if any.
  vtkSMProperty* GetProperty(const char* function) const;

  // Description:
  // Given property in the group, returns its function. Will return nullptr if the
  // property is not present in this group.
  const char* GetFunction(vtkSMProperty* property) const;

  // Description:
  // Returns the number of properties in the group.
  unsigned int GetNumberOfProperties() const;

  // Description:
  // Returns the documentation for this proxy.
  vtkGetObjectMacro(Documentation, vtkSMDocumentation);

  // Description:
  // The server manager configuration XML may define <Hints /> element for
  // a property. Hints are metadata associated with the property. The
  // Server Manager does not (and should not) interpret the hints. Hints
  // provide a mechanism to add GUI pertinant information to the server
  // manager XML.  Returns the XML element for the hints associated with
  // this property, if any, otherwise returns nullptr.
  vtkGetObjectMacro(Hints, vtkPVXMLElement);

protected:
  vtkSMPropertyGroup();
  ~vtkSMPropertyGroup() override;

  friend class vtkSMProxy;
  virtual int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element);

  void SetHints(vtkPVXMLElement* hints);
  vtkPVXMLElement* Hints;

  vtkSMDocumentation* Documentation;

private:
  vtkSMPropertyGroup(const vtkSMPropertyGroup&) = delete;
  void operator=(const vtkSMPropertyGroup&) = delete;

  char* Name;
  char* XMLLabel;
  char* PanelWidget;
  char* PanelVisibility;

  vtkSMPropertyGroupInternals* const Internals;
};

#endif
