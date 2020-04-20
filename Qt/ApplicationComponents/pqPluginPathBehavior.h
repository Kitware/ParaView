/*=========================================================================

   Program: ParaView
   Module:    pqPluginPathBehavior.h

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
#ifndef pqPluginPathBehavior_h
#define pqPluginPathBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class pqServer;

/**
* @ingroup Behaviors
* Applications may want to support auto-loading of plugins from certain
* locations when a client-server connection is made. In case of ParaView,
* PV_PLUGIN_PATH environment variable is used to locate such auto-load plugin
* locations. This behavior encapsulates this functionality.
* Currently, besides the environment_variable specified in the constructor,
* this class is hard-coded to look at a few locations relative to the
* executable. That can be changed in future allow application to customize
* those locations as well.
* TODO: This class is work in progress. Due to lack of time I am deferring
* this until later. Currently pqPluginManager does this work, we need to move
* the corresponding code to this behavior to allow better customization.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqPluginPathBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPluginPathBehavior(const QString& environment_variable, QObject* parent = 0);

protected Q_SLOTS:
  void loadDefaultPlugins(pqServer*);

private:
  Q_DISABLE_COPY(pqPluginPathBehavior)
};

#endif
