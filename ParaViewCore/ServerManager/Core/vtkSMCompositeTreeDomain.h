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
 * the required input property and then decides the range for the flat-index. A
 * flat index for a tree is obtained by performing a pre-order traversal of the
 * tree eg. A ( B ( D, E), C (F, G)) becomes: [A,B,D,E,C,F,G], so flat-index of A is
 * 0, while flat-index of C is 4.
 *
 * vtkSMCompositeTreeDomain can be used in multiple modes.
 * \li ALL : This mode is used if the property can accept any type of node index.
 *           To select this mode in XML, use the `mode="all"`.
 * \li LEAVES: This mode is used if the property can only accept leaf nodes i.e.
 *             indices for non-composite datasets. This is specified in XML
 *             using `mode="leaves"`.
 * \li NON_LEAVES: This mode is used if the property can only accept non-leaf
 *                 node indices, specified using `mode="non-leaves"` in XML
 *                 configuration.
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

// TODO: CHANGE NAME OF THIS CLASS
class VTKPVSERVERMANAGERCORE_EXPORT vtkSMCompositeTreeDomain : public vtkSMDomain
{
public:
  static vtkSMCompositeTreeDomain* New();
  vtkTypeMacro(vtkSMCompositeTreeDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Called when the 'required-property' is modified. The property must be a
   * vtkSMInputProperty. This will obtain the composite data information for the
   * input source and then determine the valid range for the flat-index.
   */
  virtual void Update(vtkSMProperty* input);

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
  virtual int IsInDomain(vtkSMProperty* vtkNotUsed(property)) { return 1; }

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
    NONE = 3
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
  virtual int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values);

protected:
  vtkSMCompositeTreeDomain();
  ~vtkSMCompositeTreeDomain();

  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

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
  vtkSMCompositeTreeDomain(const vtkSMCompositeTreeDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMCompositeTreeDomain&) VTK_DELETE_FUNCTION;
};

#endif
