/*=========================================================================

   Program: ParaView
   Module:  pqSplinePropertyWidget.cxx

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
#include "pqSplinePropertyWidget.h"
#include "ui_pqSplinePropertyWidget.h"

#include "pqDoubleLineEdit.h"
#include "pqPointPickingHelper.h"
#include "vtkNumberToString.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QPointer>

#include <sstream>
#include <utility>
#include <vector>
namespace
{
class SplineTableModel : public QAbstractTableModel
{
  using Superclass = QAbstractTableModel;

public:
  SplineTableModel(QObject* parentObj = nullptr)
    : Superclass(parentObj)
  {
  }

  ~SplineTableModel() override {}

  int rowCount(const QModelIndex& prnt = QModelIndex()) const override
  {
    Q_UNUSED(prnt);
    return static_cast<int>(this->Points.size());
  }

  int columnCount(const QModelIndex& /*parent*/) const override { return 3; }

  Qt::ItemFlags flags(const QModelIndex& idx) const override
  {
    return this->Superclass::flags(idx) | Qt::ItemIsEditable;
  }

  QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const override
  {
    if (role == Qt::DisplayRole)
    {
      return pqDoubleLineEdit::formatDoubleUsingGlobalPrecisionAndNotation(
        this->Points[idx.row()][idx.column()]);
    }
    else if (role == Qt::EditRole || role == Qt::ToolTipRole)
    {
      std::ostringstream str;
      str << vtkNumberToString()(this->Points[idx.row()][idx.column()]);
      return str.str().c_str();
    }

    return QVariant();
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
      switch (section)
      {
        case 0:
          return "X";
        case 1:
          return "Y";
        case 2:
          return "Z";
      }
    }
    return this->Superclass::headerData(section, orientation, role);
  }

  bool setData(const QModelIndex& idx, const QVariant& value, int role) override
  {
    Q_UNUSED(role);

    int row = idx.row();
    int col = idx.column();
    if (row >= 0 && static_cast<size_t>(row) < this->Points.size() && col >= 0 && col < 3)
    {
      this->Points[idx.row()][idx.column()] = value.toDouble();
      Q_EMIT this->dataChanged(idx, idx, QVector<int>({ Qt::DisplayRole }));
      return true;
    }

    return false;
  }

  const std::vector<vtkVector3d>& points() const { return this->Points; }

  int setPoint(int idx, double x, double y, double z)
  {
    vtkVector3d pt(x, y, z);
    auto pts = this->points();
    if (idx == -1)
    {
      idx = std::max(1, static_cast<int>(pts.size()) - 1);
    }

    if (idx >= static_cast<int>(pts.size()))
    {
      pts.resize(idx + 1, pt);
    }
    else
    {
      pts[idx] = pt;
    }
    this->setPoints(pts);
    return idx;
  }

  // returns true if something changed.
  bool setPoints(const std::vector<vtkVector3d>& pts)
  {
    if (this->Points == pts)
    {
      return false;
    }

    const int cursize = static_cast<int>(this->Points.size());
    const int newsize = static_cast<int>(pts.size());
    if (newsize > cursize)
    {
      Q_EMIT this->beginInsertRows(QModelIndex(), cursize, newsize - 1);
      this->Points.resize(newsize);
      std::copy(
        std::next(pts.begin(), cursize), pts.end(), std::next(this->Points.begin(), cursize));
      Q_EMIT this->endInsertRows();
    }
    else if (cursize > newsize)
    {
      Q_EMIT this->beginRemoveRows(QModelIndex(), newsize, cursize - 1);
      this->Points.resize(newsize);
      Q_EMIT this->endRemoveRows();
    }

    // check for data changes.
    std::pair<int, int> change_range(-1, -1);
    for (int cc = 0; cc < newsize; ++cc)
    {
      if (this->Points[cc] != pts[cc])
      {
        this->Points[cc] = pts[cc];
        if (change_range.first == -1)
        {
          change_range.first = change_range.second = cc;
        }
        else if (change_range.second != (cc - 1))
        {
          Q_EMIT this->dataChanged(this->index(change_range.first, 0),
            this->index(change_range.second, 2), QVector<int>({ Qt::DisplayRole }));
          change_range.first = change_range.second = cc;
        }
        else
        {
          change_range.second = cc;
        }
      }
    }
    if (change_range.second != -1)
    {
      Q_EMIT this->dataChanged(this->index(change_range.first, 0),
        this->index(change_range.second, 2), QVector<int>({ Qt::DisplayRole }));
    }

    return true;
  }

  size_t insertPoint(size_t loc)
  {
    std::vector<vtkVector3d> pts = this->points();
    loc = std::min(loc, pts.size());

    vtkVector3d point(0, 0, 0);
    if (loc < pts.size())
    {
      // inserting in middle
      point = (pts[loc] + pts[loc - 1]) / vtkVector3d(2.0);
    }
    else if (loc == pts.size() && pts.size() >= 2)
    {
      // adding at end.
      point = pts[loc - 1] + (pts[loc - 1] - pts[loc - 2]);
    }
    else if (pts.size() == 1)
    {
      point = pts[0] + vtkVector3d(1, 1, 1);
    }
    pts.insert(std::next(pts.begin(), loc), point);
    this->setPoints(pts);
    return loc;
  }

