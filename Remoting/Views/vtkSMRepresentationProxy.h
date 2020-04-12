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
 * @class vtkSMRepresentationProxy
 * @brief Proxy for a representations
 *
 * vtkSMRepresentationProxy is a vtkSMSourceProxy subclass that is designed to
 * be used either directly or as a superclass for "data" representations i.e.
 * representation that have an input (as against proxies for annotations such as 3D widgets).
 *
 * @section RepresentationMarkDirty Special handling of `MarkDirty`
 *
 * `vtkSMProxy::MarkDirty` is a mechanism that ParaView uses to know on the
 * client side (i.e. in the server-manager layer) when a pipeline on the server side
 * is "dirty" or has potential to execute on an update causing things like data
 * information to be invalidated. Simply speaking, when a property on a
 * vtkSMProxy is modified and pushed (using `vtkSMProxy::UpdateVTKObjects` or
 * `vtkSMProxy::UpdateProperty`), it calls `this->MarkDirty(this)`.
 * `vtkSMProxy::MarkDirty` invokes `vtkSMProxy::MarkConsumersAsDirty` which results
 * in any consumers of this dirtied proxy also getting the notification.
 * Consumers (and conversely producers) are setup via ProxyProperty and
 * InputProperty connections. For standard data processing pipelines, this
 * mechanism works quite well. Consider a filter proxy, say `Clip`, which has an input
 * set via a InputProperty and a implicit function set via a ProxyProperty. If
 * either the input or the implicit function is modified, it is reasonable to
 * expect the Clip filter to re-execute on an `UpdatePipeline` call. Whether the
 * call will truly cause the VTK pipeline (server side) to
 * execute is not that important. The pipeline may not re-execute and that's not
 * a big deal.
 *
 * Things get a little complicated for representations, however.
 * Data representations don't necessary have valid input connections on
 * all processes where their VTK objects are present (note, representations create
 * VTK objects on all process while data input is only available on the data
 * server nodes). As a result representations have to be explicitly told to
 * re-execute following upstream changes since they cannot rely on their
 * VTK-level upstream. Re-executing a representation means regenerating the
 * geometry or data artifacts for rendering, redelivering them to the rendering
 * nodes, etc. It can also mean cleaning up any caches the representation built for
 * flip-book animation support. In other words, re-executing a representation is
 * non-trivial task. Thus, we want do it only when absolutely needed.
 *
 * The explicit pipeline update is often referred
 * to as "update suppression" and handled by `vtkPVDataRepresentation` and
 * `vtkPVDataRepresentationPipeline`. In short, calling `Update` on
 * vtkPVDataRepresentation has no effect unless
 * `vtkPVDataRepresentation::MarkModified` was explicitly called at some point
 * since the last `Update`.
 *
 * To explicitly call `vtkPVDataRepresentation::MarkModified` for only those
 * cases where the representation must be re-executed, we follow the following
 * strategy.
 *
 * \li vtkPVDataRepresentation subclasses explicitly call `this->MarkModified`
 *     when a public API that invalidates the data pipeline is invoked.
 * \li vtkSMRepresentationProxy overrides `vtkSMProxy::MarkDirtyFromProducer`
 *     to call `MarkModified` on the VTK object if the producer was connected to
 *     the representation via a InputProperty and not a ProxyProperty. The
 *     assumption is that InputProperty connections typically imply pipeline
 *     connections while ProxyProperty connections are other supporting VTK
 *     objects e.g. LUT.
 */

#ifndef vtkSMRepresentationProxy_h
#define vtkSMRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMSourceProxy.h"

class vtkPVProminentValuesInformation;
namespace vtkPVComparativeViewNS
{
class vtkCloningVectorOfRepresentations;
}

class VTKREMOTINGVIEWS_EXPORT vtkSMRepresentationProxy : public vtkSMSourceProxy
{
public:
  static vtkSMRepresentationProxy* New();
  vtkTypeMacro(vtkSMRepresentationProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  virtual vtkPVProminentValuesInformation* GetProminentValuesInformation(std::string name,
    int fieldAssoc, int numComponents, double uncertaintyAllowed = 1e-6, double fraction = 1e-3,
    bool force = false);

  /**
   * Calls Update() on all sources. It also creates output ports if
   * they are not already created.
   */
  void UpdatePipeline() override;

  /**
   * Calls Update() on all sources with the given time request.
   * It also creates output ports if they are not already created.
   */
  void UpdatePipeline(double time) override;

  /**
   * Overridden to reset this->MarkedModified flag.
   */
  void PostUpdateData(bool) override;

  /**
   * Called after the view updates.
   */
  virtual void ViewUpdated(vtkSMProxy* view);

  /**
   * Overridden to reserve additional IDs for use by internal composite representation
   */
  vtkTypeUInt32 GetGlobalID() override;

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

  //@{
  /**
   * @deprecated in ParaView 5.7. Use
   * vtkSMProxy::SetLogName/vtkSMProxy::GetLogName instead.
   */
  VTK_LEGACY(void SetDebugName(const char* name));
  VTK_LEGACY(const char* GetDebugName());
  //@}

  void MarkDirty(vtkSMProxy* modifiedProxy) override;

protected:
  vtkSMRepresentationProxy();
  ~vtkSMRepresentationProxy() override;

  /**
   * When the representation is being marked dirty due to a producer i.e. a
   * proxy-property input-property connection, we need to decide if the
   * representation's data pipeline is indeed invalidated or is this simply a
   * "rendering" change. We override this method to make that distinction.
   */
  void MarkDirtyFromProducer(
    vtkSMProxy* modifiedProxy, vtkSMProxy* producer, vtkSMProperty* property) override;

  // Unlike subproxies in regular proxies, subproxies in representations
  // typically represent internal representations e.g. label representation,
  // representation for selection etc. In that case, if the internal
  // representation is modified, we need to ensure that any of our consumers is
  // a consumer of all our subproxies as well.
  void AddConsumer(vtkSMProperty* property, vtkSMProxy* proxy) override;
  void RemoveConsumer(vtkSMProperty* property, vtkSMProxy* proxy) override;
  void RemoveAllConsumers() override;

  void CreateVTKObjects() override;
  void OnVTKRepresentationUpdated();
  void OnVTKRepresentationUpdateSkipped();
  void OnVTKRepresentationUpdateTimeChanged();

  virtual void UpdatePipelineInternal(double time, bool doTime);

  /**
   * Mark the data information as invalid.
   */
  void InvalidateDataInformation() override;

  /**
   * Overridden to restore this->Servers flag state.
   */
  int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator) override;

private:
  vtkSMRepresentationProxy(const vtkSMRepresentationProxy&) = delete;
  void operator=(const vtkSMRepresentationProxy&) = delete;

  bool RepresentedDataInformationValid;
  vtkPVDataInformation* RepresentedDataInformation;

  bool ProminentValuesInformationValid;
  vtkPVProminentValuesInformation* ProminentValuesInformation;
  double ProminentValuesFraction;
  double ProminentValuesUncertainty;

  friend class vtkPVComparativeViewNS::vtkCloningVectorOfRepresentations;
  void ClearMarkedModified() { this->MarkedModified = false; }
  bool MarkedModified;
  bool VTKRepresentationUpdated;
  bool VTKRepresentationUpdateSkipped;
  bool VTKRepresentationUpdateTimeChanged;

  std::string DebugName;
};

#endif
