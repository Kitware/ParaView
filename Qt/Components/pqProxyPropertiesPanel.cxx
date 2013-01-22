/*=========================================================================

   Program: ParaView
   Module: pqProxyPropertiesPanel.cxx

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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

#include "pqProxyPropertiesPanel.h"

#include <QFormLayout>

#include "pq3DWidgetPropertyWidget.h"
#include "pqPropertiesPanelItem.h"
#include "pqPropertyWidget.h"
#include "pqProxy.h"

pqProxyPropertiesPanel::pqProxyPropertiesPanel(pqProxy *proxy_, QWidget *parent_)
  : QWidget(parent_),
    m_proxy(proxy_)
{
  setObjectName("ProxyPanel");

  // setup layout
  m_layout = new QFormLayout;
  m_layout->setMargin(0);
  m_layout->setSpacing(2);
  m_layout->setLabelAlignment(Qt::AlignLeft | Qt::AlignHCenter);
  setLayout(m_layout);
}

pqProxyPropertiesPanel::~pqProxyPropertiesPanel()
{
}

pqProxy* pqProxyPropertiesPanel::proxy() const
{
  return m_proxy;
}

void pqProxyPropertiesPanel::apply()
{
  foreach(const pqPropertiesPanelItem &item, m_items)
    {
    pqPropertyWidget *widget = item.PropertyWidget;
    if(widget)
      {
      widget->apply();
      }
    }
}

void pqProxyPropertiesPanel::reset()
{
  foreach(const pqPropertiesPanelItem &item, m_items)
    {
    pqPropertyWidget *widget = item.PropertyWidget;
    if(widget)
      {
      widget->reset();
      }
    }
}

void pqProxyPropertiesPanel::setVisible(bool value)
{
  this->Superclass::setVisible(value);
  foreach(const pqPropertiesPanelItem &item, m_items)
    {
    pq3DWidgetPropertyWidget *widget =
      qobject_cast<pq3DWidgetPropertyWidget*>(item.PropertyWidget);
    if (widget)
      {
      if (value)
        {
        widget->select();
        }
      else
        {
        widget->deselect();
        }
      }
    }
}

void pqProxyPropertiesPanel::addPropertyWidgetItem(pqPropertiesPanelItem item)
{
  m_items.append(item);

  if(item.LabelWidget)
    {
    m_layout->addRow(item.LabelWidget, item.PropertyWidget);
    item.LabelWidget->setParent(this);
    item.PropertyWidget->setParent(this);
    }
  else
    {
    m_layout->addRow(item.PropertyWidget);
    item.PropertyWidget->setParent(this);
    }
}

QList<pqPropertiesPanelItem> pqProxyPropertiesPanel::propertyWidgetItems() const
{
  return m_items;
}
