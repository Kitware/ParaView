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

#ifndef __vtkSMPropertyGroup_h
#define __vtkSMPropertyGroup_h

#include "vtkSMObject.h"

class vtkSMProperty;
class vtkSMPropertyGroupInternals;

class VTK_EXPORT vtkSMPropertyGroup : public vtkSMObject
{
public:
  static vtkSMPropertyGroup* New();
  vtkTypeMacro(vtkSMPropertyGroup, vtkSMObject)
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // Sets the type of the property group to \p type.
  vtkSetStringMacro(Type)

  // Description:
  // Returns the type of the property group.
  vtkGetStringMacro(Type)

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
  // Adds \p property to the group.
  void AddProperty(vtkSMProperty *property);

  // Description:
  // Returns the property at \p index.
  vtkSMProperty* GetProperty(unsigned int index) const;

  // Description:
  // Returns the number of properties in the group.
  unsigned int GetNumberOfProperties() const;

protected:
  vtkSMPropertyGroup();
  ~vtkSMPropertyGroup();

private:
  vtkSMPropertyGroup(const vtkSMPropertyGroup&); // Not implemented
  void operator=(const vtkSMPropertyGroup&); // Not implemented

  char *Name;
  char *XMLLabel;
  char *Type;
  char *PanelVisibility;

  vtkSMPropertyGroupInternals* const Internals;
};

#endif
