// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxyWidgetItem.h"

#include "pqPropertiesPanel.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqProxyPropertyWidget.h"
#include "pqProxyWidget.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QGridLayout>
#include <QLabel>
#include <QString>

//-----------------------------------------------------------------------------
pqProxyWidgetItem::pqProxyWidgetItem(
  QObject* parentObj, bool advanced, bool informationOnly, const QStringList& searchTags)
  : Superclass(parentObj)
  , SearchTags(searchTags)
  , Advanced(advanced)
  , InformationOnly(informationOnly)
  , Group(false)
{
}

//-----------------------------------------------------------------------------
pqProxyWidgetItem::~pqProxyWidgetItem() = default;

//-----------------------------------------------------------------------------
pqProxyWidgetItem* pqProxyWidgetItem::newItem(pqPropertyWidget* widget, const QString& label,
  bool advanced, bool informationOnly, const QStringList& searchTags, pqProxyWidget* parentObj)
{
  pqProxyWidgetItem* item = new pqProxyWidgetItem(parentObj, advanced, informationOnly, searchTags);
  item->PropertyWidget = widget;
  if (!label.isEmpty() && widget->showLabel())
  {
    QLabel* labelWdg = new QLabel(QString("<p>%1</p>").arg(label), widget->parentWidget());
    labelWdg->setObjectName(QString("%1Label").arg(widget->objectName()));
    labelWdg->setWordWrap(true);
    labelWdg->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    item->LabelWidget = labelWdg;
    // context menu for manipulating defaults.
    labelWdg->setContextMenuPolicy(Qt::CustomContextMenu);
    QPointer<QLabel> labelWdgPtr(labelWdg);
    QPointer<pqPropertyWidget> widgetPtr(widget);
    QObject::connect(labelWdg, &QLabel::customContextMenuRequested, parentObj,
      [labelWdgPtr, widgetPtr, parentObj](const QPoint& pt)
      {
        if (!labelWdgPtr || !widgetPtr)
        {
          return;
        }
        parentObj->showContextMenu(labelWdgPtr->mapToGlobal(pt), widgetPtr);
      });
  }
  else
  {
    // context menu for manipulating defaults.
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    QPointer<pqPropertyWidget> widgetPtr(widget);
    QObject::connect(widget, &QLabel::customContextMenuRequested, parentObj,
      [widgetPtr, parentObj](const QPoint& pt)
      {
        if (!widgetPtr)
        {
          return;
        }
        parentObj->showContextMenu(widgetPtr->mapToGlobal(pt), widgetPtr);
      });
  }
  return item;
}

//-----------------------------------------------------------------------------
pqProxyWidgetItem* pqProxyWidgetItem::newGroupItem(pqPropertyWidget* widget, const QString& label,
  bool showSeparators, bool advanced, const QStringList& searchTags, pqProxyWidget* parentObj)
{
  if (widget->isSingleRowItem())
  {
    pqProxyWidgetItem* item = newItem(widget, label, advanced, false, searchTags, parentObj);
    item->Group = true;
    return item;
  }

  pqProxyWidgetItem* item = newItem(widget, QString(), advanced, false, searchTags, parentObj);
  item->Group = true;
  if (!label.isEmpty() && widget->showLabel() && showSeparators)
  {
    item->GroupHeader = pqProxyWidget::newGroupLabelWidget(label, widget->parentWidget());
    item->GroupFooter = newGroupSeparator(widget->parentWidget());
  }
  return item;
}

//-----------------------------------------------------------------------------
pqProxyWidgetItem* pqProxyWidgetItem::newMultiItemGroupItem(const QString& group_label,
  pqPropertyWidget* widget, const QString& widget_label, bool showSeparators, bool advanced,
  bool informationOnly, const QStringList& searchTags, pqProxyWidget* parentObj)
{
  pqProxyWidgetItem* item =
    newItem(widget, widget_label, advanced, informationOnly, searchTags, parentObj);
  item->Group = true;
  // ensure GroupTag is not nullptr for multi-property groups.
  item->GroupTag = group_label.isNull() ? QString("") : group_label;
  if (!group_label.isEmpty() && showSeparators)
  {
    item->GroupHeader = pqProxyWidget::newGroupLabelWidget(group_label, widget->parentWidget());
    item->GroupFooter = newGroupSeparator(widget->parentWidget());
  }
  return item;
}

//-----------------------------------------------------------------------------
pqPropertyWidget* pqProxyWidgetItem::propertyWidget() const
{
  return this->PropertyWidget;
}

