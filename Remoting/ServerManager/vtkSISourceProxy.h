/*=========================================================================

  Program:   ParaView
  Module:    vtkSISourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSISourceProxy
 *
 * vtkSISourceProxy is the server-side helper for a vtkSMSourceProxy.
 * It adds support to handle various vtkAlgorithm specific Invoke requests
 * coming from the client. vtkSISourceProxy also inserts post-processing filters
 * for each output port from the vtkAlgorithm. These post-processing filters
 * deal with things like parallelizing the data etc.
*/

#ifndef vtkSISourceProxy_h
#define vtkSISourceProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProxy.h"

class vtkAlgorithm;
class vtkAlgorithmOutput;
class vtkInformation;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSISourceProxy : public vtkSIProxy
{
public:
  static vtkSISourceProxy* New();
  vtkTypeMacro(vtkSISourceProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the vtkAlgorithmOutput for an output port, if valid.
   */
  virtual vtkAlgorithmOutput* GetOutputPort(int port);

  /**
   * Triggers UpdateInformation() on vtkObject if possible.
   */
  void UpdatePipelineInformation() override;

  /**
   * Triggers UpdatePipeline().
   * Called from client.
   */
  virtual void UpdatePipeline(int port, double time, bool doTime);

  /**
   * setups extract selection proxies.
   */
  virtual void SetupSelectionProxy(int port, vtkSIProxy* extractSelection);

  /**
   * Allow to shut down pipeline execution. This is particularly useful for
   * a Catalyst session that does not contains any real data.
   */
  virtual void SetDisablePipelineExecution(bool value) { this->DisablePipelineExecution = value; }

  /**
   * Overridden to update the output ports.
   */
  void RecreateVTKObjects() override;

protected:
  vtkSISourceProxy();
  ~vtkSISourceProxy() override;

  /**
   * Overridden to setup the output ports and pipelines for the output ports.
   */
  bool CreateVTKObjects() override;

  /**
   * Read xml-attributes.
   */
  bool ReadXMLAttributes(vtkPVXMLElement* element) override;

  /**
   * Called after CreateVTKObjects(). The main difference for subclasses when
   * overriding CreateVTKObjects() or OnCreateVTKObjects() is that
   * CreateVTKObjects() is called before ReadXMLAttributes() is called, while
   * OnCreateVTKObjects() is called after ReadXMLAttributes().
   */
  void OnCreateVTKObjects() override;

  /**
   * Create the output ports and add post filters for each output port.
   * CreateOutputPorts is only called when an output-port is requested, i.e.
   * GetOutputPort() is called.
   */
  virtual bool CreateOutputPorts();

  //@{
  /**
   * Callbacks to add start/end events to the timer log.
   */
  void MarkStartEvent();
  void MarkEndEvent();
  //@}

  char* ExecutiveName;
  vtkSetStringMacro(ExecutiveName);
  bool DisablePipelineExecution;

  friend class vtkSICompoundSourceProxy;

private:
  vtkSISourceProxy(const vtkSISourceProxy&) = delete;
  void operator=(const vtkSISourceProxy&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
  bool PortsCreated;
  int StartEventCounter;
};

#endif
