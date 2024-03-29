// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqArrayStatusPropertyWidget.h"

#include "pqArraySelectionWidget.h"
#include "pqTreeViewSelectionHelper.h"
#include "vtkCommand.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QtDebug>

namespace
{

int parseAlignment(const char* data)
{
  if (data && strcmp(data, "right") == 0)
  {
    return Qt::AlignRight;
  }

  if (data && strcmp(data, "center") == 0)
  {
    return Qt::AlignHCenter;
  }

  if (data && strcmp(data, "justify") == 0)
  {
    return Qt::AlignJustify;
  }

  return Qt::AlignLeft;
}

}

class pqArrayStatusPropertyWidget::pqInternals
{
  QPointer<pqArraySelectionWidget> ArraySelectionWidget;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  std::map<vtkSMProperty*, std::pair<int, QString>> PropertyMap;

public:
  void setArraySelectionWidget(pqArraySelectionWidget* wdg) { this->ArraySelectionWidget = wdg; }

  void connectColumn(
    int column, const QString& key, vtkSMProperty* property, pqArrayStatusPropertyWidget* self)
  {
    this->PropertyMap[property] = std::make_pair(column, key);
    this->VTKConnector->Connect(
      property, vtkCommand::PropertyModifiedEvent, self, SLOT(updateColumn(vtkObject*)));
    this->updateColumn(property);
  }

  void updateColumn(vtkSMProperty* property)
  {
    auto iter = this->PropertyMap.find(property);
    if (iter == this->PropertyMap.end())
    {
      return;
    }

    QMap<QString, QString> mapping;
    vtkSMPropertyHelper helper(property);
    for (unsigned int cc = 0; (cc + 1) < helper.GetNumberOfElements(); cc += 2)
    {
      mapping[helper.GetAsString(cc)] = helper.GetAsString(cc + 1);
    }
    this->ArraySelectionWidget->setColumnData(
      iter->second.first, iter->second.second, std::move(mapping));
  }
};

//-----------------------------------------------------------------------------
pqArrayStatusPropertyWidget::pqArrayStatusPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqArrayStatusPropertyWidget::pqInternals())
{
  auto* xmlHints = smgroup->GetHints()
    ? smgroup->GetHints()->FindNestedElementByName("ArrayStatusPropertyWidget")
    : nullptr;

  int customColumnCount = 0;
  if (xmlHints)
  {
    xmlHints->GetScalarAttribute("customColumnCount", &customColumnCount);
    customColumnCount = std::max(0, customColumnCount);
  }

  auto selectorWidget = new pqArraySelectionWidget(customColumnCount + 1, this);
  selectorWidget->setObjectName("SelectionWidget");
  selectorWidget->setHeaderLabel(
    QCoreApplication::translate("ServerManagerXML", smgroup->GetXMLLabel()));
  selectorWidget->setMaximumRowCountBeforeScrolling(
    pqPropertyWidget::hintsWidgetHeightNumberOfRows(smgroup->GetHints()));
  selectorWidget->header()->setDefaultSectionSize(20);
  selectorWidget->header()->setMinimumSectionSize(20);

  // add context menu and custom indicator for sorting and filtering.
  new pqTreeViewSelectionHelper(selectorWidget);

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->addWidget(selectorWidget);
  hbox->setContentsMargins(0, 0, 0, 0);
  hbox->setSpacing(4);

  for (unsigned int cc = 0; cc < smgroup->GetNumberOfProperties(); cc++)
  {
    vtkSMProperty* prop = smgroup->GetProperty(cc);
    if (prop && prop->GetInformationOnly() == 0)
    {
      const char* property_name = smproxy->GetPropertyName(prop);
      if (auto hints = prop->GetHints()
          ? prop->GetHints()->FindNestedElementByName("ArraySelectionWidget")
          : nullptr)
      {
        selectorWidget->setIconType(property_name, hints->GetAttribute("icon_type"));
      }
      this->addPropertyLink(selectorWidget, property_name, SIGNAL(widgetModified()), prop);
    }
  }

  // Process custom columns.
  int column = 1;
  auto& internals = (*this->Internals);
  internals.setArraySelectionWidget(selectorWidget);
  for (unsigned int cc = 0; cc < (xmlHints ? xmlHints->GetNumberOfNestedElements() : 0); ++cc)
  {
    auto* child = xmlHints->GetNestedElement(cc);
    if (child && strcmp(child->GetName(), "Column") == 0)
    {
      const char* label = child->GetAttributeOrEmpty("label");
      selectorWidget->setHeaderLabel(column, label);
      if (auto* align = child->GetAttribute("align"))
      {
        selectorWidget->setColumnItemData(column, Qt::TextAlignmentRole, ::parseAlignment(align));
      }

      for (unsigned int kk = 0; kk < child->GetNumberOfNestedElements(); ++kk)
      {
        auto* propertyXML = child->GetNestedElement(kk);
        if (propertyXML && strcmp(propertyXML->GetName(), "Property") == 0)
        {
          auto* smproperty = smproxy->GetProperty(propertyXML->GetAttribute("name"));
          if (!smproperty)
          {
            qCritical() << "No property named " << propertyXML->GetAttribute("name");
            continue;
          }
          internals.connectColumn(column, propertyXML->GetAttribute("key"), smproperty, this);
        }
      }
      ++column;
    }
  }

  // don't show label
  this->setShowLabel(false);
}

//-----------------------------------------------------------------------------
pqArrayStatusPropertyWidget::pqArrayStatusPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqArrayStatusPropertyWidget::pqInternals())
{
  auto selectorWidget = new pqArraySelectionWidget(this);
  selectorWidget->setObjectName("SelectionWidget");
  selectorWidget->setHeaderLabel(
    QCoreApplication::translate("ServerManagerXML", smproperty->GetXMLLabel()));
  selectorWidget->setMaximumRowCountBeforeScrolling(
    pqPropertyWidget::hintsWidgetHeightNumberOfRows(smproperty->GetHints()));

  auto& internals = (*this->Internals);
  internals.setArraySelectionWidget(selectorWidget);

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->addWidget(selectorWidget);
  hbox->setContentsMargins(0, 0, 0, 0);
  hbox->setSpacing(4);

  const char* property_name = smproxy->GetPropertyName(smproperty);

  if (auto hints = smproperty->GetHints()
      ? smproperty->GetHints()->FindNestedElementByName("ArraySelectionWidget")
      : nullptr)
  {
    selectorWidget->setIconType(property_name, hints->GetAttribute("icon_type"));
  }

  this->addPropertyLink(selectorWidget, property_name, SIGNAL(widgetModified()), smproperty);

  // don't show label
  this->setShowLabel(false);
}

//-----------------------------------------------------------------------------
pqArrayStatusPropertyWidget::~pqArrayStatusPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqArrayStatusPropertyWidget::updateColumn(vtkObject* obj)
{
  auto& internals = (*this->Internals);
  internals.updateColumn(vtkSMProperty::SafeDownCast(obj));
}
