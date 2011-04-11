/*=========================================================================

  Program:   ParaView
  Module:    vtkSICompoundSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSICompoundSourceProxy
// .SECTION Description
// vtkSICompoundSourceProxy is the server-side helper for a
// vtkSMCompoundSourceProxy.
// It provides the mapping to the exposed output port to the underneath
// internal sub-proxy.

#ifndef __vtkSICompoundSourceProxy_h
#define __vtkSICompoundSourceProxy_h

#include "vtkSISourceProxy.h"

class vtkAlgorithm;

class VTK_EXPORT vtkSICompoundSourceProxy : public vtkSISourceProxy
{
public:
  static vtkSICompoundSourceProxy* New();
  vtkTypeMacro(vtkSICompoundSourceProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the vtkAlgorithmOutput for an output port, if valid.
  virtual vtkAlgorithmOutput* GetOutputPort(int port);

//BTX
protected:
  vtkSICompoundSourceProxy();
  ~vtkSICompoundSourceProxy();

  // Description:
  // Read xml-attributes.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);

  // Description:
  // Create the output ports and add post filters for each output port.
  virtual bool CreateOutputPorts();

private:
  vtkSICompoundSourceProxy(const vtkSICompoundSourceProxy&); // Not implemented
  void operator=(const vtkSICompoundSourceProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
