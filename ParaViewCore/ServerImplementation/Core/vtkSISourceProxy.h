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
// .NAME vtkSISourceProxy
// .SECTION Description
// vtkSISourceProxy is the server-side helper for a vtkSMSourceProxy.
// It adds support to handle various vtkAlgorithm specific Invoke requests
// coming from the client. vtkSISourceProxy also inserts post-processing filters
// for each output port from the vtkAlgorithm. These post-processing filters
// deal with things like parallelizing the data etc.

#ifndef vtkSISourceProxy_h
#define vtkSISourceProxy_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIProxy.h"

class vtkAlgorithm;
class vtkAlgorithmOutput;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSISourceProxy : public vtkSIProxy
{
public:
  static vtkSISourceProxy* New();
  vtkTypeMacro(vtkSISourceProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the vtkAlgorithmOutput for an output port, if valid.
  virtual vtkAlgorithmOutput* GetOutputPort(int port);

  // Description:
  // Triggers UpdateInformation() on vtkObject if possible.
  virtual void UpdatePipelineInformation();

  // Description:
  // Triggers UpdatePipeline().
  // Called from client.
  virtual void UpdatePipeline(int port, double time, bool doTime);

  // Description:
  // setups extract selection proxies.
  virtual void SetupSelectionProxy(int port, vtkSIProxy* extractSelection);

  // Description:
  // Allow to shut down pipeline execution. This is particulary useful for
  // a Catalyst session that does not contains any real data.
  virtual void SetDisablePipelineExecution(bool value)
  { this->DisablePipelineExecution = value; }

  // Description:
  // Overridden to update the output ports.
  void RecreateVTKObjects() VTK_OVERRIDE;

protected:
  vtkSISourceProxy();
  ~vtkSISourceProxy();

  // Description:
  // Overridden to setup the output ports and pipelines for the output ports.
  bool CreateVTKObjects() VTK_OVERRIDE;

  // Description:
  // Read xml-attributes.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);

  // Description:
  // Create the output ports and add post filters for each output port.
  // CreateOutputPorts is only called when an output-port is requested, i.e.
  // GetOutputPort() is called.
  virtual bool CreateOutputPorts();

  // Description:
  // Callbacks to add start/end events to the timer log.
  void MarkStartEvent();
  void MarkEndEvent();

  char *ExecutiveName;
  vtkSetStringMacro(ExecutiveName);
  bool DisablePipelineExecution;

  friend class vtkSICompoundSourceProxy;
private:
  vtkSISourceProxy(const vtkSISourceProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSISourceProxy&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
  bool PortsCreated;

};

#endif
