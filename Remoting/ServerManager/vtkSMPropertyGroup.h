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

  //@{
  /**
   * Get/Sets the name of the property group to \p name.
   */
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);
  //@}

  //@{
  /**
   * Gets/Sets the name of the property group to \p name.
   */
  vtkSetStringMacro(XMLLabel);
  vtkGetStringMacro(XMLLabel);
  //@}

  //@{
  /**
   * Get/Sets the name of the panel widget to use for the property group.
   */
  vtkSetStringMacro(PanelWidget);
  vtkGetStringMacro(PanelWidget);
  //@}

  //@{
  /**
   * Get/Sets the panel visibility for the property group.
   * @sa vtkSMProperty::SetPanelVisibility()
   */
  vtkSetStringMacro(PanelVisibility);
  vtkGetStringMacro(PanelVisibility);
  //@}

  /**
   * Returns true if the property group contains zero properties.
   */
  bool IsEmpty() const;

  /**
   * Adds \p property to the group.
   * `name` is the name of the property. If nullptr, then the property's XML
   * name is used.
   * `function` is the role assigned to this property. If nullptr, the name
   * (either specified as argument, or the XML name for the property) will be
   * used.
   */
  void AddProperty(const char* function, vtkSMProperty* property, const char* name = nullptr);

  /**
   * Returns the name for a property at the given index.
   */
  const char* GetPropertyName(unsigned int index) const;

  /**
   * Returns the property at \p index.
   */
  vtkSMProperty* GetProperty(unsigned int index) const;

  /**
   * Returns the property associated with a given function, if any.
   */
  vtkSMProperty* GetProperty(const char* function) const;

  /**
   * Given property in the group, returns its function. Will return nullptr if the
   * property is not present in this group.
   */
  const char* GetFunction(vtkSMProperty* property) const;

  /**
   * Returns the number of properties in the group.
   */
  unsigned int GetNumberOfProperties() const;

  /**
   * Returns the documentation for this proxy.
   */
  vtkGetObjectMacro(Documentation, vtkSMDocumentation);

  //@{
  /**
   * The server manager configuration XML may define `<Hints />` element
   * for a property-group. Hints are metadata associated with the property-group.
   * The Server Manager does not (and should not) interpret the hints.
   * Hints provide a mechanism to add GUI pertinent information to the
   * server manager XML.  Returns the XML element for the hints associated
   * with this property, if any, otherwise returns `nullptr`;
   */
  vtkGetObjectMacro(Hints, vtkPVXMLElement);
  void SetHints(vtkPVXMLElement* hints);
  //@}

protected:
  vtkSMPropertyGroup();
  ~vtkSMPropertyGroup() override;

  friend class vtkSMProxy;
  virtual int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element);

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
