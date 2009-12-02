/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlugin.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPlugin.h"

#include "vtkObjectFactory.h"

#include <vtkstd/vector>

class vtkPVPluginInternals
{
public:
  vtkstd::vector<vtkPVPlugin::Callback> Callbacks;
  vtkstd::vector<void*> CallDatas;
};

static vtkPVPluginInternals PVPluginInternals;

//-----------------------------------------------------------------------------
void vtkPVPlugin::RegisterPluginManagerCallback(
  vtkPVPlugin::Callback callback, void* calldata)
{
  ::PVPluginInternals.Callbacks.push_back(callback);
  ::PVPluginInternals.CallDatas.push_back(calldata);
}

//-----------------------------------------------------------------------------
void vtkPVPlugin::ImportPlugin(vtkPVPlugin* plugin)
{
  for (size_t cc=0; cc < PVPluginInternals.Callbacks.size(); cc++)
    {
    PVPluginInternals.Callbacks[cc](plugin,
      PVPluginInternals.CallDatas[cc]);
    }
}
