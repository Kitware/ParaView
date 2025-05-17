// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqQuickLaunchDialogExtended.h"

#include "ui_pqQuickLaunchDialogExtended.h"

#include "pqExtendedSortFilterProxyModel.h"
#include "pqHelpReaction.h"
#include "pqKeyEventFilter.h"
#include "pqProxyActionListModel.h"
#include "pqQtConfig.h"

#include <QAction>
#include <QItemSelectionModel>

//-----------------------------------------------------------------------------
pqQuickLaunchDialogExtended::pqQuickLaunchDialogExtended(
  QWidget* parent, const QList<QAction*>& actions)
  : Superclass(parent, Qt::Dialog | Qt::FramelessWindowHint)
  , Model(new pqProxyActionListModel(actions))
  , AvailableSortFilterModel(new pqExtendedSortFilterProxyModel(this))
  , DisabledSortFilterModel(new pqExtendedSortFilterProxyModel(this))
  , Ui(new Ui::QuickLaunchDialogExtended)
{
  this->Ui->setupUi(this);
  QIcon warningIcon = this->style()->standardIcon(QStyle::SP_MessageBoxWarning);
  QSize iconSize = this->Ui->GoToHelp->size();
  this->Ui->RequirementIcon->setPixmap(warningIcon.pixmap(iconSize));

  this->AvailableSortFilterModel->setSourceModel(this->Model.get());
  this->DisabledSortFilterModel->setSourceModel(this->Model.get());
  this->Ui->AvailableProxies->setModel(this->AvailableSortFilterModel.get());
  this->Ui->DisabledProxies->setModel(this->DisabledSortFilterModel.get());

  this->Ui->AvailableProxies->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->Ui->DisabledProxies->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->AvailableSortFilterModel->setDynamicSortFilter(false);
  this->DisabledSortFilterModel->setDynamicSortFilter(false);

  this->AvailableSortFilterModel->setExclusion(pqProxyActionListModel::ProxyEnabledRole, false);
  this->DisabledSortFilterModel->setExclusion(pqProxyActionListModel::ProxyEnabledRole, true);

  auto keyEventFilter = new pqKeyEventFilter(this);
  keyEventFilter->filter(this);
  keyEventFilter->forwardType(this->Ui->Request, pqKeyEventFilter::TextInput);
  keyEventFilter->filter(this->Ui->AvailableProxies);
  keyEventFilter->filter(this->Ui->DisabledProxies);

  QObject::connect(keyEventFilter, &pqKeyEventFilter::accepted, this,
    &pqQuickLaunchDialogExtended::createCurrentProxy);
  QObject::connect(
    keyEventFilter, &pqKeyEventFilter::rejected, this, &pqQuickLaunchDialogExtended::reject);
  QObject::connect(keyEventFilter, &pqKeyEventFilter::textChanged, this,
    &pqQuickLaunchDialogExtended::handleTextChanged);
  QObject::connect(
    keyEventFilter, &pqKeyEventFilter::motion, this, &pqQuickLaunchDialogExtended::move);
  QObject::connect(keyEventFilter, &pqKeyEventFilter::focusChanged, this,
    &pqQuickLaunchDialogExtended::toggleFocus);

  QObject::connect(
    this->Ui->Request, &QLineEdit::textChanged, this, &pqQuickLaunchDialogExtended::requestChanged);
  QObject::connect(this->Ui->CreateProxy, &QPushButton::released, this,
    &pqQuickLaunchDialogExtended::createCurrentProxyWithoutApply);
  QObject::connect(this->Ui->AvailableProxies, &QListView::doubleClicked, this,
    &pqQuickLaunchDialogExtended::createCurrentProxyWithoutApply);
  QObject::connect(
    this->Ui->GoToHelp, &QPushButton::released, this, &pqQuickLaunchDialogExtended::showProxyHelp);

  QItemSelectionModel* availableSelection = this->Ui->AvailableProxies->selectionModel();
  QItemSelectionModel* disabledSelection = this->Ui->DisabledProxies->selectionModel();

  QObject::connect(availableSelection, &QItemSelectionModel::currentChanged,
    [&](const QModelIndex& current, const QModelIndex& previous)
    {
      QItemSelectionModel* disabled = this->Ui->DisabledProxies->selectionModel();
      if (!previous.isValid() && current.isValid() && disabled->hasSelection())
      {
        disabled->clear();
      }
      this->currentChanged(current, this->AvailableSortFilterModel.get());
    });

  QObject::connect(disabledSelection, &QItemSelectionModel::currentChanged,
    [&](const QModelIndex& current, const QModelIndex& previous)
    {
      QItemSelectionModel* available = this->Ui->AvailableProxies->selectionModel();
      if (!previous.isValid() && current.isValid() && available->hasSelection())
      {
        available->clear();
      }
      this->currentChanged(current, this->DisabledSortFilterModel.get());
    });

  this->requestChanged("");
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialogExtended::requestChanged(const QString& request)
{
  this->AvailableSortFilterModel->setRequest(request);
  this->DisabledSortFilterModel->setRequest(request);

  this->AvailableSortFilterModel->setRequest(pqProxyActionListModel::DocumentationRole, request);
  this->DisabledSortFilterModel->setRequest(pqProxyActionListModel::DocumentationRole, request);

  this->AvailableSortFilterModel->update();
  this->DisabledSortFilterModel->update();

  QListView* current = this->currentList();
  if (this->Ui->AvailableProxies->model()->rowCount() > 0)
  {
    current = this->Ui->AvailableProxies;
  }
  this->makeCurrent(current, 0);

  if (current->model()->rowCount() == 0)
  {
    this->toggleFocus();
  }
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialogExtended::createCurrentProxyWithoutApply()
{
  this->createCurrentProxy(false);
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialogExtended::createCurrentProxy(bool autoApply)
{
  auto current = this->Ui->AvailableProxies->currentIndex();
  QModelIndex modelCurrent = this->AvailableSortFilterModel->mapToSource(current);
  if (!modelCurrent.isValid())
  {
    return;
  }

  QVariant actionVar = this->Model->data(modelCurrent, pqProxyActionListModel::ActionRole);
  QAction* action = actionVar.value<QAction*>();
  if (action)
  {
    action->trigger();
    this->accept();
    if (autoApply)
    {
      Q_EMIT this->applyRequested();
    }
  }
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialogExtended::currentChanged(
  const QModelIndex& currentIndex, QSortFilterProxyModel* proxyModel)
{
  QModelIndex modelCurrent = proxyModel->mapToSource(currentIndex);
  if (!modelCurrent.isValid())
  {
    return;
  }

  QVariant name = this->Model->data(modelCurrent, Qt::DisplayRole);
  QVariant doc = this->Model->data(modelCurrent, pqProxyActionListModel::DocumentationRole);
  QVariant req = this->Model->data(modelCurrent, pqProxyActionListModel::RequirementRole);

  QString currentName = name.toString();

  this->Ui->CreateProxy->setEnabled(proxyModel == this->AvailableSortFilterModel.get());
  this->Ui->CreateProxy->setText(currentName);
  this->Ui->ShortHelp->setText(doc.toString());
  this->Ui->Requirements->setText(req.toString());
  this->Ui->RequirementIcon->setVisible(!req.toString().isEmpty());
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialogExtended::cancel()
{
  if (this->Ui->Request->text().isEmpty())
  {
    this->reject();
  }
  else
  {
    this->Ui->Request->clear();
  }
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialogExtended::showProxyHelp()
{
#ifdef PARAVIEW_USE_QTHELP
  QListView* list = this->currentList();
  const QModelIndex& viewCurrent = list->currentIndex();
  QVariant group = viewCurrent.data(pqProxyActionListModel::GroupRole);
  QVariant name = viewCurrent.data(pqProxyActionListModel::NameRole);

  this->reject();
  pqHelpReaction::showProxyHelp(group.toString(), name.toString());
#endif
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialogExtended::makeCurrent(QListView* view, int row)
{
  if (view->model()->rowCount() <= row)
  {
    return;
  }

  const QModelIndex& current = view->model()->index(row, 0);
  view->setCurrentIndex(current);

  QListView* other =
    this->Ui->AvailableProxies == view ? this->Ui->DisabledProxies : this->Ui->AvailableProxies;
  other->selectionModel()->clear();
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialogExtended::toggleFocus()
{
  auto toFocus = this->currentList();
  if (toFocus == this->Ui->AvailableProxies)
  {
    toFocus = this->Ui->DisabledProxies;
  }
  else
  {
    toFocus = this->Ui->AvailableProxies;
  }

  this->makeCurrent(toFocus, 0);
}

//-----------------------------------------------------------------------------
QListView* pqQuickLaunchDialogExtended::currentList()
{
  QItemSelectionModel* disabledModel = this->Ui->DisabledProxies->selectionModel();
  if (disabledModel->hasSelection())
  {
    return this->Ui->DisabledProxies;
  }

  return this->Ui->AvailableProxies;
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialogExtended::move(int key)
{
  QListView* list = this->currentList();
  auto currentIndex = list->currentIndex();
  const int current = currentIndex.row();
  const int max = list->model()->rowCount() - 1;
  int target = current;
  if (key == Qt::Key_Up)
  {
    target--;
  }
  else if (key == Qt::Key_Down)
  {
    target++;
  }
  else if (key == Qt::Key_PageUp)
  {
    target -= 10;
  }
  else if (key == Qt::Key_PageDown)
  {
    target += 10;
  }
  else if (key == Qt::Key_Home)
  {
    target = 0;
  }
  else if (key == Qt::Key_End)
  {
    target = max;
  }

  target = std::max(0, target);
  target = std::min(target, max);

  this->makeCurrent(list, target);
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialogExtended::handleTextChanged(int key)
{
  QString text = this->Ui->Request->text();
  QKeySequence sequence(key);
  text += sequence.toString().toLower();
  this->Ui->Request->setFocus();
  this->Ui->Request->setText(text);
}

//-----------------------------------------------------------------------------
pqQuickLaunchDialogExtended::~pqQuickLaunchDialogExtended() = default;