//-----------------------------------------------------------------------------
void pqProxyWidgetItem::appendToDefaultVisibilityForRepresentations(const QString& val)
{
  if (!val.isEmpty() && !this->DefaultVisibilityForRepresentations.contains(val))
  {
    this->DefaultVisibilityForRepresentations.append(val);
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidgetItem::apply() const
{
  if (this->PropertyWidget)
  {
    this->PropertyWidget->apply();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidgetItem::reset() const
{
  if (this->PropertyWidget)
  {
    this->PropertyWidget->reset();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidgetItem::select() const
{
  if (this->PropertyWidget)
  {
    this->PropertyWidget->select();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidgetItem::deselect() const
{
  if (this->PropertyWidget)
  {
    this->PropertyWidget->deselect();
  }
}

//-----------------------------------------------------------------------------
bool pqProxyWidgetItem::canShowWidget(
  bool show_advanced, const QString& filterText, vtkSMProxy* proxy) const
{
  if (show_advanced == false && this->isAdvanced(proxy) == true)
  {
    // skip advanced properties.
    return false;
  }
  else if (filterText.isEmpty() == false &&
    this->SearchTags.filter(filterText, Qt::CaseInsensitive).empty())
  {
    // skip properties not matching search criteria.
    return false;
  }

  Q_FOREACH (const pqPropertyWidgetDecorator* decorator, this->PropertyWidget->decorators())
  {
    if (decorator && !decorator->canShowWidget(show_advanced))
    {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool pqProxyWidgetItem::enableWidget() const
{
  if (this->InformationOnly)
  {
    return false;
  }
  Q_FOREACH (const pqPropertyWidgetDecorator* decorator, this->PropertyWidget->decorators())
  {
    if (decorator && !decorator->enableWidget())
    {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool pqProxyWidgetItem::isAdvanced(vtkSMProxy* proxy) const
{
  if (!this->DefaultVisibilityForRepresentations.empty() && proxy &&
    proxy->GetProperty("Representation") &&
    this->DefaultVisibilityForRepresentations.contains(
      vtkSMPropertyHelper(proxy, "Representation").GetAsString(), Qt::CaseInsensitive))
  {
    return false;
  }
  return this->Advanced;
}

//-----------------------------------------------------------------------------
void pqProxyWidgetItem::show(
  const pqProxyWidgetItem* prevVisibleItem, bool enabled, bool show_advanced) const
{
  // If `this` is not a group, but previous item was, we need to ensure the
  // previous items group footer is visible.
  if (!this->Group && prevVisibleItem && prevVisibleItem->Group && prevVisibleItem->GroupFooter)
  {
    prevVisibleItem->GroupFooter->show();
  }

  // If `this` is a group, and belongs to different group than
  // prevVisibleItem then we need to show `this`'s header.
  if (this->GroupHeader)
  {
    this->GroupHeader->setVisible(this->GroupTag.isNull() || prevVisibleItem == nullptr ||
      this->GroupTag != prevVisibleItem->GroupTag);
  }

  if (this->LabelWidget)
  {
    this->LabelWidget->show();
  }

  this->PropertyWidget->updateWidget(show_advanced);
  this->PropertyWidget->setReadOnly(!enabled);

  if (this->InformationOnly)
  {
    QPalette palette = this->PropertyWidget->palette();
    auto styleSheet =
      QString(":disabled { color: %1; background-color: %2 }")
        .arg(palette.color(QPalette::Active, QPalette::WindowText).name(QColor::HexArgb))
        .arg(palette.color(QPalette::Active, QPalette::Base).name(QColor::HexArgb));
    this->PropertyWidget->setStyleSheet(styleSheet);
  }
  this->PropertyWidget->show();

  if (this->GroupFooter)
  {
    // I know what you're thinking: "Typo!!!", it's not. It's deliberate. A
    // footer should only be shown by a next item. It's only needed if a
    // following item wants it.
    this->GroupFooter->hide();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidgetItem::hide() const
{
  if (this->GroupHeader)
  {
    this->GroupHeader->hide();
  }
  if (this->LabelWidget)
  {
    this->LabelWidget->hide();
  }
  this->PropertyWidget->hide();
  if (this->GroupFooter)
  {
    this->GroupFooter->hide();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidgetItem::appendToLayout(QGridLayout* glayout, bool singleColumn)
{
  if (this->GroupHeader)
  {
    glayout->addWidget(this->GroupHeader, glayout->rowCount(), 0, 1, -1);
  }
  if (this->LabelWidget)
  {
    const int row = glayout->rowCount();
    if (singleColumn)
    {
      glayout->addWidget(this->LabelWidget, row, 0, 1, -1);
      glayout->addWidget(this->PropertyWidget, (row + 1), 0, 1, -1);
    }
    else
    {
      glayout->addWidget(this->LabelWidget, row, 0, Qt::AlignVCenter | Qt::AlignLeft);
      glayout->addWidget(this->PropertyWidget, row, 1);
    }
  }
  else
  {
    glayout->addWidget(this->PropertyWidget, glayout->rowCount(), 0, 1, -1);
  }
  if (this->GroupFooter)
  {
    glayout->addWidget(this->GroupFooter, glayout->rowCount(), 0, 1, -1);
  }
}

//-----------------------------------------------------------------------------
QFrame* pqProxyWidgetItem::newHLine(QWidget* parent)
{
  QFrame* line = new QFrame(parent);
  line->setFrameShadow(QFrame::Sunken);
  line->setFrameShape(QFrame::HLine);
  line->setLineWidth(1);
  line->setMidLineWidth(0);
  return line;
}

//-----------------------------------------------------------------------------
QWidget* pqProxyWidgetItem::newGroupSeparator(QWidget* parent)
{
  QWidget* widget = new QWidget(parent);
  QVBoxLayout* vbox = new QVBoxLayout(widget);
  vbox->setContentsMargins(0, 0, 0, pqPropertiesPanel::suggestedVerticalSpacing());
  vbox->setSpacing(0);
  vbox->addWidget(newHLine(widget));
  return widget;
}
