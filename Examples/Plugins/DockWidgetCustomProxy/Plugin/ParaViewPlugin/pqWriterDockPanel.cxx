// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqWriterDockPanel.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqProxyWidget.h>
#include <pqServer.h>

#include <QTimer>

//----------------------------------------------------------------------------
pqWriterDockPanel::pqWriterDockPanel(const QString& title, QWidget* parent, Qt::WindowFlags flags)
  : Superclass(title, parent, flags)
{
  // Proxy creation needs to be delayed so ParaView can load the proxy XML
  QTimer::singleShot(0, this, &pqWriterDockPanel::constructor);
}

//----------------------------------------------------------------------------
pqWriterDockPanel::pqWriterDockPanel(QWidget* parent, Qt::WindowFlags flags)
  : Superclass(parent, flags)
{
  // Proxy creation needs to be delayed so ParaView can load the proxy XML
  QTimer::singleShot(0, this, &pqWriterDockPanel::constructor);
}

//----------------------------------------------------------------------------
void pqWriterDockPanel::constructor()
{
  this->setWindowTitle("Writer Dock Panel");

  pqActiveObjects& activeObjects = pqActiveObjects::instance();
  pqServer* server = activeObjects.activeServer();

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  vtkSMProxy* proxy = builder->createProxy("custom_proxy", "MyStringWriter", server, "prototypes");

  pqProxyWidget* proxyWidget = new pqProxyWidget(proxy);
  QObject::connect(proxyWidget, &pqProxyWidget::changeFinished, proxyWidget, &pqProxyWidget::apply);
  this->setWidget(proxyWidget);
}
