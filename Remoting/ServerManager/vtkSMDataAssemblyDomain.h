// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMDataAssemblyDomain
 * @brief a domain that uses vtkDataAssembly
 *
 * vtkSMDataAssemblyDomain is intended for vtkSMStringVectorProperty that uses
 * vtkDataAssembly to qualify values. For example, the "Paths" property on
 * `vtkExtractBlockUsingDataAssembly` filter that lets the user select paths to
 * extract uses this domain.
 *
 * Example usage in a ServerManager XML is as follows:
 *
 * @code{xml}
 * <SourceProxy ...>
 * ...
 *   <StringVectorProperty clean_command="ClearNodePaths"
 *                      command="AddNodePath"
 *                      name="Paths"
 *                      number_of_elements_per_command="1"
 *                      panel_visibility="default"
 *                      repeat_command="1"
 *                      animateable="0">
 *     <DataAssemblyDomain name="data_assembly" (optional)entity_type=3
 *       (optional)mode="leaves" (optional)default_mode="nonempty-leaf">
 *       <RequiredProperties>
 *         <Property function="Input" name="Input" />
 *       </RequiredProperties>
 *     </DataAssemblyDomain>
 *     <Documentation>
 *       This property lists the paths for blocks to extract.
 *     </Documentation>
 *   </StringVectorProperty>
 * </SourceProxy>
 * @endcode
 *
 * The `entity_type` attribute is optional and can be used to specify the
 * default IOSS selector (based on vtkIOSSReader::EntityTypes) to use when
 * building the data assembly. If not specified, the default is nothing.
 *
 * The `mode` attribute is optional and can be used to specify the type of
 * selectors that the property can have. The default is `all`, which means
 * the property can have selectors for all nodes in the data assembly.
 * The `leaves` mode can be used to restrict the property to only have
 * selectors for leaf nodes in the data assembly.
 *
 * The `default_mode` attribute is optional and can be used to specify how
 * the default value for the property is set by `SetDefaultValues()`.
 * The default is `DEFAULT`, which implies the default value is what the user has provided
 * NONEMPTY_LEAF` indicates that the first non-empty leaf node is set as the
 * default value, if possible. Setting the `mode` to `leaves` also implies
 * `default_mode="nonempty-leaf"`.
 *
 * vtkSMDataAssemblyDomain can also be used on readers. In that case, it uses
 * vtkPVDataAssemblyInformation to obtain the data assembly from the reader. The
 * following snippet shows how this domain can be used on a reader. Note, the
 * 'Tag' required property is essential to use this domain on a reader. It is
 * used to determine when to fetch the vtkDataAssembly from the reader.
 *
 * @code{xml}
 *
 * <SourceProxy ...>
 * ...
 *   <IntVectorProperty name="AssemblyTag"
 *     command="GetAssemblyTag"
 *     information_only="1">
 *     <Documentation>
 *       This is simply an int that changes whenever a new assembly is built
 *       by the reader. This can be used to determine if the assembly should be fetched from
 *       the reader whenever the reader is updated.
 *     </Documentation>
 *   </IntVectorProperty>
 *
 *   <StringVectorProperty name="AssemmblySelectors"
 *     command="AddSelector"
 *     clean_command="ClearSelectors"
 *     repeat_command="1"
 *     number_of_elements_per_command="1"
 *     panel_widget="data_assembly_editor" >
 *     <DataAssemblyDomain name="data_assembly">
 *       <RequiredProperties>
 *         <Property function="Tag" name="AssemblyTag" />
 *       </RequiredProperties>
 *     </DataAssemblyDomain>
 *     <Documentation>
 *       Specify the selectors for the data assembly chosen using **Assembly**
 *       to choose the blocks to extract from the input dataset.
 *     </Documentation>
 *     <Hints>
 *       <!-- AssemblyTag == 0 implies there's no assembly in the file,
 *       in which case, we want to hide this widget entirely -->
 *       <PropertyWidgetDecorator type="GenericDecorator"
 *         mode="visibility"
 *         property="AssemblyTag"
 *         value="0"
 *         inverse="1" />
 *     </Hints>
 *   </StringVectorProperty>
 *
 * </SourceProxy>
 * @endcode
 */

#ifndef vtkSMDataAssemblyDomain_h
#define vtkSMDataAssemblyDomain_h

#include "vtkRemotingServerManagerModule.h" // needed for exports
#include "vtkSMDomain.h"
#include "vtkSmartPointer.h" //  needed for vtkSmartPointer

#include <string> // for std::string

class vtkDataAssembly;
class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDataAssemblyDomain : public vtkSMDomain
{
public:
  static vtkSMDataAssemblyDomain* New();
  vtkTypeMacro(vtkSMDataAssemblyDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the name for the chosen assembly, if any.
   */
  const char* GetDataAssemblyName() const
  {
    return this->Name.empty() ? nullptr : this->Name.c_str();
  }

  /**
   * Provides access to the data assembly.
   */
  vtkDataAssembly* GetDataAssembly() const;

  void Update(vtkSMProperty* requestingProperty) override;

  ///@{
  /**
   * Mode indicates if the property is interested in all nodes, leaves only or
   * non-leaves only. Can be configured in XML using the "mode" attribute.
   * Values can be "all", "leaves", "non-leaves". Default is all nodes.
   */
  vtkGetMacro(Mode, int);
  vtkSetMacro(Mode, int);
  ///@}

  enum
  {
    ALL = 0,
    LEAVES = 1,
  };

  enum DefaultModes
  {
    DEFAULT = 0,
    NONEMPTY_LEAF = 1
  };

  ///@{
  /**
   * DefaultMode controls how the default value for the property is set by
   * SetDefaultValues(). DEFAULT implies the default value is picked based on
   * the default strategy for the selected Mode. NONEMPTY_LEAF indicates that
   * the first non-empty leaf node is set as the default value, if possible.
   */
  vtkGetMacro(DefaultMode, int);
  vtkSetMacro(DefaultMode, int);
  ///@}

  int SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values) override;

protected:
  vtkSMDataAssemblyDomain();
  ~vtkSMDataAssemblyDomain() override;

private:
  vtkSMDataAssemblyDomain(const vtkSMDataAssemblyDomain&) = delete;
  void operator=(const vtkSMDataAssemblyDomain&) = delete;

  void ChooseAssembly(const std::string& name, vtkDataAssembly* assembly);
  void FetchAssembly(int tag);

  /**
   * entity_type is an optional attribute that can be used to specify the default
   * IOSS selector (based on vtkIOSSReader::EntityTypes) to use when
   * building the data assembly. If not specified, the default is nothing.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  int LastTag = 0;

  vtkSmartPointer<vtkDataAssembly> Assembly;
  std::string AssemblyXMLContents;
  std::string Name;
  int EntityType = -1;
  int Mode = ALL;
  int DefaultMode = DEFAULT;
};

#endif
