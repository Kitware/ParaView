/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMRepresentationProxy
 *
 *
*/

#ifndef vtkSMRepresentationProxy_h
#define vtkSMRepresentationProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMSourceProxy.h"

class vtkPVProminentValuesInformation;
namespace vtkPVComparativeViewNS
{
class vtkCloningVectorOfRepresentations;
}

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMRepresentationProxy : public vtkSMSourceProxy
{
public:
  static vtkSMRepresentationProxy* New();
  vtkTypeMacro(vtkSMRepresentationProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Calls MarkDirty() and invokes ModifiedEvent.
   */
  virtual void MarkDirty(vtkSMProxy* modifiedProxy) VTK_OVERRIDE;

  /**
   * Returns information about the data that is finally rendered by this
   * representation.
   */
  virtual vtkPVDataInformation* GetRepresentedDataInformation();

  /**
   * Returns information about a specific array component's prominent values (or NULL).

   * The \a name, \a fieldAssoc, and \a numComponents arguments specify
   * which arrays on the input dataset to examine. Because multiblock
   * datasets may have multiple arrays of the same name on different blocks,
   * and these arrays may not have the same storage type or number of
   * components, this method requires you to specify the number of
   * components per tuple the array(s) of interest must have.
   * You may call GetRepresentedDataInformation() to obtain the number of
   * components for any array.

   * See vtkAbstractArray::GetProminentComponentValues for more information
   * about the \a uncertaintyAllowed and \a fraction arguments.
   */
  virtual vtkPVProminentValuesInformation* GetProminentValuesInformation(vtkStdString name,
    int fieldAssoc, int numComponents, double uncertaintyAllowed = 1e-6, double fraction = 1e-3);

  /**
   * Calls Update() on all sources. It also creates output ports if
   * they are not already created.
   */
  virtual void UpdatePipeline() VTK_OVERRIDE;

  /**
   * Calls Update() on all sources with the given time request.
   * It also creates output ports if they are not already created.
   */
  virtual void UpdatePipeline(double time) VTK_OVERRIDE;

  /**
   * Overridden to reset this->MarkedModified flag.
   */
  virtual void PostUpdateData() VTK_OVERRIDE;

  /**
   * Called after the view updates.
   */
  virtual void ViewUpdated(vtkSMProxy* view);

  /**
   * Overridden to reserve additional IDs for use by internal composite representation
   */
  virtual vtkTypeUInt32 GetGlobalID() VTK_OVERRIDE;

  //@{
  /**
   * Set the representation type. Default implementation simply updates the
   * "Representation" property, if present with the value provided. Subclasses
   * can override this method to add custom logic to manage the representation
   * state to support the change e.g. pick a scalar color array when switching
   * to Volume or Slice representation, for example. Returns true, if the change
   * was successful, otherwise returns false.
   */
  virtual bool SetRepresentationType(const char* type);
  static bool SetRepresentationType(vtkSMProxy* repr, const char* type)
  {
    vtkSMRepresentationProxy* self = vtkSMRepresentationProxy::SafeDownCast(repr);
    return self ? self->SetRepresentationType(type) : false;
  }
  //@}

protected:
  vtkSMRepresentationProxy();
  ~vtkSMRepresentationProxy();

  // Unlike subproxies in regular proxies, subproxies in representations
  // typically represent internal representations e.g. label representation,
  // representation for selection etc. In that case, if the internal
  // representation is modified, we need to ensure that any of our consumers is
  // a consumer of all our subproxies as well.
  virtual void AddConsumer(vtkSMProperty* property, vtkSMProxy* proxy) VTK_OVERRIDE;
  virtual void RemoveConsumer(vtkSMProperty* property, vtkSMProxy* proxy) VTK_OVERRIDE;
  virtual void RemoveAllConsumers() VTK_OVERRIDE;

  virtual void CreateVTKObjects() VTK_OVERRIDE;
  void OnVTKRepresentationUpdated();

  virtual void UpdatePipelineInternal(double time, bool doTime);

  /**
   * Mark the data information as invalid.
   */
  virtual void InvalidateDataInformation() VTK_OVERRIDE;

  /**
   * Overridden to restore this->Servers flag state.
   */
  virtual int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator) VTK_OVERRIDE;

private:
  vtkSMRepresentationProxy(const vtkSMRepresentationProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMRepresentationProxy&) VTK_DELETE_FUNCTION;

  /**
   * HACK: Returns true for lookuptable, piecewise function proxies which are
   * not expected to modify data pipeline.
   */
  bool SkipDependency(vtkSMProxy* producer);

  bool RepresentedDataInformationValid;
  vtkPVDataInformation* RepresentedDataInformation;

  bool ProminentValuesInformationValid;
  vtkPVProminentValuesInformation* ProminentValuesInformation;
  double ProminentValuesFraction;
  double ProminentValuesUncertainty;

  //@{
  /**
   * When ViewTime changes, we mark all inputs modified so that they fetch the
   * updated data information.
   */
  void ViewTimeChanged();
  friend class vtkSMViewProxy;
  //@}

  friend class vtkPVComparativeViewNS::vtkCloningVectorOfRepresentations;
  void ClearMarkedModified() { this->MarkedModified = false; }
  bool MarkedModified;
  bool VTKRepresentationUpdated;
};

#endif
