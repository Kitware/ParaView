/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingOptionsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStreamingOptionsProxy - write access for streaming paraview options
// .SECTION Description
// This proxy lets the client control a set of global variables related to 
// streaming paraview. The variables are options that the user can modify 
// via the options dialog, to affect how streaming paraview operates. The 
// options are saved in config files and persist acrossed sessions. The 
// object that this proxy controls instantiated into a singleton on each 
// paraview process. When the client changes the settings, the proxy 
// mechanism ensures that the server nodes all see the change.

#ifndef __vtkSMStreamingOptionsProxy_h
#define __vtkSMStreamingOptionsProxy_h

#include "vtkSMProxy.h"

class vtkStreamingOptions;

class VTK_EXPORT vtkSMStreamingOptionsProxy : public vtkSMProxy
{
public:
  static vtkSMStreamingOptionsProxy* New();
  vtkTypeMacro(vtkSMStreamingOptionsProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the instance name used to register the helper proxy with
  // the proxy manager.
  static const char* GetInstanceName();

  // Description:
  // Get the streaming helper proxy.
  static vtkSMStreamingOptionsProxy* GetProxy();

protected:
  vtkSMStreamingOptionsProxy();
  ~vtkSMStreamingOptionsProxy();

  static int StreamingFactoryRegistered;

private:
  vtkSMStreamingOptionsProxy(const vtkSMStreamingOptionsProxy&); // Not implemented
  void operator=(const vtkSMStreamingOptionsProxy&); // Not implemented

};

#endif

