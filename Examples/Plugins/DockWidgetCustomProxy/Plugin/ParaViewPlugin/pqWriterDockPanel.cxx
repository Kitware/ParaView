/*=========================================================================

   Program: ParaView
   Module:    pqWriterDockPanel.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

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
