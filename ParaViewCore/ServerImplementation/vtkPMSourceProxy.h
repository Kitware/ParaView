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
// .NAME vtkPMSourceProxy
// .SECTION Description
// vtkPMSourceProxy is the server-side helper for a vtkSMSourceProxy.
// It adds support to handle various vtkAlgorithm specific Invoke requests
// coming from the client. vtkPMSourceProxy also inserts post-processing filters
// for each output port from the vtkAlgorithm. These post-processing filters
// deal with things like parallelizing the data etc.

#ifndef __vtkPMSourceProxy_h
#define __vtkPMSourceProxy_h

#include "vtkPMProxy.h"

class vtkAlgorithm;
class vtkAlgorithmOutput;

class VTK_EXPORT vtkPMSourceProxy : public vtkPMProxy
{
public:
  static vtkPMSourceProxy* New();
  vtkTypeMacro(vtkPMSourceProxy, vtkPMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the vtkAlgorithmOutput for an output port, if valid.
  virtual vtkAlgorithmOutput* GetOutputPort(int port);

  // Description:
  // Triggers UpdatePipeline().
  // Called from client.
  virtual void UpdatePipeline(int port, double time, bool doTime);

  // Description:
  // When using streaming, this method is called instead on UpdatePipeline().
  virtual void UpdateStreamingPipeline(
    int pass, int num_of_passes, double resolution,
    int port, double time, bool doTime);

  // Description:
  // setups extract selection proxies.
  virtual void SetupSelectionProxy(int port, vtkPMProxy* extractSelection);

//BTX
protected:
  vtkPMSourceProxy();
  ~vtkPMSourceProxy();

  // Description:
  // Creates the VTKObjects. Overridden to add post-filters to the pipeline.
  virtual bool CreateVTKObjects(vtkSMMessage* message);

  // Description:
  // Read xml-attributes.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);

  // Description:
  // Create the output ports and add post filters for each output port.
  // CreateOutputPorts is only called when an output-port is requested, i.e.
  // GetOutputPort() is called.
  virtual bool CreateOutputPorts();

  // Description:
  // Called to initialize a single output port. This assigns each output port an
  // interpreter id and then initializes the translator/extract pieces/post
  // filters.
  virtual bool InitializeOutputPort(vtkAlgorithm* alo, int port);

  // Description:
  // Create the extent translator (sources with no inputs only).
  // Needs to be before "ExtractPieces" because translator propagates.
  bool CreateTranslatorIfNecessary(vtkAlgorithm* algo, int port);

  // Description:
  // Insert a filter to extract (and redistribute) unstructured
  // pieces if the source cannot generate pieces.
  void InsertExtractPiecesIfNecessary(vtkAlgorithm* algo, int port);

  // Description:
  // Insert a filter to create the Post Filter
  // so that filters can request data conversions
  void InsertPostFilterIfNecessary(vtkAlgorithm* algo, int port);

  // Description:
  // Callbacks to add start/end events to the timer log.
  void MarkStartEvent();
  void MarkEndEvent();

  char *ExecutiveName;
  vtkSetStringMacro(ExecutiveName);

  friend class vtkPMCompoundSourceProxy;
private:
  vtkPMSourceProxy(const vtkPMSourceProxy&); // Not implemented
  void operator=(const vtkPMSourceProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
  bool PortsCreated;
  //ETX
};

#endif
