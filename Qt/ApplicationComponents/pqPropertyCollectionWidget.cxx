/*=========================================================================

   Program: ParaView
   Module:  pqPropertyCollectionWidget.cxx

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
#include "pqPropertyCollectionWidget.h"
#include "ui_pqPropertyCollectionWidget.h"

#include "pqProxyWidget.h"
#include "pqSMAdaptor.h"
#include "pqView.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMVectorProperty.h"
#include "vtkSmartPointer.h"

#include <QPointer>
#include <QScopedValueRollback>
#include <QtDebug>
#include <cassert>
#include <map>
#include <string>
#include <utility>
#include <vector>

class pqPropertyCollectionWidget::pqInternals
{
public:
  struct Item
  {
    vtkSmartPointer<vtkSMProxy> Proxy;
    QPointer<QToolButton> RemoveButton;
    QPointer<QWidget> SeparatorWidget;
    QPointer<pqProxyWidget> Widget;
  };

  std::vector<Item> Items;
  std::map<std::string, std::string> NameFunctionMap;
  std::pair<std::string, std::string> PrototypeName;
  std::string ItemLabel;
  Ui::PropertyCollectionWidget Ui;
  bool UpdatingProperty = false;

  void setTuple(int index, const char* pname, const QList<QVariant>& value)
  {
    assert(index >= 0 && index < static_cast<int>(this->Items.size()));
    assert(this->NameFunctionMap.find(pname) != this->NameFunctionMap.end());

    auto& item = this->Items[index];
    auto targetProperty = item.Proxy->GetProperty(this->NameFunctionMap[pname].c_str());
    assert(targetProperty != nullptr);

    pqSMAdaptor::setMultipleElementProperty(targetProperty, value);
    item.Proxy->UpdateVTKObjects();
  }

  QList<QVariant> value(const char* pname) const
  {
    QList<QVariant> result;
    for (auto& item : this->Items)
    {
      auto targetProperty = item.Proxy->GetProperty(this->NameFunctionMap.at(pname).c_str());
      assert(targetProperty != nullptr);
      result += pqSMAdaptor::getMultipleElementProperty(targetProperty);
    }
    return result;
  }

  void resize(int count, pqPropertyCollectionWidget* self)
  {
    int delta = count - static_cast<int>(this->Items.size());
    if (delta < 0)
    {
      this->shrinkBy(std::abs(delta), self);
    }
    else if (delta > 0)
    {
      this->growBy(delta, self);
    }
  }

  void shrinkBy(int delta, pqPropertyCollectionWidget* self)
  {
    assert(delta > 0);
    for (int cc = 0; cc < delta; ++cc)
    {
      this->removeItem(static_cast<int>(this->Items.size()) - 1, self);
    }
  }

  void growBy(int delta, pqPropertyCollectionWidget* self)
  {
    assert(delta > 0);
    auto pxm = self->proxy()->GetSessionProxyManager();
    for (int cc = 0; cc < delta; ++cc)
    {
      auto prototype = vtkSmartPointer<vtkSMProxy>::Take(
        pxm->NewProxy(this->PrototypeName.first.c_str(), this->PrototypeName.second.c_str()));
      prototype->SetLocation(0);
      prototype->PrototypeOn();
      prototype->UpdateVTKObjects();

      auto removeButton = new QToolButton();
      removeButton->setObjectName(QString::fromUtf8("removeButton"));
      removeButton->setIcon(QIcon(QString::fromUtf8(":/QtWidgets/Icons/pqDelete.svg")));
      removeButton->setToolTip(
        QApplication::translate("PropertyCollectionWidget", "Remove", nullptr));
      removeButton->setProperty(
        "ParaView::PropertyCollectionWidget::index", static_cast<int>(this->Items.size()));

      QObject::connect(removeButton, &QToolButton::clicked, [this, self, removeButton](bool) {
        const int index =
          removeButton->property("ParaView::PropertyCollectionWidget::index").toInt();
        this->removeItem(index, self);
        self->updateProperties();
      });

      auto separator = pqProxyWidget::newGroupLabelWidget(
        QString("%1 #%2").arg(this->ItemLabel.c_str()).arg(this->Items.size() + 1),
        this->Ui.container, { removeButton });
      this->Ui.container->layout()->addWidget(separator);

      auto widget = new pqProxyWidget(prototype);
      widget->setProperty(
        "ParaView::PropertyCollectionWidget::index", static_cast<int>(this->Items.size()));
      widget->setApplyChangesImmediately(true);
      widget->updatePanel();
      widget->setView(self->view());
      QObject::connect(self, &pqPropertyWidget::viewChanged, widget, &pqProxyWidget::setView);

      QObject::connect(
        widget, &pqProxyWidget::changeFinished, [self]() { self->updateProperties(); });
      this->Ui.container->layout()->addWidget(widget);

      this->Items.emplace_back(Item{ prototype, removeButton, separator, widget });
    }
  }

  void removeItem(int index, pqPropertyCollectionWidget*)
  {
    assert(index >= 0 && index < static_cast<int>(this->Items.size()));
    auto& item = this->Items[index];
    this->Ui.container->layout()->removeWidget(item.SeparatorWidget);
    this->Ui.container->layout()->removeWidget(item.Widget);
    delete item.Widget;
    delete item.SeparatorWidget;
    this->Items.erase(this->Items.begin() + index);

    // rename items
    for (int cc = index; cc < static_cast<int>(this->Items.size()); ++cc)
    {
      auto& citem = this->Items[cc];
      citem.SeparatorWidget->findChild<QLabel*>()->setText(
        QString("<b>%1 #%2</b>").arg(this->ItemLabel.c_str()).arg(cc + 1));
      citem.RemoveButton->setProperty("ParaView::PropertyCollectionWidget::index", cc);
      citem.Widget->setProperty("ParaView::PropertyCollectionWidget::index", cc);
    }
  }
};

//-----------------------------------------------------------------------------
pqPropertyCollectionWidget::pqPropertyCollectionWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqPropertyCollectionWidget::pqInternals())
{
  this->setShowLabel(false);

  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);
  internals.Ui.groupLabel->setText(
    QString("<b>%1</b>").arg(smgroup->GetXMLLabel() ? smgroup->GetXMLLabel() : smgroup->GetName()));

  auto hints = smgroup->GetHints()
    ? smgroup->GetHints()->FindNestedElementByName("PropertyCollectionWidgetPrototype")
    : nullptr;
  if (!hints || !hints->GetAttribute("group") || !hints->GetAttribute("name"))
  {
    qWarning(
      "Missing valid `PropertyCollectionWidgetPrototype` hints. Please fix XML configuration.");
    return;
  }

  internals.PrototypeName.first = hints->GetAttribute("group");
  internals.PrototypeName.second = hints->GetAttribute("name");

  auto pxm = smproxy->GetSessionProxyManager();
  auto prototype = pxm->GetPrototypeProxy(
    internals.PrototypeName.first.c_str(), internals.PrototypeName.second.c_str());
  if (!prototype)
  {
    qWarning() << "Failed to create prototype proxy (" << internals.PrototypeName.first.c_str()
               << ", " << internals.PrototypeName.second.c_str() << ")";
    return;
  }

  internals.ItemLabel = prototype->GetXMLLabel();

  for (unsigned int cc = 0, max = smgroup->GetNumberOfProperties(); cc < max; ++cc)
  {
    auto aprop = smgroup->GetProperty(cc);
    if (!aprop || aprop->GetInformationOnly())
    {
      continue;
    }

    auto prop = vtkSMVectorProperty::SafeDownCast(aprop);
    if (!prop)
    {
      vtkLogF(WARNING, "Only vtkSMVectorProperty subclass are supported. '%s' is of type '%s'.",
        aprop->GetXMLName(), aprop->GetClassName());
      continue;
    }

    if (!prop->GetRepeatable())
    {
      vtkLogF(WARNING, "Only repeatable properties are supported. '%s' is not repeatable.",
        prop->GetXMLName());
      continue;
    }

    auto pname = smproxy->GetPropertyName(prop);
    auto pfunction = smgroup->GetFunction(prop);
    // use name if function is missing.
    pfunction = pfunction ? pfunction : pname;

    auto target = vtkSMVectorProperty::SafeDownCast(prototype->GetProperty(pfunction));
    if (!target)
    {
      vtkLogF(WARNING, "Missing property named '%s' on prototype proxy '%s'", pfunction,
        prototype->GetXMLName());
      continue;
    }

    if (!target->IsA(prop->GetClassName()))
    {
      vtkLogF(WARNING, "Type mismatch between property '%s' (%s) and prototype property '%s' (%s).",
        pname, prop->GetClassName(), pfunction, target->GetClassName());
      continue;
    }

    if (static_cast<int>(target->GetNumberOfElements()) != prop->GetNumberOfElementsPerCommand())
    {
      vtkLogF(WARNING, "Prototype property '%s' must have exactly as many elements as each "
                       "repeated entry for source property '%s'. Expected %d, got %d.",
        pfunction, pname, prop->GetNumberOfElementsPerCommand(), target->GetNumberOfElements());
      continue;
    }

    internals.NameFunctionMap[pname] = pfunction;
    this->addPropertyLink(this, pname, SIGNAL(widgetModified()), prop);
  }

  QObject::connect(internals.Ui.addItemButton, &QToolButton::clicked, [this](bool) {
    this->Internals->growBy(1, this);
    this->updateProperties();
  });

  QObject::connect(internals.Ui.removeAllButton, &QToolButton::clicked, [this](bool) {
    this->Internals->resize(0, this);
    this->updateProperties();
  });
}

//-----------------------------------------------------------------------------
pqPropertyCollectionWidget::~pqPropertyCollectionWidget()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
bool pqPropertyCollectionWidget::event(QEvent* evt)
{
  auto& internals = (*this->Internals);
  if (evt->type() == QEvent::DynamicPropertyChange && !internals.UpdatingProperty)
  {
    auto devt = dynamic_cast<QDynamicPropertyChangeEvent*>(evt);
    this->propertyChanged(devt->propertyName().data());
    return true;
  }

  return this->Superclass::event(evt);
}

//-----------------------------------------------------------------------------
void pqPropertyCollectionWidget::propertyChanged(const char* pname)
{
  auto& internals = (*this->Internals);
  assert(internals.UpdatingProperty == false);

  const QVariant value = this->property(pname);
  if (!value.isValid() || !value.canConvert<QList<QVariant> >())
  {
    return;
  }

  auto listVariants = value.value<QList<QVariant> >();
  auto smprop = vtkSMVectorProperty::SafeDownCast(this->proxy()->GetProperty(pname));
  assert(smprop != nullptr);

  const int numComponents = static_cast<int>(smprop->GetNumberOfElementsPerCommand());
  const int numTuples = listVariants.size() / numComponents;

  // ensure we have exactly as many tuples as the current property value. Note,
  // here, the most recently updated property wins.
  internals.resize(numTuples, this);

  for (int tt = 0; tt < numTuples; ++tt)
  {
    QList<QVariant> tuple;
    for (int cc = 0; cc < numComponents; cc++)
    {
      tuple.push_back(listVariants[tt * numComponents + cc]);
    }
    internals.setTuple(tt, pname, tuple);
  }
}

//-----------------------------------------------------------------------------
void pqPropertyCollectionWidget::updateProperties()
{
  auto& internals = (*this->Internals);
  assert(internals.UpdatingProperty == false);

  bool modified = false;
  for (auto& pair : internals.NameFunctionMap)
  {
    auto value = internals.value(pair.first.c_str());
    if (this->property(pair.first.c_str()) != value)
    {
      QScopedValueRollback<bool> rollback(internals.UpdatingProperty, true);
      this->setProperty(pair.first.c_str(), QVariant(value));
      modified = true;
    }
  }

  if (modified)
  {
    Q_EMIT this->widgetModified();
  }
}
