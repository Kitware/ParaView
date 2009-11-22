/*=========================================================================

   Program: ParaView
   Module:    pqBrandPluginsLoader.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 
   
   See License_v1.2.txt for the full ParaView license.
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

========================================================================*/
#ifndef __pqBrandPluginsLoader_h 
#define __pqBrandPluginsLoader_h

#include <QObject>
#include "pqCoreExport.h"

class QStringList;

/// pqBrandPluginsLoader is used to load the plugins required to be loaded at
/// the start of a ParaView-based application, if any. Given the list of plugin
/// names, it tries to locate and load them during the application
/// initialization process. You need to use this class only if you are
/// writing a custom main.
class PQCORE_EXPORT pqBrandPluginsLoader : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqBrandPluginsLoader(QObject* parent=0);

  /// Called at startup to load required list of plugins to be loaded on
  /// startup. Typically this method is called after the MainWindow has been
  /// created but before the event loop is started and before the GUI
  /// configuration XMLs, if any, are loaded.
  /// The argument is a list of plugin-names (without platform specific
  /// extensions or path).
  /// The locations where the plugin is searched for are as follows in the
  /// given order:
  /// \li executable-dir (for Mac *.app, it's the app dir)
  /// \li executable-dir/plugins/pluginname
  /// \li *.app/Contents/Plugins/ (for Mac)
  bool loadPlugins(const QStringList& plugins, bool skip_missing_plugins=false);

private:
  Q_DISABLE_COPY(pqBrandPluginsLoader)
};

#endif


