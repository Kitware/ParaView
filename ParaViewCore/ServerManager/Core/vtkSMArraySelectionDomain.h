/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArraySelectionDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMArraySelectionDomain
 * @brief   used on properties that allow users to
 * select arrays.
 *
 * vtkSMArraySelectionDomain is a domain that can be for used for properties
 * that allow users to set selection-statuses for multiple arrays (or similar
 * items). This is similar to vtkSMArrayListDomain, the only different is that
 * vtkSMArrayListDomain is designed to work with data-information obtained
 * from the required Input property, while vtkSMArraySelectionDomain depends on
 * a required information-only property ("ArrayList") that provides the
 * arrays available.
 *
 * Supported Required-Property functions:
 * \li \c ArrayList : points a string-vector property that produces the
 * (array_name, status) tuples. This is typically an information-only property.
*/

#ifndef vtkSMArraySelectionDomain_h
#define vtkSMArraySelectionDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMStringListDomain.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMArraySelectionDomain : public vtkSMStringListDomain
{
public:
  static vtkSMArraySelectionDomain* New();
  vtkTypeMacro(vtkSMArraySelectionDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Since this domain relies on an information only property to get the default
   * status, we override this method to copy the values the info property as the
   * default array selection.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

  /**
   * Global flag to toggle between (a) the default behavior of setting default
   * values according to infoProperty and (b) setting all default values to on.
   */
  static void SetLoadAllVariables(bool choice)
  {
    vtkSMArraySelectionDomain::LoadAllVariables = choice;
  }
  static bool GetLoadAllVariables() { return vtkSMArraySelectionDomain::LoadAllVariables; }

protected:
  vtkSMArraySelectionDomain();
  ~vtkSMArraySelectionDomain() override;

  static bool LoadAllVariables;

private:
  vtkSMArraySelectionDomain(const vtkSMArraySelectionDomain&) = delete;
  void operator=(const vtkSMArraySelectionDomain&) = delete;
};

#endif
