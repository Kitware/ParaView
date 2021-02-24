/*=========================================================================

   Program: ParaView
   Module:  pqLogViewerWidget.cxx

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
#include "pqLogViewerWidget.h"
#include "ui_pqLogViewerWidget.h"

#include "vtkPVLogger.h"

#include <QByteArray>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QString>
#include <QTextStream>

#include "pqCoreUtilities.h"
#include "pqFileDialog.h"

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
    this->Ui.details->setText(tr(""));
  }

  void addLines(const QVector<QStringRef>& lines)
  {
    /* clang-format off */
    QRegularExpression scopeBegin(R"==(^\s*{ (?<label>.*))==");
    QRegularExpression scopeEnd(R"==(^\s*} (?<time>[^:]+):.*)==");
    /* clang-format on */
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
          litem->setData(line.toString(), RAW_DATA_SUFFIX_ROLE);
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
        item4->setData(line.toString(), RAW_DATA_ROLE);
        // item4->setData(QVariant(Qt::AlignLeft|Qt::AlignTop),
        // Qt::TextAlignmentRole);

        // Change log message color for warnings and errors
        QColor color;
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
    [&internals](const QItemSelection&, const QItemSelection&) {
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
      internals.Ui.details->setText(detailText);
    });

  QObject::connect(
    internals.Ui.treeView->verticalScrollBar(), &QScrollBar::valueChanged, [&internals, this]() {
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
  auto lines = text.splitRef('\n'); // TODO: handle '\r'?
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
QVector<QString> pqLogViewerWidget::extractLogParts(const QStringRef& txt, bool& is_raw)
{
  QVector<QString> parts{ 5 };
  /* clang-format off */
  QRegularExpression re(
    R"==(\(\s*(?<time>\S+)\s*\) \[(?<tid>.+)\]\s*(?<fname>\S+)\s+(?<v>\S+)\s*\| (\.\s+)*(?<txt>.*))==");
  /* clang-format on */
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
    parts[4] = txt.toString();
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
  pqFileDialog fileDialog(nullptr, pqCoreUtilities::mainWidget(), "Save log", QString(),
    "Text Files (*.txt);;All Files (*)");
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() != pqFileDialog::Accepted)
  {
    // Canceled
    return;
  }

  QString filename = fileDialog.getSelectedFiles().first();
  QByteArray filename_ba = filename.toLocal8Bit();
  std::ofstream fileStream;
  fileStream.open(filename_ba.data());
  if (fileStream.is_open())
  {
    fileStream << text.toStdString();
    fileStream.close();
  }
}

//-----------------------------------------------------------------------------
void pqLogViewerWidget::updateColumnVisibilities()
{
  auto& internals = (*this->Internals);
  internals.Ui.treeView->setColumnHidden(1, !this->Advanced);
  internals.Ui.treeView->setColumnHidden(2, !this->Advanced);
}
