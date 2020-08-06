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
/**
 * @class   vtkSMDataTypeDomain
 * @brief   restricts the input proxies to one or more data types
 *
 * vtkSMDataTypeDomain restricts the input proxies to one or more data types.
 * These data types are specified in the XML with the \c \<DataType\> element.
 * VTK class names are used. It is possible to specify a superclass
 * (i.e. vtkDataSet) for a more general domain. Works with vtkSMSourceProxy
 * only. Valid XML elements are:
 * @verbatim
 * * <DataType value=""> where value is the classname for the data type
 * for example: vtkDataSet, vtkImageData,...
 * @endverbatim
 * Optional XML attributes for composite datasets only:
 * \li \c child_match : Value can be "any" or "all". This is used if specific
 * types of child datasets are required. "any" indicates that composite datasets
 * with any child matching the nested data types are in the domain. "all"
 * indicates that all the child datasets must match the nested data types(can be
 * mix-match). Example:
 * \code
 * <DataType value="vtkCompositeDataSet" child_match="any">
 *     <DataType value="vtkImageData" />
 * </DataType>
 * <DataType value="vtkMultiBlockDataSet" child_match="all">
 *     <DataType value="vtkImageData" />
 *     <DataType value="vtkUnstructuredGrid" />
 * </DataType>
 * \endcode
 * The first element accepts vtkCompositeDataSet with at least one child dataset
 * of vtkImageData. The second element accepts vtkMultiBlockDataSet with all the
 * child datasets being either vtkImageData or vtkUnstructuredGrid.
 *
 * @sa
 * vtkSMDomain  vtkSMSourceProxy
*/

#ifndef vtkSMDataTypeDomain_h
#define vtkSMDataTypeDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDomain.h"

#include <string>
#include <vector>

class vtkSMSourceProxy;

struct vtkSMDataTypeDomainInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDataTypeDomain : public vtkSMDomain
{
public:
  static vtkSMDataTypeDomain* New();
  vtkTypeMacro(vtkSMDataTypeDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the value of the property is in the domain.
   * The property has to be a vtkSMProxyProperty which points
   * to a vtkSMSourceProxy. If all data types of the input's
   * parts are in the domain, it returns. It returns 0 otherwise.
   */
  int IsInDomain(vtkSMProperty* property) override;

  /**
   * Returns true if all parts of the source proxy are in the domain.
   */
  int IsInDomain(vtkSMSourceProxy* proxy, int outputport = 0);

  /**
   * Returns the number of acceptable data types.
   */
  unsigned int GetNumberOfDataTypes();

  /**
   * Returns a data type.
   */
  const char* GetDataTypeName(unsigned int idx);

  /**
   * Returns if the data type is composite.
   */
  bool DataTypeHasChildren(unsigned int idx);

  /**
   * Returns if the child match type of a composite data type
   */
  const char* GetDataTypeChildMatchTypeAsString(unsigned int idx);

  /**
   * Returns the allowed child types of a composite data type
   */
  const std::vector<std::string>& GetDataTypeChildren(unsigned int idx);

protected:
  vtkSMDataTypeDomain();
  ~vtkSMDataTypeDomain() override;

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  vtkSMDataTypeDomainInternals* DTInternals;

  int CompositeDataSupported;
  vtkSetMacro(CompositeDataSupported, int);
  vtkGetMacro(CompositeDataSupported, int);

  int CompositeDataRequired;
  vtkSetMacro(CompositeDataRequired, int);
  vtkGetMacro(CompositeDataRequired, int);

private:
  vtkSMDataTypeDomain(const vtkSMDataTypeDomain&) = delete;
  void operator=(const vtkSMDataTypeDomain&) = delete;
};

#endif
