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
/**
 * @class   vtkSICompoundSourceProxy
 *
 * vtkSICompoundSourceProxy is the server-side helper for a
 * vtkSMCompoundSourceProxy.
 * It provides the mapping to the exposed output port to the underneath
 * internal sub-proxy.
*/

#ifndef vtkSICompoundSourceProxy_h
#define vtkSICompoundSourceProxy_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSISourceProxy.h"

class vtkAlgorithm;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSICompoundSourceProxy : public vtkSISourceProxy
{
public:
  static vtkSICompoundSourceProxy* New();
  vtkTypeMacro(vtkSICompoundSourceProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns the vtkAlgorithmOutput for an output port, if valid.
   */
  virtual vtkAlgorithmOutput* GetOutputPort(int port) VTK_OVERRIDE;

protected:
  vtkSICompoundSourceProxy();
  ~vtkSICompoundSourceProxy();

  /**
   * Read xml-attributes.
   */
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element) VTK_OVERRIDE;

  /**
   * Create the output ports and add post filters for each output port.
   */
  virtual bool CreateOutputPorts() VTK_OVERRIDE;

private:
  vtkSICompoundSourceProxy(const vtkSICompoundSourceProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSICompoundSourceProxy&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
