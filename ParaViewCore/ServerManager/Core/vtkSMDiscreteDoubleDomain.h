/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDiscreteDoubleDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMDiscreteDoubleDomain
 * @brief set of double
 *
 * vtkSMDiscreteDoubleDomain represents a set of double values.
 * It supports a maximum of 256 values.
 *
 * Values are specified in xml as follow:
 *
 * \code{.xml}
 * <DiscreteDoubleDomain values='val1 val2 ... valX' />
 * \endcode
 *
 */

#ifndef vtkSMDiscreteDoubleDomain_h
#define vtkSMDiscreteDoubleDomain_h

#include "vtkPVServerManagerCoreModule.h" // needed for exports
#include "vtkSMDomain.h"

#include <vector> // needed for std::vector

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMDiscreteDoubleDomain : public vtkSMDomain
{
public:
  static vtkSMDiscreteDoubleDomain* New();
  vtkTypeMacro(vtkSMDiscreteDoubleDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the value of the property is in the domain.
   * The property has to be a vtkSMDoubleVectorProperty. If the
   * vector value is in the domain, it returns 1. It returns
   * 0 otherwise.
   */
  int IsInDomain(vtkSMProperty* property) override;

  /**
   * Returns the vector of values.
   */
  std::vector<double> GetValues();

  /**
   * Returns if Values is non empty.
   */
  bool GetValuesExists();

  /**
   * Updates this from the property.
   */
  void Update(vtkSMProperty* property) override;

protected:
  vtkSMDiscreteDoubleDomain();
  ~vtkSMDiscreteDoubleDomain();

  /**
   * Reads the "values" property, with a maximum of 256 values.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  /**
   * A vector of allowed values for the domain.
   */
  std::vector<double> Values;

private:
  vtkSMDiscreteDoubleDomain(const vtkSMDiscreteDoubleDomain&) = delete;
  void operator=(const vtkSMDiscreteDoubleDomain&) = delete;
};

#endif
