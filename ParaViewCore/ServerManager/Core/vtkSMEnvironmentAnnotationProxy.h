/*=========================================================================

  Program:   ParaView
  Module:    vtkSMEnvironmentAnnotationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMEnvironmentAnnotationProxy - proxy for a VTK paraview environment 
// on a server
// .SECTION Description
// vtkSMEnvironmentAnnotationProxy provides methods to add visual annotations
// to the render window.
// Each port represents one output of one filter. These are created
// automatically (when CreateOutputPorts() is called) by the source.
// Each vtkSMEnvironmentAnnotationProxy is capable of displaying the 
// OS type, the user name, the date and time, the file name, and the 
// full path of a loaded source.
// .SECTION See Also
// vtkSMProxy vtkSMOutputPort vtkSMInputProperty

#ifndef __vtkSMEnvironmentAnnotationProxy_h
#define __vtkSMEnvironmentAnnotationProxy_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMSourceProxy.h"

class vtkPVArrayInformation;
class vtkPVDataInformation;
class vtkPVDataSetAttributesInformation;
//BTX
struct vtkSMEnvironmentAnnotationProxyInternals;
//ETX
class vtkSMOutputPort;
class vtkSMProperty;
class vtkSMSessionProxyManager;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMEnvironmentAnnotationProxy : public vtkSMSourceProxy
{
public:
  static vtkSMEnvironmentAnnotationProxy* New();
  vtkTypeMacro(vtkSMEnvironmentAnnotationProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void UpdateVTKObjects();
protected:
  vtkSMEnvironmentAnnotationProxy();
  ~vtkSMEnvironmentAnnotationProxy();

private:
  vtkSMEnvironmentAnnotationProxy(const vtkSMEnvironmentAnnotationProxy&); // Not implemented
  void operator=(const vtkSMEnvironmentAnnotationProxy&); // Not implemented
//ETX
};

#endif
