/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSILModel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMSILModel
 * @brief   is a helper for to work with SILs.
 *
 * vtkSMSILModel makes it easier to make checks/unchecks for the SIL while
 * respecting the links/dependencies defined by the SIL.
 *
 * There are two ways of initializing the model:
 * \li One way is to initialize it with a SIL (using Initialize(vtkGraph*).
 * Then the model can be used as a simple API to check/uncheck elements.
 * \li Second way is to initialize with a proxy and property (using
 * Initialize(vtkSMProxy, vtkSMProperty*). In that case, the SIL is obtained
 * from  the property's vtkSMSILDomain. Also as the user changes the check
 * states, the property is updated/pushed.
 *
 * @par Events:
 * \li vtkCommand::UpdateDataEvent -- fired when the check state of any element
 * changes. calldata = vertexid for the element whose check state changed.
 *
 * @section vtkSMSILModelLegacyWarning Legacy Warning
 *
 * While not deprecated, this class exists to support readers that use legacy
 * representation for SIL which used a `vtkGraph` to represent the SIL. It is
 * recommended that newer code uses vtkSubsetInclusionLattice (or subclass) to
 * represent the SIL. In that case, there is no need for such a helper class as
 * similar API is exposed on vtkSubsetInclusionLattice and subclasses.
 *
 */

#ifndef vtkSMSILModel_h
#define vtkSMSILModel_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"
#include <set> // required for vtkset

class vtkGraph;
class vtkSMStringVectorProperty;
class vtkSMProxy;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMSILModel : public vtkSMObject
{
public:
  static vtkSMSILModel* New();
  vtkTypeMacro(vtkSMSILModel, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum CheckState
  {
    UNCHECKED = 0,
    PARTIAL = 1,
    CHECKED = 2
  };

  //@{
  /**
   * Initialize the model using a SIL.
   * There are two ways of initializing the model:
   * \li One way is to initialize it with a SIL (using Initialize(vtkGraph*).
   * Then the model can be used as a simple API to check/uncheck elements.
   * \li Second way is to initialize with a proxy and property (using
   * Initialize(vtkSMProxy, vtkSMProperty*). In that case, the SIL is obtained
   * from  the property's vtkSMSILDomain. Also as the user changes the check
   * states, the property is updated/pushed.
   */
  void Initialize(vtkGraph* sil);
  vtkGetObjectMacro(SIL, vtkGraph);
  //@}

  //@{
  /**
   * Initialize the model using a proxy and its property.
   * If a property is set, then the model keeps the property updated when the
   * check states are changed or when the property changes, the model's internal
   * check states are updated. If the property has a SILDomain, then the model
   * attaches itself to the domain so that whenever the domains is updated (i.e.
   * a new SIL is obtained from the server) the model updates the sil as well.

   * There are two ways of initializing the model:
   * \li One way is to initialize it with a SIL (using Initialize(vtkGraph*).
   * Then the model can be used as a simple API to check/uncheck elements.
   * \li Second way is to initialize with a proxy and property (using
   * Initialize(vtkSMProxy, vtkSMProperty*). In that case, the SIL is obtained
   * from  the property's vtkSMSILDomain. Also as the user changes the check
   * states, the property is updated/pushed.
   */
  void Initialize(vtkSMProxy*, vtkSMStringVectorProperty*);
  vtkGetObjectMacro(Proxy, vtkSMProxy);
  vtkGetObjectMacro(Property, vtkSMStringVectorProperty);
  //@}

  /**
   * Returns the number of children for the given vertex.
   * A node is a child node if it has no out-going edges or all out-going edges
   * have "CrossEdges" set to 1. If vertex id is invalid, returns -1.
   */
  int GetNumberOfChildren(vtkIdType vertexid);

  /**
   * Returns the vertex id for the n-th child where n=child_index. Returns 0 if
   * request is invalid.
   */
  vtkIdType GetChildVertex(vtkIdType parentid, int child_index);

  /**
   * Returns the parent vertex i.e. the vertex at the end of an in-edge which is
   * not a cross-edge. It's an error to call this method for the root vertex id
   * i.e. 0.
   */
  vtkIdType GetParentVertex(vtkIdType parent);

  /**
   * Get the name for the vertex.
   */
  const char* GetName(vtkIdType vertex);

  /**
   * Get the check state for a vertex.
   */
  int GetCheckStatus(vtkIdType vertex);

  //@{
  /**
   * Set the check state for a vertex.
   * Returns true if the status was changed, false if unaffected.
   */
  bool SetCheckState(vtkIdType vertex, int status);
  bool SetCheckState(const char* name, int status)
  {
    vtkIdType vertex = this->FindVertex(name);
    if (vertex != -1)
    {
      return this->SetCheckState(vertex, status);
    }
    vtkErrorMacro("Failed to locate " << name);
    return false;
  }
  //@}

  //@{
  /**
   * Convenience methods to check/uncheck all items.
   */
  void CheckAll();
  void UncheckAll();
  //@}

  /**
   * Updates the property using the check states maintained by the model.
   */
  void UpdatePropertyValue(vtkSMStringVectorProperty*);

  /**
   * Updates the check states maintained internally by the model using the
   * status from the property.
   */
  void UpdateStateFromProperty(vtkSMStringVectorProperty*);

  /**
   * Locate a vertex with the given name. Returns -1 if the vertex is not found.
   */
  vtkIdType FindVertex(const char* name);

  void GetLeaves(std::set<vtkIdType>& leaves, vtkIdType root, bool traverse_cross_edges);

protected:
  vtkSMSILModel();
  ~vtkSMSILModel() override;

  void UpdateProperty();
  void OnPropertyModified();
  void OnDomainModified();

  /// Called to check/uncheck an item.
  void Check(vtkIdType vertexid, bool checked, vtkIdType inedgeid = -1);

  /// Determine vertexid's check state using its immediate children.
  /// If the check-state for the vertex has changed, then it propagates the call
  /// to the parent node.
  void UpdateCheck(vtkIdType vertexid);

  bool BlockUpdate;

  void SetSIL(vtkGraph*);

  vtkSMProxy* Proxy;
  vtkSMStringVectorProperty* Property;
  vtkGraph* SIL;
  vtkCommand* PropertyObserver;
  vtkCommand* DomainObserver;

private:
  vtkSMSILModel(const vtkSMSILModel&) = delete;
  void operator=(const vtkSMSILModel&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
