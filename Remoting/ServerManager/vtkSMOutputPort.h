/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOutputPort.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMOutputPort
 * @brief   reference for an output port of a vtkAlgorithm.
 *
 * This object manages one output port of an algorithm. It is used
 * internally by vtkSMSourceProxy to manage all of it's outputs.
 * @attention
 * Previously, vtkSMOutputPort used to be a vtkSMProxy subclass since it was
 * indeed a Proxy for the output port. However that has now changed. This merely
 * sits as a datastructure to manage ports specific things like
 * data-information. However for backwards compatibility, to keep the impact
 * minimal, we leave this as a sub-class of a Proxy with GlobalID=0 and
 * Session=nullptr.
*/

#ifndef vtkSMOutputPort_h
#define vtkSMOutputPort_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer
#include "vtkWeakPointer.h"  // needed for vtkWeakPointer

#include <map> // needed for std::map

class vtkCollection;
class vtkPVClassNameInformation;
class vtkPVDataAssemblyInformation;
class vtkPVDataInformation;
class vtkPVTemporalDataInformation;
class vtkSMCompoundSourceProxy;
class vtkSMSourceProxy;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMOutputPort : public vtkSMProxy
{
public:
  static vtkSMOutputPort* New();
  vtkTypeMacro(vtkSMOutputPort, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns data information. If data information is marked
   * invalid, calls GatherDataInformation.
   * If data information is gathered then this fires the
   * vtkCommand::UpdateInformationEvent event.
   */
  virtual vtkPVDataInformation* GetDataInformation();

  //@{
  /**
   * For composite datasets, `GetDataInformation` returns summary data information for
   * all blocks combined. However, applications may require information about
   * specific subset of blocks. In that case, one can use this API. Internally,
   * the data information is cached per selector / assembly-name pair. That way,
   * if the data information is not re-gathered unless changed.
   *
   * @arg \c selector the selector expression
   * @arg \c assemblyName name of the assembly to use to apply the selector
   *         to to determine the subset. If none specified, hierarchy is assumed.
   */
  vtkPVDataInformation* GetSubsetDataInformation(
    const char* selector, const char* assemblyName = nullptr);
  //@}

  /**
   * A `GetSubsetDataInformation` overload that uses composite index. It is only
   * supported for multiblock datasets.
   */
  vtkPVDataInformation* GetSubsetDataInformation(unsigned int compositeIndex);

  /**
   * Returns data information collected over all timesteps provided by the
   * pipeline. If the data information is not valid, this results iterating over
   * the pipeline and hence can be slow. Use with caution.
   */
  virtual vtkPVTemporalDataInformation* GetTemporalDataInformation();

  /**
   * Returns the classname of the data object on this output port.
   */
  virtual const char* GetDataClassName();

  /**
   * Returns classname information.
   */
  virtual vtkPVClassNameInformation* GetClassNameInformation();

  /**
   * Mark data information as invalid.
   */
  virtual void InvalidateDataInformation();

  //@{
  /**
   * Returns the index of the port the output is obtained from.
   */
  vtkGetMacro(PortIndex, int);
  //@}

  /**
   * Provides access to the source proxy to which the output port belongs.
   */
  vtkSMSourceProxy* GetSourceProxy();

  //@{
  /** Overridden to forward to the source proxy.
   */
  vtkSMSession* GetSession() override;
  vtkSMSessionProxyManager* GetSessionProxyManager() override;
  //@}
protected:
  vtkSMOutputPort();
  ~vtkSMOutputPort() override;

  /**
   * Get the classname of the dataset from server.
   */
  virtual void GatherClassNameInformation();

  /**
   * Get information about dataset from server.
   * Fires the vtkCommand::UpdateInformationEvent event.
   */
  virtual void GatherDataInformation();

  /**
   * Get temporal information from the server.
   */
  virtual void GatherTemporalDataInformation();

  void SetSourceProxy(vtkSMSourceProxy* src);

  // When set to non-nullptr, GetSourceProxy() returns this rather than the real
  // source-proxy set using SetSourceProxy(). This provides a mechanism for
  // vtkSMCompoundSourceProxy to take ownership of ports that don't really
  // belong to it.
  void SetCompoundSourceProxy(vtkSMCompoundSourceProxy* src);

  /**
   * An internal update pipeline method that subclasses may override.
   */
  virtual void UpdatePipelineInternal(double time, bool doTime);

  // The index of the port the output is obtained from.
  vtkSetMacro(PortIndex, int);
  int PortIndex;

  vtkWeakPointer<vtkSMSourceProxy> SourceProxy;
  vtkWeakPointer<vtkSMCompoundSourceProxy> CompoundSourceProxy;

  vtkPVClassNameInformation* ClassNameInformation;
  int ClassNameInformationValid;

  vtkPVDataInformation* DataInformation;
  bool DataInformationValid;

  vtkPVTemporalDataInformation* TemporalDataInformation;
  bool TemporalDataInformationValid;

  std::map<std::string, std::map<int, vtkSmartPointer<vtkPVDataInformation> > >
    SubsetDataInformations;

private:
  vtkSMOutputPort(const vtkSMOutputPort&) = delete;
  void operator=(const vtkSMOutputPort&) = delete;

  friend class vtkSMSourceProxy;
  friend class vtkSMCompoundSourceProxy;
  void UpdatePipeline();

  // Update Pipeline with the given timestep request.
  void UpdatePipeline(double time);
};

#endif
