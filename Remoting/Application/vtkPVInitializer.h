/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInitializer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPVInitializer_h
#define vtkPVInitializer_h

#include "vtkPVPlugin.h"
#include "vtkPVServerManagerPluginInterface.h"

#include "paraview_client_server.h"
#include "paraview_server_manager.h"

class vtkClientServerInterpreter;

class vtkPVInitializerPlugin : public vtkPVPlugin, public vtkPVServerManagerPluginInterface
{
  const char* GetPluginName() override { return "vtkPVInitializerPlugin"; }
  const char* GetPluginVersionString() override { return "0.0"; }
  bool GetRequiredOnServer() override { return false; }
  bool GetRequiredOnClient() override { return false; }
  const char* GetRequiredPlugins() override { return ""; }
  const char* GetDescription() override { return ""; }
  void GetBinaryResources(std::vector<std::string>&) override {}
  const char* GetEULA() override { return nullptr; }

  void GetXMLs(std::vector<std::string>& xmls) override
  {
    paraview_server_manager_initialize(xmls);
  }

  vtkClientServerInterpreterInitializer::InterpreterInitializationCallback
  GetInitializeInterpreterCallback() override
  {
    return paraview_client_server_initialize;
  }
};

void paraview_initialize()
{
  static vtkPVInitializerPlugin instance;
  vtkPVPlugin::ImportPlugin(&instance);
}

#endif
