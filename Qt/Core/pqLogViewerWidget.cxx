// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLogViewerWidget.h"
#include "ui_pqLogViewerWidget.h"

#include "vtkPVLogger.h"
#include "vtkSMSessionProxyManager.h"

#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqServer.h"

#include <cassert>
#include <cmath>

//-----------------------------------------------------------------------------
const int RAW_DATA_ROLE = Qt::UserRole + 1;
const int RAW_DATA_SUFFIX_ROLE = Qt::UserRole + 2;

//-----------------------------------------------------------------------------
class pqLogViewerWidget::pqInternals
{
public:
  Ui::LogViewerWidget Ui;
  QStandardItemModel Model;
  QSortFilterProxyModel FilterModel;
  QVector<QStandardItem*> ActiveScopeItem;
  QStandardItem* LastItem = nullptr;

  void reset()
  {
    this->Model.clear();
    this->ActiveScopeItem.clear();
    this->LastItem = nullptr;
    this->Ui.details->setText(QCoreApplication::translate("pqLogViewerWidget", ""));
  }

  void addLines(const QStringList& lines)
  {
    QRegularExpression scopeBegin(R"==(^\s*{ (?<label>.*))==");
    QRegularExpression scopeEnd(R"==(^\s*} (?<time>[^:]+):.*)==");
    for (const auto& line : lines)
    {
      bool is_raw_log;
      auto parts = extractLogParts(line, is_raw_log);
      if (!is_raw_log)
      {
        if (this->Model.columnCount() == 0)
        {
          // got header!
          this->Model.setColumnCount(5);
          this->Model.setHeaderData(0, Qt::Horizontal, "Message");
          this->Model.setHeaderData(1, Qt::Horizontal, "Process");
          this->Model.setHeaderData(2, Qt::Horizontal, "File:line");
          this->Model.setHeaderData(3, Qt::Horizontal, "Verbosity");
          this->Model.setHeaderData(4, Qt::Horizontal, "Time");
          this->Ui.treeView->header()->moveSection(0, 4);
          if (parts[0] == "uptime")
          {
            continue;
          }
        }

        auto ematch = scopeEnd.match(parts[4]);
        if (ematch.hasMatch())
        {
          assert(this->ActiveScopeItem.isEmpty() == false);
          auto litem = this->ActiveScopeItem.last();
          auto txt = litem->data(Qt::DisplayRole).toString();
          litem->setData(txt + " - " + ematch.captured("time"), Qt::DisplayRole);
          litem->setData(line, RAW_DATA_SUFFIX_ROLE);
          this->ActiveScopeItem.pop_back();
          this->LastItem = nullptr;
          continue;
        }

        auto item0 = new QStandardItem(parts[0]);
        auto item1 = new QStandardItem(parts[1]);
        auto item2 = new QStandardItem(parts[2]);
        auto item3 = new QStandardItem(parts[3]);
        auto item4 = new QStandardItem(parts[4]);
        const int height = this->Ui.treeView->fontMetrics().boundingRect("(").height();
        item4->setData(QSize(0, height * 1.50), Qt::SizeHintRole);
        item4->setData(line, RAW_DATA_ROLE);
        // item4->setData(QVariant(Qt::AlignLeft|Qt::AlignTop),
        // Qt::TextAlignmentRole);

        // Change log message color for warnings and errors
        QColor color = QApplication::palette().color(QPalette::Text);
        if (parts[3] == "WARN")
        {
          color.setRgb(174, 173, 39);
        }
        else if (parts[3] == "ERR")
        {
          color.setRgb(194, 54, 33);
        }
        QBrush brush;
        brush.setColor(color);
        item0->setForeground(brush);
        item1->setForeground(brush);
        item2->setForeground(brush);
        item3->setForeground(brush);
        item4->setForeground(brush);

        auto parent = !this->ActiveScopeItem.isEmpty() ? this->ActiveScopeItem.last()
                                                       : this->Model.invisibleRootItem();
        parent->appendRow(QList<QStandardItem*>{ item4, item1, item2, item3, item0 });
        this->LastItem = item4;

        auto smatch = scopeBegin.match(parts[4]);
        if (smatch.hasMatch())
        {
          item4->setData(smatch.captured("label"), Qt::DisplayRole);
          this->ActiveScopeItem.push_back(item4);
          this->LastItem = nullptr;
        }
      }
      else
      {
        if (this->LastItem != nullptr)
        {
          QString txt = this->LastItem->data(Qt::DisplayRole).toString();
          txt = txt + " " + parts[4];
          this->LastItem->setData(txt, Qt::DisplayRole);
          this->LastItem->setData(
            this->LastItem->data(RAW_DATA_ROLE).toString() + "\n" + line, RAW_DATA_ROLE);
        }
        else
        {
          // TODO: not entirely sure how to handle preamble. Ignoring for now.
        }
      }
    }
  }

