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
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
void vtkPVPlugin::ImportPlugin(vtkPVPlugin* plugin)
{
  // Register the plugin with the plugin manager on the current process. That
  // will kick in the code to process the plugin e.g. initialize CSInterpreter,
  // load XML etc.
  vtkPVPluginTracker::GetInstance()->RegisterPlugin(plugin);
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