private:
  Q_DISABLE_COPY(SplineTableModel);
  std::vector<vtkVector3d> Points;
};
}

//-----------------------------------------------------------------------------
class pqSplinePropertyWidget::pqInternals
{
public:
  Ui::SplinePropertyWidget Ui;
  SplineTableModel Model;
  pqPropertyLinks InternalLinks;
};

//-----------------------------------------------------------------------------
pqSplinePropertyWidget::pqSplinePropertyWidget(vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup,
  pqSplinePropertyWidget::ModeTypes mode, QWidget* parentObject)
  : Superclass("representations",
      (mode == pqSplinePropertyWidget::POLYLINE ? "PolyLineWidgetRepresentation"
                                                : "SplineWidgetRepresentation"),
      smproxy, smgroup, parentObject)
  , Internals(new pqSplinePropertyWidget::pqInternals())
{
  pqInternals& internals = (*this->Internals);
  Ui::SplinePropertyWidget& ui = internals.Ui;
  ui.setupUi(this);
  ui.PointsTable->setModel(&internals.Model);
  ui.PointsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  if (vtkSMProperty* handlePositions = smgroup->GetProperty("HandlePositions"))
  {
    this->addPropertyLink(this, "points", SIGNAL(pointsChanged()), handlePositions);
  }
  else
  {
    qCritical("Missing required property for function 'HandlePositions'.");
  }

  if (vtkSMProperty* closed = smgroup->GetProperty("Closed"))
  {
    this->addPropertyLink(ui.Closed, "checked", SIGNAL(toggled(bool)), closed);
    ui.Closed->setText(closed->GetXMLLabel());
  }
  else
  {
    ui.Closed->hide();
  }

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  this->connect(ui.Add, &QAbstractButton::clicked, [&]() {
    auto cidx = ui.PointsTable->currentIndex();
    const size_t loc =
      cidx.isValid() ? static_cast<size_t>(cidx.row() + 1) : internals.Model.points().size();
    internals.Model.insertPoint(loc);
    ui.PointsTable->setCurrentIndex(internals.Model.index(static_cast<int>(loc), 0));
  });

  this->connect(ui.PointsTable, &pqExpandableTableView::editPastLastRow,
    [&]() { internals.Model.insertPoint(internals.Model.points().size()); });

  this->connect(ui.Remove, &QAbstractButton::clicked, [&]() {
    const auto selModel = ui.PointsTable->selectionModel();
    const auto& pts = internals.Model.points();
    std::vector<vtkVector3d> newpts;
    newpts.reserve(pts.size());
    for (int cc = 0; cc < static_cast<int>(pts.size()); ++cc)
    {
      if (!selModel->isRowSelected(cc, QModelIndex()))
      {
        newpts.push_back(pts[cc]);
      }
    }
    internals.Model.setPoints(newpts);
  });

  ui.Remove->setEnabled(false);
  QObject::connect(ui.PointsTable->selectionModel(), &QItemSelectionModel::selectionChanged,
    [&]() { ui.Remove->setEnabled(ui.PointsTable->selectionModel()->hasSelection()); });

  this->connect(ui.DeleteAll, &QAbstractButton::clicked, [&]() { internals.Model.setPoints({}); });

  if (auto prop = this->widgetProxy()->GetProperty("CurrentHandleIndex"))
  {
    internals.InternalLinks.addPropertyLink(
      this, "currentRow", SIGNAL(currentRowChanged()), this->widgetProxy(), prop);
    QObject::connect(ui.PointsTable->selectionModel(), &QItemSelectionModel::currentChanged, [&]() {
      Q_EMIT this->currentRowChanged();
      this->render();
    });
  }

  QObject::connect(&internals.Model, &QAbstractTableModel::dataChanged,
    [this]() { Q_EMIT this->pointsChanged(); });
  QObject::connect(&internals.Model, &QAbstractTableModel::rowsInserted,
    [this]() { Q_EMIT this->pointsChanged(); });
  QObject::connect(&internals.Model, &QAbstractTableModel::rowsRemoved,
    [this]() { Q_EMIT this->pointsChanged(); });

  // Setup picking handlers
  auto pickCurrent = [&](double x, double y, double z) {
    const auto idx = this->currentRow();
    if (idx != -1)
    {
      internals.Model.setPoint(idx, x, y, z);
    }
  };

  pqPointPickingHelper* pickHelper = new pqPointPickingHelper(QKeySequence(tr("P")), false, this);
  pickHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  QObject::connect(pickHelper, &pqPointPickingHelper::pick, pickCurrent);

  pqPointPickingHelper* pickHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+P")), true, this);
  pickHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper2->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  QObject::connect(pickHelper2, &pqPointPickingHelper::pick, pickCurrent);

  pqPointPickingHelper* pickHelper3 = new pqPointPickingHelper(QKeySequence(tr("1")), false, this);
  pickHelper3->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper3->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  QObject::connect(pickHelper3, &pqPointPickingHelper::pick, [&](double x, double y, double z) {
    this->setCurrentRow(internals.Model.setPoint(0, x, y, z));
  });

  pqPointPickingHelper* pickHelper4 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+1")), true, this);
  pickHelper4->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper4->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  QObject::connect(pickHelper4, &pqPointPickingHelper::pick, [&](double x, double y, double z) {
    this->setCurrentRow(internals.Model.setPoint(0, x, y, z));
  });

  pqPointPickingHelper* pickHelper5 = new pqPointPickingHelper(QKeySequence(tr("2")), false, this);
  pickHelper5->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper5->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  QObject::connect(pickHelper5, &pqPointPickingHelper::pick, [&](double x, double y, double z) {
    this->setCurrentRow(internals.Model.setPoint(-1, x, y, z));
  });

  pqPointPickingHelper* pickHelper6 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+2")), true, this);
  pickHelper6->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper6->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  QObject::connect(pickHelper6, &pqPointPickingHelper::pick, [&](double x, double y, double z) {
    this->setCurrentRow(internals.Model.setPoint(-1, x, y, z));
  });
}

