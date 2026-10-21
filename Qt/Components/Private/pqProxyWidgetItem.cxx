// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxyWidgetItem.h"

#include "pqLabel.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqProxyPropertyWidget.h"
#include "pqProxyWidget.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QGridLayout>
#include <QLabel>
#include <QString>
#include <QToolButton>

namespace
{
//-----------------------------------------------------------------------------
void SetupCollapseButton(
  QToolButton* collapseButton, pqLabel* groupLabel, pqProxyWidgetItem* item, bool collapseByDefault)
{
  auto updateCollapseState = [item, collapseButton](bool collapsed)
  {
    item->setCollapsed(collapsed);
    collapseButton->setArrowType(collapsed ? Qt::RightArrow : Qt::DownArrow);
    item->collapse();
  };

  if (collapseButton)
  {
    QObject::connect(collapseButton, &QToolButton::toggled, item, updateCollapseState);
    collapseButton->setChecked(collapseByDefault);
    updateCollapseState(collapseByDefault);
  }

  // Make the label also clickable
  if (groupLabel && collapseButton)
  {
    QObject::connect(
      groupLabel, &pqLabel::clicked, item, [collapseButton]() { collapseButton->toggle(); });
  }
}
};

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
  return newGroupItem(widget, label, showSeparators, advanced, searchTags, parentObj, false);
}

//-----------------------------------------------------------------------------
pqProxyWidgetItem* pqProxyWidgetItem::newGroupItem(pqPropertyWidget* widget, const QString& label,
  bool showSeparators, bool advanced, const QStringList& searchTags, pqProxyWidget* parentObj,
  bool collapsedByDefault)
{
  if (widget->isSingleRowItem())
  {
    pqProxyWidgetItem* item = newItem(widget, label, advanced, false, searchTags, parentObj);
    item->Group = true;
    return item;
  }

  pqProxyWidgetItem* item = newItem(widget, QString(), advanced, false, searchTags, parentObj);
  item->Group = true;
  item->GroupTag = label;

  QToolButton* collapseButton = nullptr;
  pqLabel* titleLabel = nullptr;

  if (!label.isEmpty() && widget->showLabel() && showSeparators)
  {
    item->GroupHeader = pqProxyWidget::newGroupLabelWidget(
      label, widget->parentWidget(), &titleLabel, &collapseButton);

    item->CollapseButton = collapseButton;
    ::SetupCollapseButton(collapseButton, titleLabel, item, collapsedByDefault);
    item->GroupFooter = newGroupSeparator(widget->parentWidget());
  }
  return item;
}

//-----------------------------------------------------------------------------
pqProxyWidgetItem* pqProxyWidgetItem::newMultiItemGroupItem(const QString& groupLabel,
  pqPropertyWidget* widget, const QString& widgetLabel, bool showSeparators, bool advanced,
  bool informationOnly, const QStringList& searchTags, pqProxyWidget* parentObj)
{
  return newMultiItemGroupItem(groupLabel, widget, widgetLabel, showSeparators, advanced,
    informationOnly, searchTags, parentObj, false);
}

//-----------------------------------------------------------------------------
pqProxyWidgetItem* pqProxyWidgetItem::newMultiItemGroupItem(const QString& groupLabel,
  pqPropertyWidget* widget, const QString& widgetLabel, bool showSeparators, bool advanced,
  bool informationOnly, const QStringList& searchTags, pqProxyWidget* parentObj, bool collapse)
{
  pqProxyWidgetItem* item =
    newItem(widget, widgetLabel, advanced, informationOnly, searchTags, parentObj);
  item->Group = true;
  // ensure GroupTag is not nullptr for multi-property groups.
  item->GroupTag = groupLabel.isNull() ? QString("") : groupLabel;
  if (!groupLabel.isEmpty() && showSeparators)
  {
    QToolButton* collapseButton = nullptr;
    pqLabel* titleLabel = nullptr;
    item->GroupHeader = pqProxyWidget::newGroupLabelWidget(
      groupLabel, widget->parentWidget(), &titleLabel, &collapseButton);

    item->CollapseButton = collapseButton;
    ::SetupCollapseButton(collapseButton, titleLabel, item, collapse);
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
void pqProxyWidgetItem::setCollapsed(bool collapsed)
{
  this->Collapsed = collapsed;
}

//-----------------------------------------------------------------------------
bool pqProxyWidgetItem::canShowWidget(
  bool showAdvanced, const QString& filterText, vtkSMProxy* proxy) const
{
  if (showAdvanced == false && this->isAdvanced(proxy) == true)
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
    if (decorator && !decorator->canShowWidget(showAdvanced))
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
  const pqProxyWidgetItem* prevVisibleItem, bool enabled, bool showAdvanced, bool forceUncollapsed)
{
  // If `this` is not a group, but previous item was and is not collapsed, we need to ensure the
  // previous items group footer is visible.
  if (!this->Group && prevVisibleItem && prevVisibleItem->Group && prevVisibleItem->GroupFooter &&
    (!prevVisibleItem->Collapsed || forceUncollapsed))
  {
    prevVisibleItem->GroupFooter->show();
  }

  // If `this` is a group, and belongs to different group than
  // prevVisibleItem then we need to show `this`'s header.
  if (this->GroupHeader)
  {
    this->GroupHeader->setVisible(this->GroupTag.isNull() || prevVisibleItem == nullptr ||
      this->GroupTag != prevVisibleItem->GroupTag);

    // If the previous item is part of the same group, we must copy its collapse state
    if (prevVisibleItem != nullptr && this->GroupTag == prevVisibleItem->GroupTag)
    {
      this->Collapsed = prevVisibleItem->Collapsed;
    }
  }

  this->PropertyWidget->updateWidget(showAdvanced);
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

  if (this->CollapseButton)
  {
    this->CollapseButton->setArrowType(
      this->Collapsed && !forceUncollapsed ? Qt::RightArrow : Qt::DownArrow);
  }

  // Hide collapse elements that belong to a group
  bool visible = !this->GroupHeader || !this->Collapsed || forceUncollapsed;
  this->PropertyWidget->setVisible(visible);
  if (this->LabelWidget)
  {
    this->LabelWidget->setVisible(visible);
  }

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
