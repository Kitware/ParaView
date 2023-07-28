// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqArrayListWidget.h"

#include "pqAnnotationsModel.h"
#include "pqExpandableTableView.h"
#include "pqHeaderView.h"

#include <QEvent>
#include <QScopedValueRollback>
#include <QVBoxLayout>
#include <QtCore>

class pqArrayListModel : public pqAnnotationsModel
{
  // Enable the use of the tr() method in a class without Q_OBJECT macro
  Q_DECLARE_TR_FUNCTIONS(pqArrayListModel)
  typedef pqAnnotationsModel Superclass;

public:
  pqArrayListModel(pqArrayListWidget* parent = nullptr)
    : Superclass(parent)
    , ParentWidget(parent)
  {
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
      switch (section)
      {
        case VALUE:
          return this->ArrayLabel;
        case LABEL:
          return tr("New Name");
        default:
          break;
      }
    }

    if (role == Qt::DecorationRole && section == pqAnnotationsModel::VALUE)
    {
      if (this->IconType == "point")
      {
        return QIcon(":/pqWidgets/Icons/pqNodalData.svg");
      }
      else if (this->IconType == "cell")
      {
        return QIcon(":/pqWidgets/Icons/pqCellCenterData.svg");
      }
      else if (this->IconType == "field")
      {
        return QIcon(":/pqWidgets/Icons/pqGlobalData.svg");
      }
      else if (this->IconType == "vertex")
      {
        return QIcon(":/pqWidgets/Icons/pqNodalData.svg");
      }
      else if (this->IconType == "edge")
      {
        return QIcon(":/pqWidgets/Icons/pqEdgeCenterData.svg");
      }
      else if (this->IconType == "face")
      {
        return QIcon(":/pqWidgets/Icons/pqFaceCenterData.svg");
      }
      else if (this->IconType == "row")
      {
        return QIcon(":/pqWidgets/Icons/pqSpreadsheet.svg");
      }
    }

    return this->Superclass::headerData(section, orientation, role);
  }

  Qt::ItemFlags flags(const QModelIndex& idx) const override
  {
    auto value = this->Superclass::flags(idx);
    if (idx.column() == VALUE)
    {
      return value & ~Qt::ItemIsEditable;
    }

    return value;
  }

  QString ArrayLabel = tr("Array Name");
  QString IconType = "";
  pqArrayListWidget* ParentWidget = nullptr;
};

pqArrayListWidget::pqArrayListWidget(QWidget* parent)
  : Superclass(parent)
{
  auto lay = new QVBoxLayout(this);
  this->TableView = new pqExpandableTableView(this);
  lay->addWidget(this->TableView);
  lay->setContentsMargins(0, 0, 0, 0);
  lay->setSpacing(2);

  pqHeaderView* myheader = new pqHeaderView(Qt::Horizontal, this->TableView);
  this->TableView->setHorizontalHeader(myheader);
  myheader->setHighlightSections(false);
  myheader->setSectionResizeMode(QHeaderView::ResizeToContents);
  myheader->setStretchLastSection(true);

  this->Model = new pqArrayListModel(this);
  this->Model->setSupportsReorder(true);

  this->TableView->setModel(this->Model);
  this->TableView->setSortingEnabled(true);
  this->TableView->setColumnHidden(pqAnnotationsModel::COLOR, true);
  this->TableView->setColumnHidden(pqAnnotationsModel::OPACITY, true);
  this->TableView->setColumnHidden(pqAnnotationsModel::VISIBILITY, true);

  this->connect(this->TableView, SIGNAL(widgetModified), SLOT(updateProperty()));
  QObject::connect(
    this->Model, &pqArrayListModel::dataChanged, this, &pqArrayListWidget::updateProperty);
}

//-----------------------------------------------------------------------------
void pqArrayListWidget::setMaximumRowCountBeforeScrolling(int size)
{
  this->TableView->setMaximumRowCountBeforeScrolling(size);
}

//-----------------------------------------------------------------------------
bool pqArrayListWidget::event(QEvent* evt)
{
  if (evt->type() == QEvent::DynamicPropertyChange && !this->UpdatingProperty)
  {
    auto devt = dynamic_cast<QDynamicPropertyChangeEvent*>(evt);
    this->propertyChanged(devt->propertyName());
    return true;
  }

  return this->Superclass::event(evt);
}

//-----------------------------------------------------------------------------
void pqArrayListWidget::setHeaderLabel(const QString& label)
{
  this->Model->ArrayLabel = label;
}

//-----------------------------------------------------------------------------
void pqArrayListWidget::setIconType(const QString& icon_type)
{
  this->Model->IconType = icon_type;
}

//-----------------------------------------------------------------------------
void pqArrayListWidget::propertyChanged(const QString& pname)
{
  if (this->UpdatingProperty)
  {
    return;
  }

  QVariant property = this->property(pname.toUtf8().data());
  const QList<QVariant> status_values = property.value<QList<QVariant>>();
  std::vector<std::pair<QString, QString>> annotations;
  for (auto idx = 0; idx < status_values.size() - 1; idx += 2)
  {
    annotations.emplace_back(status_values[idx].toString(), status_values[idx + 1].toString());
  }

  this->Model->setAnnotations(annotations, true);
}

//-----------------------------------------------------------------------------
void pqArrayListWidget::updateProperty()
{
  // for scope
  {
    QScopedValueRollback<bool> rollback(this->UpdatingProperty, true);
    QList<QVariant> values;
    std::vector<std::pair<QString, QString>> annotations = this->Model->annotations();
    for (auto annotation : annotations)
    {
      values.push_back(annotation.first);
      values.push_back(annotation.second);
    }

    this->setProperty(this->Model->ArrayLabel.toLatin1(), QVariant::fromValue(values));
  }

  Q_EMIT this->widgetModified();
}
