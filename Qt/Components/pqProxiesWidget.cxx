/*=========================================================================

   Program: ParaView
   Module:  pqProxiesWidget.cxx

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
#include "pqProxiesWidget.h"

#include "pqExpanderButton.h"
#include "pqPropertiesPanel.h"
#include "pqProxyWidget.h"
#include "pqView.h"
#include "vtkSMProxy.h"
#include "vtkWeakPointer.h"

#include <QList>
#include <QMap>
#include <QPointer>
#include <QVBoxLayout>

class pqProxiesWidget::pqInternals
{
public:
  struct ProxyInfo
  {
    vtkWeakPointer<vtkSMProxy> Proxy;
    QPointer<pqProxyWidget> ProxyWidget;
  };

  /// Key == "ComponentName", Value == List of "ProxyInfo".
  typedef QMap<QString, QList<ProxyInfo> > MapOfProxyList;
  MapOfProxyList ComponentProxies;
  QList<QPointer<pqExpanderButton> > Expanders;
  QPointer<pqView> View;
  QStringList OrderedComponentNames;

  void clear()
  {
    foreach (const QList<ProxyInfo>& proxies, this->ComponentProxies)
    {
      for (int cc = 0, max = proxies.size(); cc < max; cc++)
      {
        delete proxies[cc].ProxyWidget;
      }
    }
    this->OrderedComponentNames.clear();
    this->ComponentProxies.clear();
    this->clearExpanders();
  }
  void clearExpanders()
  {
    foreach (pqExpanderButton* expander, this->Expanders)
    {
      delete expander;
    }
    this->Expanders.clear();
  }
};

//-----------------------------------------------------------------------------
pqProxiesWidget::pqProxiesWidget(QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags)
  , Internals(new pqInternals())
{
}

//-----------------------------------------------------------------------------
pqProxiesWidget::~pqProxiesWidget()
{
  this->Internals->clear();
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::clear()
{
  this->Internals->clear();
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::addProxy(vtkSMProxy* proxy, const QString& componentName /*=QString()*/,
  const QStringList& properties /*=QStringList()*/, bool applyChangesImmediately /*=false*/,
  bool showHeadersFooters /*=true*/)
{
  // TODO: add check to avoid duplicate insertions.

  QList<pqInternals::ProxyInfo>& proxies = this->Internals->ComponentProxies[componentName];
  pqInternals::ProxyInfo info;
  info.Proxy = proxy;
  info.ProxyWidget = new pqProxyWidget(proxy, properties, showHeadersFooters, this);
  info.ProxyWidget->setApplyChangesImmediately(applyChangesImmediately);
  info.ProxyWidget->setView(this->Internals->View);
  this->connect(info.ProxyWidget, SIGNAL(changeAvailable()), SLOT(triggerChangeAvailable()));
  this->connect(info.ProxyWidget, SIGNAL(changeFinished()), SLOT(triggerChangeFinished()));
  this->connect(info.ProxyWidget, SIGNAL(restartRequired()), SLOT(triggerRestartRequired()));

  if (!this->Internals->OrderedComponentNames.contains(componentName))
  {
    this->Internals->OrderedComponentNames.push_back(componentName);
  }

  proxies.push_back(info);
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::updateLayout()
{
  pqInternals& internals = *this->Internals;
  internals.clearExpanders();
  delete this->layout();
  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setMargin(0);
  vbox->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

  // Don't add expander buttons if there's only 1 component and that components
  // name is empty.
  const QList<QString>& keys = internals.OrderedComponentNames;
  bool add_expanders = (keys.size() > 1) || ((keys.size() == 1) && (!keys[0].isEmpty()));
  for (int cc = 0, max = keys.size(); cc < max; cc++)
  {
    const QString& key = keys[cc];
    pqExpanderButton* expander = NULL;
    if (add_expanders)
    {
      expander = new pqExpanderButton(this);
      expander->setObjectName(QString("Expander%1").arg(key));
      expander->setText(key);
      expander->setChecked(true);
      vbox->addWidget(expander);
      internals.Expanders.push_back(expander);
    }

    const QList<pqInternals::ProxyInfo>& proxies = internals.ComponentProxies[key];
    foreach (const pqInternals::ProxyInfo& info, proxies)
    {
      if (expander)
      {
        info.ProxyWidget->connect(expander, SIGNAL(toggled(bool)), SLOT(setVisible(bool)));
      }
      vbox->addWidget(info.ProxyWidget);
    }
  }
  vbox->addStretch(1);
}

//-----------------------------------------------------------------------------
QMap<QString, bool> pqProxiesWidget::expanderState() const
{
  QMap<QString, bool> state;
  const pqInternals& internals = *this->Internals;
  for (pqExpanderButton* expander : internals.Expanders)
  {
    state[expander->text()] = expander->checked();
  }
  return state;
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::setExpanderState(const QMap<QString, bool>& state)
{
  pqInternals& internals = *this->Internals;
  for (pqExpanderButton* expander : internals.Expanders)
  {
    auto iter = state.find(expander->text());
    if (iter != state.end())
    {
      expander->setChecked(iter.value());
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::triggerChangeFinished()
{
  if (pqProxyWidget* pwSender = qobject_cast<pqProxyWidget*>(this->sender()))
  {
    Q_EMIT this->changeFinished(pwSender->proxy());
  }
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::triggerChangeAvailable()
{
  if (pqProxyWidget* pwSender = qobject_cast<pqProxyWidget*>(this->sender()))
  {
    Q_EMIT this->changeAvailable(pwSender->proxy());
  }
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::triggerRestartRequired()
{
  if (pqProxyWidget* pwSender = qobject_cast<pqProxyWidget*>(this->sender()))
  {
    Q_EMIT this->restartRequired(pwSender->proxy());
  }
}

//-----------------------------------------------------------------------------
bool pqProxiesWidget::filterWidgets(bool show_advanced, const QString& filterText)
{
  bool retval = false;
  // forward to internal pqProxyWidget instances.
  pqInternals& internals = *this->Internals;
  for (pqInternals::MapOfProxyList::const_iterator iter = internals.ComponentProxies.constBegin();
       iter != internals.ComponentProxies.constEnd(); ++iter)
  {
    foreach (const pqInternals::ProxyInfo& info, iter.value())
    {
      bool val = info.ProxyWidget->filterWidgets(show_advanced, filterText);
      retval |= val;
    }
  }
  return retval;
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::apply() const
{
  // forward to internal pqProxyWidget instances.
  pqInternals& internals = *this->Internals;
  for (pqInternals::MapOfProxyList::const_iterator iter = internals.ComponentProxies.constBegin();
       iter != internals.ComponentProxies.constEnd(); ++iter)
  {
    foreach (const pqInternals::ProxyInfo& info, iter.value())
    {
      info.ProxyWidget->apply();
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::reset() const
{
  // forward to internal pqProxyWidget instances.
  pqInternals& internals = *this->Internals;
  for (pqInternals::MapOfProxyList::const_iterator iter = internals.ComponentProxies.constBegin();
       iter != internals.ComponentProxies.constEnd(); ++iter)
  {
    foreach (const pqInternals::ProxyInfo& info, iter.value())
    {
      info.ProxyWidget->reset();
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::setView(pqView* pqview)
{
  // forward to internal pqProxyWidget instances.
  pqInternals& internals = *this->Internals;
  internals.View = pqview;
  for (pqInternals::MapOfProxyList::const_iterator iter = internals.ComponentProxies.constBegin();
       iter != internals.ComponentProxies.constEnd(); ++iter)
  {
    foreach (const pqInternals::ProxyInfo& info, iter.value())
    {
      info.ProxyWidget->setView(pqview);
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxiesWidget::updatePanel()
{
  // forward to internal pqProxyWidget instances.
  pqInternals& internals = *this->Internals;
  for (pqInternals::MapOfProxyList::const_iterator iter = internals.ComponentProxies.constBegin();
       iter != internals.ComponentProxies.constEnd(); ++iter)
  {
    foreach (const pqInternals::ProxyInfo& info, iter.value())
    {
      info.ProxyWidget->updatePanel();
    }
  }
}
