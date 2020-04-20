/*=========================================================================

   Program: ParaView
   Module:  pqPluginToolBarBehavior.h

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
#ifndef pqPluginToolBarBehavior_h
#define pqPluginToolBarBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

/**
 * @class pqPluginToolBarBehavior
 * @ingroup Behaviors
 * @brief Behavior that monitors loaded plugins to handle any implementations of
 *        pqToolBarInterface.
 *
 * pqPluginToolBarBehavior monitors loaded plugins to handle implementations of
 * pqToolBarInterface. If any found, this will execute the appropriate calls to
 * create and add the toolbar to the UI.
 *
 * Plugins that add toolbars generally use `add_paraview_toolbar` macro in their
 * CMake code.
 */

class QMainWindow;
class PQAPPLICATIONCOMPONENTS_EXPORT pqPluginToolBarBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPluginToolBarBehavior(QMainWindow* parent = nullptr);
  virtual ~pqPluginToolBarBehavior();

public Q_SLOTS:
  void addPluginInterface(QObject* iface);

private:
  Q_DISABLE_COPY(pqPluginToolBarBehavior)
};

#endif
