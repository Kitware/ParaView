/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeTreeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMCompositeTreeDomain
 * @brief   domain used to restrict an
 * vtkSMIntVectorProperty values to valid \c flat-index for a
 * vtkCompositeDataSet.
 *
 * vtkSMCompositeTreeDomain can be added to a vtkSMIntVectorProperty. This
 * domain requires a vtkSMInputProperty which is used to provide the input to
 * the filter. This domain obtains data information from the input selected on
 * the required input property and then decides the range for values the
 * property can have.
 *
 * Broadly speaking, there are two ways of identifying unique node in a
 * composite dataset: `flat-index` (also called `composite-index`) and
 * `level-block-index`. `flat-index` applies to all types of composite
 * datasets while `level-block-index` (or just `level-index`) applies only to AMR
 * datasets. `flat-index` for any node in an arbitrary composite-dataset
 * is simply the index of that node in a pre-order traversal of the tree with
 * the root composite-dataset getting the index 0. `level-index` for an AMR
 * dataset is the AMR level number while `level-block-index` is a pair of
 * the AMR level number and block number for the node in that level.
 *
 * The type of index the property expects, is defined by the domain's mode.
 * Supported modes are:
 *  -# vtkSMCompositeTreeDomain::ALL: (default) \n
 *     The property uses `flat-index` and can accept index for any node (leaf or non-leaf).
 *     This can be specified in XML using the `mode="all"`.
 *
 *  -# vtkSMCompositeTreeDomain::LEAVES:\n
 *     The property uses `flat-index` however can only accept flat-indices for
 *     leaf-nodes.
 *     This can be specified in XML using the `mode="leaves"`.
 *
 *  -# vtkSMCompositeTreeDomain::AMR: \n
 *     The property uses `level-index` i.e. AMR level number or
 *     `level-block-index`. If the property has 2 elements (or for repeatable
 *     properties, if number of elements per command is 2) then
 *     `level-block-index` is used, otherwise simply the `level-index` is used.
 *     This only makes sense for filters dealing with AMR datasets.
 *     This can be specified in XML using the `mode="amr"`.
 *
 *  -# vtkSMCompositeTreeDomain::NON_LEAVES: (deprecated)\n
 *     No longer supported (as of ParaView 5.4) and simply interpreted as
 *     vtkSMCompositeTreeDomain::ALL.
 *     This used to be specified in XML using the `mode="non-leaves"`.
 *
 * vtkSMCompositeTreeDomain also provides ability to set default value on the
 * property. If mode is LEAVES, then the default value selected is the first
 * non-null leaf node. If mode is ALL, the same behaviour for default value is
 * possible by using `default_mode="nonempty-leaf"` in XML.
 * e.g.
 * \code{.xml}
 *   <CompositeTreeDomain name="tree" mode="all" default_mode="nonempty-leaf">
 *     <RequiredProperties>
 *       <Property function="Input" name="Input" />
 *     </RequiredProperties>
 *   </CompositeTreeDomain>
 * \endcode
*/

#ifndef vtkSMCompositeTreeDomain_h
#define vtkSMCompositeTreeDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkPVDataInformation;
class vtkSMInputProperty;
class vtkSMSourceProxy;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMCompositeTreeDomain : public vtkSMDomain
{
public:
  static vtkSMCompositeTreeDomain* New();
  vtkTypeMacro(vtkSMCompositeTreeDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called when the 'required-property' is modified. The property must be a
   * vtkSMInputProperty. This will obtain the composite data information for the
   * input source and then determine the valid range for the flat-index.
   */
  void Update(vtkSMProperty* input) override;

  //@{
  /**
   * Get the vtkPVDataInformation which provides the tree structure for the
   * composite dataset.
   */
  vtkGetObjectMacro(Information, vtkPVDataInformation);
  //@}

  /**
   * Returns the source proxy whose data information is returned by
   * GetInformation().
   */
  vtkSMSourceProxy* GetSource();

  //@{
  /**
   * Returns the port for the source proxy from which the data information is
   * obtained by GetInformation().
   */
  vtkGetMacro(SourcePort, int);
  //@}

  /**
   * Is the (unchecked) value of the property in the domain? Overwritten by
   * sub-classes.
   */
  int IsInDomain(vtkSMProperty* vtkNotUsed(property)) override { return 1; }

  //@{
  /**
   * Mode indicates if the property is interested in all nodes, leaves only or
   * non-leaves only. Can be configured in XML using the "mode" attribute.
   * Values can be "all", "leaves", "non-leaves". Default is all nodes.
   */
  vtkGetMacro(Mode, int);
  vtkSetMacro(Mode, int);
  //@}

  enum
  {
    ALL = 0,
    LEAVES = 1,
    NON_LEAVES = 2,
    NONE = 3,
    AMR = 4,
  };

  enum DefaultModes
  {
    DEFAULT = 0,
    NONEMPTY_LEAF = 1
  };

  //@{
  /**
   * DefaultMode controls how the default value for the property is set by
   * SetDefaultValues(). DEFAULT implies the default value is picked based on
   * the default strategy for the selected Mode. NONEMPTY_LEAF indicates that
   * the first non-empty leaf node is set as the default value, if possible.
   */
  vtkGetMacro(DefaultMode, int);
  vtkSetMacro(DefaultMode, int);
  //@}

  /**
   * A vtkSMProperty is often defined with a default value in the
   * XML itself. However, many times, the default value must be determined
   * at run time. To facilitate this, domains can override this method
   * to compute and set the default value for the property.
   * Note that unlike the compile-time default values, the
   * application must explicitly call this method to initialize the
   * property.
   * Returns 1 if the domain updated the property.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

protected:
  vtkSMCompositeTreeDomain();
  ~vtkSMCompositeTreeDomain() override;

  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  /**
   * Internal implementation called by Update(vtkSMProperty*);
   */
  void Update(vtkSMInputProperty* iproperty);

  void InvokeModifiedIfChanged();

  void SetInformation(vtkPVDataInformation*);
  vtkPVDataInformation* Information;

  vtkTimeStamp UpdateTime;
  vtkPVDataInformation* LastInformation; // not reference counted.

  vtkWeakPointer<vtkSMSourceProxy> Source;
  int Mode;
  int DefaultMode;
  int SourcePort;

private:
  vtkSMCompositeTreeDomain(const vtkSMCompositeTreeDomain&) = delete;
  void operator=(const vtkSMCompositeTreeDomain&) = delete;
};

#endif