  QString rawText(const QModelIndex& idx) const
  {
    if (!idx.isValid())
    {
      return QString();
    }

    const QModelIndex realIndex = this->FilterModel.index(idx.row(), 0, idx.parent());

    QString resultText;
    QTextStream stream(&resultText);
    stream << this->FilterModel.data(realIndex, RAW_DATA_ROLE).toString();
    if (this->FilterModel.hasChildren(realIndex))
    {
      for (int cc = 0, max = this->FilterModel.rowCount(realIndex); cc < max; ++cc)
      {
        stream << "\n" << this->rawText(this->FilterModel.index(cc, 0, realIndex));
      }
    }
    auto suffix = this->FilterModel.data(realIndex, RAW_DATA_SUFFIX_ROLE);
    if (suffix.isValid())
    {
      stream << "\n" << suffix.toString();
    }
    return resultText;
  }
};

//-----------------------------------------------------------------------------
pqLogViewerWidget::pqLogViewerWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqLogViewerWidget::pqInternals())
{
  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);
  internals.Ui.splitter->setSizes(QList<int>{ 800, 200 });
  internals.FilterModel.setRecursiveFilteringEnabled(true);
  internals.FilterModel.setSourceModel(&internals.Model);
  internals.Ui.treeView->setModel(&internals.FilterModel);

  QObject::connect(internals.Ui.treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
    [&internals](const QItemSelection&, const QItemSelection&)
    {
      auto indexes = internals.Ui.treeView->selectionModel()->selectedRows();
      QString detailText;
      for (auto index : indexes)
      {
        // Check if the parent of this item is already in the list. Skip if so.
        if (!(index.parent().isValid() && indexes.contains(index.parent())))
        {
          detailText.append(internals.rawText(index));
          detailText.append("\n");
        }
      }
      const char* detailTextStr = detailText.toUtf8().data();
      internals.Ui.details->setText(
        QCoreApplication::translate("pqLogViewerWidget", detailTextStr));
    });

  QObject::connect(internals.Ui.treeView->verticalScrollBar(), &QScrollBar::valueChanged,
    [&internals, this]()
    {
      if (this->signalsBlocked())
      {
        return;
      }
      auto modelIndex = internals.Ui.treeView->indexAt(internals.Ui.treeView->rect().topLeft());
      if (modelIndex.isValid())
      {
        auto item = internals.Model.item(modelIndex.row(), 4);
        if (item)
        {
          double time = item->data(Qt::DisplayRole).toString().replace('s', '0').toDouble();
          this->scrolled(time);
        }
      }
    });

  QObject::connect(internals.Ui.filter, &QLineEdit::textChanged,
    [&internals](const QString& txt) { internals.FilterModel.setFilterWildcard(txt); });
  QObject::connect(
    internals.Ui.advancedButton, &QToolButton::clicked, this, &pqLogViewerWidget::toggleAdvanced);
  internals.reset();

  QObject::connect(
    internals.Ui.exportLogButton, &QPushButton::clicked, this, &pqLogViewerWidget::exportLog);
}

//-----------------------------------------------------------------------------
pqLogViewerWidget::~pqLogViewerWidget() = default;

