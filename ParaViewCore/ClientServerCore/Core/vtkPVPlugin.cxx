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

#include "vtkPVPluginTracker.h"
#include "vtkProcessModule.h"
#include <vtksys/SystemTools.hxx>

#include <cassert>
#include <sstream>

vtkPVPlugin::EULAConfirmationCallback vtkPVPlugin::EULAConfirmationCallbackPtr = nullptr;

//-----------------------------------------------------------------------------
void vtkPVPlugin::ImportPlugin(vtkPVPlugin* plugin)
{
  assert(plugin != nullptr);

  // If plugin has an EULA, confirm it before proceeding.
  if (plugin->GetEULA() == nullptr || vtkPVPlugin::ConfirmEULA(plugin))
  {
    // Register the plugin with the plugin manager on the current process. That
    // will kick in the code to process the plugin e.g. initialize CSInterpreter,
    // load XML etc.
    vtkPVPluginTracker::GetInstance()->RegisterPlugin(plugin);
  }
}

//-----------------------------------------------------------------------------
vtkPVPlugin::vtkPVPlugin()
{
  this->FileName = NULL;
}

//-----------------------------------------------------------------------------
vtkPVPlugin::~vtkPVPlugin()
{
  delete[] this->FileName;
  this->FileName = NULL;
}

//-----------------------------------------------------------------------------
void vtkPVPlugin::SetFileName(const char* filename)
{
  delete[] this->FileName;
  this->FileName = NULL;
  this->FileName = vtksys::SystemTools::DuplicateString(filename);
}

//-----------------------------------------------------------------------------
void vtkPVPlugin::GetBinaryResources(std::vector<std::string>&)
{
}

//-----------------------------------------------------------------------------
void vtkPVPlugin::SetEULAConfirmationCallback(vtkPVPlugin::EULAConfirmationCallback ptr)
{
  vtkPVPlugin::EULAConfirmationCallbackPtr = ptr;
}

//-----------------------------------------------------------------------------
bool vtkPVPlugin::ConfirmEULA(vtkPVPlugin* plugin)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm->GetPartitionId() == 0 && plugin->GetEULA() != nullptr)
  {
    if (vtkPVPlugin::EULAConfirmationCallbackPtr != nullptr)
    {
      return vtkPVPlugin::EULAConfirmationCallbackPtr(plugin);
    }

    std::ostringstream str;
    str << "-----------------------------------------------------" << endl
        << "  By loading the '" << plugin->GetPluginName()
        << "' plugin you have accepted the EULA shipped with it." << endl
        << "  If that is not acceptable, please restart the application without loading " << endl
        << "  the '" << plugin->GetPluginName() << "' plugin." << endl;
    str << "-----------------------------------------------------" << endl;
    vtkOutputWindowDisplayText(str.str().c_str());
  }
  return true;
}
