/*=========================================================================

   Program: ParaView
   Module:    pqPluginActionGroupBehavior.h

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
#ifndef pqPluginActionGroupBehavior_h
#define pqPluginActionGroupBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class QMainWindow;

/**
* @ingroup Behaviors
* pqPluginActionGroupBehavior adds support for loading menus/toolbars from
* plugins. In other words, it adds support for plugins created using
* ADD_PARAVIEW_ACTION_GROUP.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqPluginActionGroupBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPluginActionGroupBehavior(QMainWindow* parent = 0);

public Q_SLOTS:
  void addPluginInterface(QObject* iface);

private:
  Q_DISABLE_COPY(pqPluginActionGroupBehavior)
};

#endif
