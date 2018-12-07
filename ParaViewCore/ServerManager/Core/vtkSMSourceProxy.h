/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMSourceProxy
 * @brief   proxy for a VTK source on a server
 *
 * vtkSMSourceProxy manages VTK source(s) that are created on a server
 * using the proxy pattern. In addition to functionality provided
 * by vtkSMProxy, vtkSMSourceProxy provides method to connect and
 * update sources. Each source proxy has one or more output ports
 * (vtkSMOutputPort).
 * Each port represents one output of one filter. These are created
 * automatically (when CreateOutputPorts() is called) by the source.
 * Each vtkSMSourceProxy creates a property called DataInformation.
 * This property is a composite property that provides information
 * about the output(s) of the VTK sources (obtained from the server)
 * @sa
 * vtkSMProxy vtkSMOutputPort vtkSMInputProperty
*/

#ifndef vtkSMSourceProxy_h
#define vtkSMSourceProxy_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkPVArrayInformation;
class vtkPVDataInformation;
class vtkPVDataSetAttributesInformation;

struct vtkSMSourceProxyInternals;

class vtkSMOutputPort;
class vtkSMProperty;
class vtkSMSessionProxyManager;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMSourceProxy : public vtkSMProxy
{
public:
  static vtkSMSourceProxy* New();
  vtkTypeMacro(vtkSMSourceProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Calls UpdateInformation() on all sources.
   */
  void UpdatePipelineInformation() override;

  /**
   * Calls Update() on all sources. It also creates output ports if
   * they are not already created.
   */
  virtual void UpdatePipeline();

  /**
   * Calls Update() on all sources with the given time request.
   * It also creates output ports if they are not already created.
   */
  virtual void UpdatePipeline(double time);

  //@{
  /**
   * Returns if the output port proxies have been created.
   */
  vtkGetMacro(OutputPortsCreated, int);
  //@}

  /**
   * Return the number of output ports.
   */
  virtual unsigned int GetNumberOfOutputPorts();

  /**
   * Return an output port.
   */
  virtual vtkSMOutputPort* GetOutputPort(unsigned int idx);

  /**
   * Return an output port, given the name for the port.
   * Each output port is assigned a unique name (either using the xml
   * configuration or automatically). The automatically assigned names are of
   * the type Output0, Output1 etc.
   */
  virtual vtkSMOutputPort* GetOutputPort(const char* portname);

  /**
   * Returns the port index, given the name of an output port.
   * Each output port is assigned a unique name (either using the xml
   * configuration or automatically). The automatically assigned names are of
   * the type Output0, Output1 etc.
   * Returns VTK_UNSIGNED_INT_MAX (i.e. ~0u) when a port with given port name
   * does not exist.
   */
  virtual unsigned int GetOutputPortIndex(const char* portname);

  /**
   * Returns the output port name given its index.
   * Each output port is assigned an unique name (either using the xml
   * configuration or automatically). The automatically assigned names are of
   * the type Output-0, Output-1 etc.
   */
  virtual const char* GetOutputPortName(unsigned int index);

  //@{
  /**
   * It is possible to provide some documentation for each output port in the
   * configuration xml. These methods provide access to the port specific
   * documentation, if any. If no documentation is present, these methods will
   * return 0.
   */
  vtkSMDocumentation* GetOutputPortDocumentation(unsigned int index);
  vtkSMDocumentation* GetOutputPortDocumentation(const char* portname);
  //@}

  /**
   * Creates the output port proxies for this filter.
   * Each output port proxy corresponds to an actual output port on the
   * algorithm.
   */
  virtual void CreateOutputPorts();

  //@{
  /**
   * DataInformation is used by the source proxy to obtain information
   * on the output(s) from the server.
   * If \c update is false the pipeline will not
   * be updated before gathering the data information.
   */
  vtkPVDataInformation* GetDataInformation() { return this->GetDataInformation(0); }
  vtkPVDataInformation* GetDataInformation(unsigned int outputIdx);
  //@}

  /**
   * Creates extract selection proxies for each output port if not already
   * created.
   */
  virtual void CreateSelectionProxies();

  /**
   * Set/Get the selection input. This is used to set the selection input to the
   * extarction proxy for the output port identified by \c portIndex.
   * If no extraction proxies are present, this method has no effect.
   */
  void SetSelectionInput(unsigned int portIndex, vtkSMSourceProxy* input, unsigned int outputPort);

  //@{
  /**
   * API to query selection input set using SetSelectionInput.
   */
  vtkSMSourceProxy* GetSelectionInput(unsigned int portIndex);
  unsigned int GetSelectionInputPort(unsigned int portIndex);
  //@}

  /**
   * Clean all selection inputs for the given port.
   */
  void CleanSelectionInputs(unsigned int portIndex);

  /**
   * Returns the source proxy which provides the selected data from the given
   * output port.
   */
  vtkSMSourceProxy* GetSelectionOutput(unsigned int portIndex);

  //@{
  /**
   * This returns information about whether the VTK algorithm supports
   * multiple processes or not. SINGLE_PROCESS means that this algorithm
   * works only in serial, MULTIPLE_PROCESSES means that it will only
   * useful in parallel (or it is useless in serial), BOTH means both :-)
   * Default is BOTH. This ivar is filled from the xml configuration.
   * This variable should not be used to determine if MPI is initialized.
   * Instead use MPISupport for that.
   */
  vtkGetMacro(ProcessSupport, int);
  //@}

  //@{
  /**
   * This returns information about whether the VTK algorithm explicitly
   * needs MPI to be initialized. It still may only run with a single
   * process. An example of this is a reader that uses MPI IO routines.
   */
  vtkGetMacro(MPIRequired, bool);
  //@}

  /**
   * Returns the number of output ports provided by the algorithm.
   */
  unsigned int GetNumberOfAlgorithmOutputPorts();

  /**
   * Returns the number of non-optional input ports required by the algorithm.
   * This value is cached after the first call.
   */
  virtual unsigned int GetNumberOfAlgorithmRequiredInputPorts();

  /**
   * Overridden to reserve additional IDs for use by "ExtractSelection" proxies.
   */
  vtkTypeUInt32 GetGlobalID() override;

  enum ProcessSupportType
  {
    SINGLE_PROCESS,
    MULTIPLE_PROCESSES,
    BOTH
  };

  /**
   * Marks the selection proxies dirty as well as chain to superclass.
   */
  void MarkDirty(vtkSMProxy* modifiedProxy) override;

protected:
  vtkSMSourceProxy();
  ~vtkSMSourceProxy() override;

  friend class vtkSMInputProperty;
  friend class vtkSMOutputPort;

  int OutputPortsCreated;

  int ProcessSupport;
  bool MPIRequired;

  /**
   * Mark the data information as invalid.
   */
  virtual void InvalidateDataInformation();

  /**
   * Call superclass' and then assigns a new executive
   * (vtkCompositeDataPipeline)
   */
  void CreateVTKObjects() override;

  char* ExecutiveName;
  vtkSetStringMacro(ExecutiveName);

  /**
   * Read attributes from an XML element.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) override;

  /**
   * Internal method which creates the output port proxies using the proxy specified.
   */
  void CreateOutputPortsInternal(vtkSMProxy* op);

  //@{
  /**
   * Method to set an output port at the given index. Provided for subclasses to
   * add output ports. It replaces the output port at the given index, if any,
   * The output ports will be resize to fit the specified index.
   */
  void SetOutputPort(
    unsigned int index, const char* name, vtkSMOutputPort* port, vtkSMDocumentation* doc);
  void RemoveAllOutputPorts();
  void SetExtractSelectionProxy(unsigned int index, vtkSMSourceProxy* proxy);
  void RemoveAllExtractSelectionProxies();
  //@}

  /**
   * Overwritten from superclass to invoke
   */
  void PostUpdateData() override;

  // flag used to avoid creation of extract selection proxies for this source
  // proxy.
  bool DisableSelectionProxies;
  bool SelectionProxiesCreated;

private:
  vtkSMSourceProxyInternals* PInternals;

  // used by GetNumberOfAlgorithmOutputPorts to cache the value to avoid
  // unnecessary information gathers.
  unsigned int NumberOfAlgorithmOutputPorts;
  unsigned int NumberOfAlgorithmRequiredInputPorts;

  vtkSMSourceProxy(const vtkSMSourceProxy&) = delete;
  void operator=(const vtkSMSourceProxy&) = delete;
};

#endif
