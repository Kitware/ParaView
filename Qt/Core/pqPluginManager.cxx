/*=========================================================================

   Program: ParaView
   Module:    pqPluginManager.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqPluginManager.h"

#include <QLibrary>
#include <QPluginLoader>

#include "vtkSMXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkSMObject.h"

#include "pqPlugin.h"

pqPluginManager::pqPluginManager(QObject* p)
  : QObject(p)
{
}

pqPluginManager::~pqPluginManager()
{
}

QObjectList pqPluginManager::interfaces()
{
  return this->Interfaces;
}

bool pqPluginManager::loadPlugin(pqServer* /*server*/, const QString& lib)
{
  bool success = false;

  // look for SM stuff in the plugin
  QLibrary plugin(lib);
  if(plugin.load())
    {
    // TODO  support server side loading
    typedef const char* (*ModuleXML)();
    typedef void (*ModuleInit)(vtkClientServerInterpreter*);
    ModuleXML modxml = (ModuleXML)plugin.resolve("ModuleXML");
    ModuleInit modinit = (ModuleInit)plugin.resolve("ModuleInit");

    // module consists of either xml&init or just xml
    if(modxml && modinit)
      {
      (*modinit)(vtkProcessModule::GetProcessModule()->GetInterpreter());
      }
    
    if(modxml)
      {
      success = true;
      const char* xml = (*modxml)();
      vtkSMXMLParser* parser = vtkSMXMLParser::New();
      parser->Parse(xml);
      parser->ProcessConfiguration(vtkSMObject::GetProxyManager());
      parser->Delete();
      emit this->serverManagerExtensionLoaded();
      }

    QPluginLoader qplugin(lib);
    qplugin.load();
    pqPlugin* pqplugin = qobject_cast<pqPlugin*>(qplugin.instance());
    if(pqplugin)
      {
      success = true;
      QObjectList ifaces = pqplugin->interfaces();
      foreach(QObject* iface, ifaces)
        {
        this->Interfaces.append(iface);
        emit this->guiInterfaceLoaded(iface);
        }
      }
    }

  // this is not a paraview plugin, unload it
  if(success == false && plugin.isLoaded())
    {
    plugin.unload();
    }

  return success;
}