//-----------------------------------------------------------------------------
void pqLogViewerWidget::setLog(const QString& text)
{
  auto& internals = (*this->Internals);
  internals.reset();
  this->appendLog(text);

  this->updateColumnVisibilities();
}

//-----------------------------------------------------------------------------
void pqLogViewerWidget::appendLog(const QString& text)
{
  auto& internals = (*this->Internals);
  auto lines = text.split('\n'); // TODO: handle '\r'?
  internals.addLines(lines);
}

//-----------------------------------------------------------------------------
void pqLogViewerWidget::setFilterWildcard(QString wildcard)
{
  auto& internals = (*this->Internals);
  internals.FilterModel.setFilterWildcard(wildcard);
}

//-----------------------------------------------------------------------------
void pqLogViewerWidget::scrollToTime(double time)
{
  auto& internals = (*this->Internals);
  bool block = this->blockSignals(true);
  auto modelIndex = internals.Ui.treeView->indexAt(internals.Ui.treeView->rect().topLeft());
  int prevDirection = 0;
  while (modelIndex.isValid())
  {
    double itemTime = internals.Model.item(modelIndex.row(), 4)
                        ->data(Qt::DisplayRole)
                        .toString()
                        .replace('s', '0')
                        .toDouble();
    if (fabs(itemTime - time) < 1e-1)
    {
      internals.Ui.treeView->scrollTo(modelIndex, QAbstractItemView::PositionAtTop);
      break;
    }
    int direction = itemTime < time ? 1 : -1;
    if (!prevDirection)
    {
      prevDirection = direction;
    }
    else if (prevDirection != direction)
    {
      internals.Ui.treeView->scrollTo(modelIndex, QAbstractItemView::PositionAtTop);
      break;
    }
    modelIndex = itemTime < time ? internals.Ui.treeView->indexBelow(modelIndex)
                                 : internals.Ui.treeView->indexAbove(modelIndex);
  }
  this->blockSignals(block);
}

//-----------------------------------------------------------------------------
QVector<QString> pqLogViewerWidget::extractLogParts(const QString& txt, bool& is_raw)
{
  QVector<QString> parts{ 5 };
  QRegularExpression re(
    R"==(\(\s*(?<time>\S+)\s*\) \[(?<tid>.+)\]\s*(?<fname>\S+)\s+(?<v>\S+)\s*\| (\.\s+)*(?<txt>.*))==");
  auto match = re.match(txt);
  if (match.hasMatch())
  {
    is_raw = false;
    parts[0] = match.captured("time");
    parts[1] = match.captured("tid");
    parts[2] = match.captured("fname");
    parts[3] = match.captured("v");
    parts[4] = match.captured("txt");
  }
  else
  {
    is_raw = true;
    parts[4] = txt;
  }
  return parts;
}

//-----------------------------------------------------------------------------
void pqLogViewerWidget::toggleAdvanced()
{
  this->Advanced = !this->Advanced;
  this->updateColumnVisibilities();
}

//-----------------------------------------------------------------------------
void pqLogViewerWidget::exportLog()
{
  QString text = this->Internals->Ui.details->toPlainText();
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  pqFileDialog fileDialog(server, pqCoreUtilities::mainWidget(), tr("Save log"), QString(),
    tr("Text Files") + " (*.txt);;" + tr("All Files") + " (*)", false, false);
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() != pqFileDialog::Accepted)
  {
    // Canceled
    return;
  }

  QString filename = fileDialog.getSelectedFiles()[0];
  QByteArray filename_ba = filename.toUtf8();
  vtkTypeUInt32 location = fileDialog.getSelectedLocation();
  auto pxm = server->proxyManager();
  if (!pxm->SaveString(text.toStdString().c_str(), filename_ba.data(), location))
  {
    qCritical() << tr("Failed to save log to ") << filename;
  }
}

//-----------------------------------------------------------------------------
void pqLogViewerWidget::updateColumnVisibilities()
{
  auto& internals = (*this->Internals);
  internals.Ui.treeView->setColumnHidden(1, !this->Advanced);
  internals.Ui.treeView->setColumnHidden(2, !this->Advanced);
}
