/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataTypeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDataTypeDomain - restricts the input proxies to one or more data types
// .SECTION Description
// vtkSMDataTypeDomain restricts the input proxies to one or more data types.
// These data types are specified in the XML with the <DataType> element.
// VTK class names are used. It is possible to specify a superclass
// (i.e. vtkDataSet) for a more general domain. Works with vtkSMSourceProxy
// only. Valid XML elements are:
// @verbatim
// * <DataType value=""> where value is the classname for the data type
// for example: vtkDataSet, vtkImageData,...
// @endverbatim
// .SECTION See Also
// vtkSMDomain  vtkSMSourceProxy

#ifndef __vtkSMDataTypeDomain_h
#define __vtkSMDataTypeDomain_h

#include "vtkSMDomain.h"

class vtkSMSourceProxy;
//BTX
struct vtkSMDataTypeDomainInternals;
//ETX

class VTK_EXPORT vtkSMDataTypeDomain : public vtkSMDomain
{
public:
  static vtkSMDataTypeDomain* New();
  vtkTypeMacro(vtkSMDataTypeDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMProxyProperty which points
  // to a vtkSMSourceProxy. If all data types of the input's
  // parts are in the domain, it returns. It returns 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if all parts of the source proxy are in the domain.
  int IsInDomain(vtkSMSourceProxy* proxy, int outputport=0);

  // Description:
  // Returns the number of acceptable data types.
  unsigned int GetNumberOfDataTypes();

  // Description:
  // Returns a data type.
  const char* GetDataType(unsigned int idx);

//BTX
protected:
  vtkSMDataTypeDomain();
  ~vtkSMDataTypeDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  vtkSMDataTypeDomainInternals* DTInternals;

  int CompositeDataSupported;
  vtkSetMacro(CompositeDataSupported, int);
  vtkGetMacro(CompositeDataSupported, int);

private:
  vtkSMDataTypeDomain(const vtkSMDataTypeDomain&); // Not implemented
  void operator=(const vtkSMDataTypeDomain&); // Not implemented
//ETX
};

#endif
