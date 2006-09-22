/*=========================================================================

   Program: ParaView
   Module:    pqProxyTabWidget.cxx

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

// this include
#include "pqProxyTabWidget.h"

// Qt includes
#include <QScrollArea>

// VTK includes

// ParaView Server Manager includes

// ParaView widget includes

// ParaView core includes

// ParaView components includes
#include "pqObjectInspectorWidget.h"
#include "pqProxyInformationWidget.h"
#include "pqSourceDisplayEditor.h"


//-----------------------------------------------------------------------------
pqProxyTabWidget::pqProxyTabWidget(QWidget* p)
  : QTabWidget(p)
{
  QScrollArea* scr = new QScrollArea;
  scr->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scr->setWidgetResizable(true);
  scr->setFrameShape(QFrame::NoFrame);
  this->Inspector = new pqObjectInspectorWidget();
  scr->setWidget(this->Inspector);
  this->addTab(scr, tr("Properties"));

  scr = new QScrollArea;
  scr->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scr->setWidgetResizable(true);
  scr->setFrameShape(QFrame::NoFrame);
  this->Display = new pqSourceDisplayEditor();
  scr->setWidget(this->Display);
  this->addTab(scr, tr("Display"));

  scr = new QScrollArea;
  scr->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scr->setWidgetResizable(true);
  scr->setFrameShape(QFrame::NoFrame);
  this->Information = new pqProxyInformationWidget();
  scr->setWidget(this->Information);
  this->addTab(scr, tr("Information"));

  // TODO: allow information page to work without help
  QObject::connect(this->Inspector, SIGNAL(accepted()),
                   this->Information, SLOT(updateInformation()),
                   Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
pqProxyTabWidget::~pqProxyTabWidget()
{
}

//-----------------------------------------------------------------------------
void pqProxyTabWidget::setProxy(pqProxy* proxy) 
{
  this->Inspector->setProxy(proxy);
  this->Display->setProxy(proxy);
  this->Information->setProxy(proxy);
}

void pqProxyTabWidget::setRenderModule(pqRenderModule* rm) 
{
  this->Inspector->setRenderModule(rm);
}

//-----------------------------------------------------------------------------
/// get the proxy for which properties are displayed
pqProxy* pqProxyTabWidget::getProxy()
{
  return this->Information->getProxy();
}

pqObjectInspectorWidget* pqProxyTabWidget::getObjectInspector()
{
  return this->Inspector;
}


