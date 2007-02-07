/*=========================================================================

   Program: ParaView
   Module:    pqPluginManager.h

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

#ifndef _pqPluginManager_h
#define _pqPluginManager_h

#include <QObject>
#include "pqCoreExport.h"

class pqServer;

/// plugin loader takes care of loading plugins
/// containing GUI extensions and server manager extensions
class PQCORE_EXPORT pqPluginManager : public QObject
{
  Q_OBJECT
public:
  pqPluginManager(QObject* p = 0);
  ~pqPluginManager();

  /// attempt to load a plugin
  /// return true on success
  bool loadPlugin(pqServer* server, const QString& lib);

  /// return all interfaces that have been loaded
  QObjectList interfaces();
  
signals:
  /// signal for when an interface is loaded
  void guiInterfaceLoaded(QObject* iface);
  
  /// signal for when some GUI XML is loaded
  /// which can be used to add new readers/writers to the file dialog, etc..
  void guiXMLLoaded(const QString& xml);

  /// notification that new extensions were added to the server manager
  void serverManagerExtensionLoaded();

private:

  QObjectList Interfaces;
};

#endif