//-----------------------------------------------------------------------------
pqSplinePropertyWidget::~pqSplinePropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqSplinePropertyWidget::setLineColor(const QColor& color)
{
  double dcolor[3];
  dcolor[0] = color.redF();
  dcolor[1] = color.greenF();
  dcolor[2] = color.blueF();

  vtkSMProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "LineColor").Set(dcolor, 3);
  wdgProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSplinePropertyWidget::placeWidget()
{
  // nothing to do.
}

//-----------------------------------------------------------------------------
void pqSplinePropertyWidget::setCurrentRow(int row)
{
  auto selModel = this->Internals->Ui.PointsTable->selectionModel();
  auto currentIdx = selModel->currentIndex();
  if (!currentIdx.isValid() || currentIdx.row() != row)
  {
    selModel->setCurrentIndex(this->Internals->Model.index(row, 0),
      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    Q_EMIT this->currentRowChanged();
  }
}

//-----------------------------------------------------------------------------
int pqSplinePropertyWidget::currentRow() const
{
  auto selModel = this->Internals->Ui.PointsTable->selectionModel();
  auto currentIdx = selModel->currentIndex();
  return (currentIdx.isValid() ? currentIdx.row() : -1);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSplinePropertyWidget::points() const
{
  auto& model = this->Internals->Model;
  auto pts = model.points();
  QList<QVariant> retval;
  retval.reserve(static_cast<int>(pts.size()));
  for (const vtkVector3d& coord : pts)
  {
    retval.push_back(QVariant(coord.GetX()));
    retval.push_back(QVariant(coord.GetY()));
    retval.push_back(QVariant(coord.GetZ()));
  }
  return retval;
}

//-----------------------------------------------------------------------------
void pqSplinePropertyWidget::setPoints(const QList<QVariant>& pts)
{
  std::vector<vtkVector3d> coords;
  coords.reserve(pts.size() / 3);
  for (int cc = 0; (cc + 2) < pts.size(); cc += 3)
  {
    coords.push_back(
      vtkVector3d(pts[cc].toDouble(), pts[cc + 1].toDouble(), pts[cc + 2].toDouble()));
  }

  auto& model = this->Internals->Model;
  if (model.setPoints(coords))
  {
    Q_EMIT this->pointsChanged();
  }
}
