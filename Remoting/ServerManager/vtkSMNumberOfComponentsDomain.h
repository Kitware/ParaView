/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNumberOfComponentsDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMNumberOfComponentsDomain
 * @brief   int range domain based on the number of
 * components available in a particular data array.
 *
 * vtkSMNumberOfComponentsDomain is used for properties that allow the user to
 * choose the component number (or associated name) to process for the chosen array.
 * It needs two required properties with following functions:
 * * Input -- input property for the filter.
 * * ArraySelection -- string vector property used to select the array.
 * This domain will not work if either of the required properties is missing.
*/

#ifndef vtkSMNumberOfComponentsDomain_h
#define vtkSMNumberOfComponentsDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMEnumerationDomain.h"

class vtkSMSourceProxy;
class vtkSMInputArrayDomain;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMNumberOfComponentsDomain : public vtkSMEnumerationDomain
{
public:
  static vtkSMNumberOfComponentsDomain* New();
  vtkTypeMacro(vtkSMNumberOfComponentsDomain, vtkSMEnumerationDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Updates the range based on the scalar range of the currently selected
   * array. This requires Input (vtkSMProxyProperty) and ArraySelection
   * (vtkSMStringVectorProperty) properties. Currently, this uses
   * only the first component of the array.
   */
  void Update(vtkSMProperty* prop) override;

protected:
  vtkSMNumberOfComponentsDomain();
  ~vtkSMNumberOfComponentsDomain() override;

  /**
   * Set the appropriate ivars from the xml element.
   */
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  /**
   * Internal update method doing the actual work.
   */
  void Update(
    const char* arrayname, vtkSMSourceProxy* sp, vtkSMInputArrayDomain* iad, int outputport);

private:
  vtkSMNumberOfComponentsDomain(const vtkSMNumberOfComponentsDomain&) = delete;
  void operator=(const vtkSMNumberOfComponentsDomain&) = delete;

  bool EnableMagnitude;
};

#endif
