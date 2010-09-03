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

class VTK_EXPORT vtkPMSourceProxy : public vtkPMProxy
{
public:
  static vtkPMSourceProxy* New();
  vtkTypeMacro(vtkPMSourceProxy, vtkPMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the vtkClientServerID for an output port, if valid.
  vtkClientServerID GetOutputPortID(int port);

//BTX
  // Description:
  // Invoke a given method on the underneath objects
  virtual void Invoke(vtkSMMessage* msg);

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
  // Triggers UpdatePipeline().
  virtual void UpdatePipeline(int port, double time, bool doTime);

  // Description:
  // Triggers UpdateInformation().
  virtual void UpdateInformation()

  char *ExecutiveName;
  vtkSetStringMacro(ExecutiveName);
private:
  vtkPMSourceProxy(const vtkPMSourceProxy&); // Not implemented
  void operator=(const vtkPMSourceProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
  //ETX
};

#endif
