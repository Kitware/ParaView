/*=========================================================================

   Program: ParaView
   Module:  pqMultiBlockInspectorWidget.cxx

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
#include "pqMultiBlockInspectorWidget.h"
#include "ui_pqMultiBlockInspectorWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCompositeDataInformationTreeModel.h"
#include "pqDataRepresentation.h"
#include "pqDoubleRangeDialog.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "pqTreeViewExpandState.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkPVLogger.h"
#include "vtkSMDoubleMapProperty.h"
#include "vtkSMDoubleMapPropertyIterator.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"

#include <QColorDialog>
#include <QIdentityProxyModel>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QStyle>

#include <cassert>

namespace
{
//-----------------------------------------------------------------------------
/**
 * Creates a pixmap for color.
 */
//-----------------------------------------------------------------------------
QPixmap ColorPixmap(int iconSize, const QColor& color, bool inherited)
{
  QPixmap pixmap(iconSize, iconSize);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);
  if (color.isValid())
  {
    QPen pen = painter.pen();
    pen.setColor(Qt::black);
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    painter.setPen(pen);

    QBrush brush = painter.brush();
    brush.setColor(color);
    brush.setStyle(inherited ? Qt::Dense5Pattern : Qt::SolidPattern);
    painter.setBrush(brush);
  }
  else
  {
    QPen pen = painter.pen();
    pen.setColor(Qt::black);
    pen.setStyle(Qt::DashLine);
    pen.setWidth(1);
    painter.setPen(pen);

    painter.setBrush(Qt::NoBrush);
  }
  const int radius = 3 * iconSize / 8;
  painter.drawEllipse(QPoint(iconSize / 2, iconSize / 2), radius, radius);
  return pixmap;
}

//-----------------------------------------------------------------------------
/**
 * Creates a pixmap for opacity.
 */
//-----------------------------------------------------------------------------
QPixmap OpacityPixmap(int iconSize, double opacity, bool inherited)
{
  QPixmap pixmap(iconSize, iconSize);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);

  if (opacity >= 0.0 && opacity <= 1.0)
  {
    QPen pen = painter.pen();
    pen.setColor(Qt::black);
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    painter.setPen(pen);

    QBrush brush = painter.brush();
    brush.setColor(Qt::gray);
    brush.setStyle(inherited ? Qt::Dense7Pattern : Qt::SolidPattern);
    painter.setBrush(brush);
  }
  else
  {
    QPen pen = painter.pen();
    pen.setColor(Qt::black);
    pen.setStyle(Qt::DashLine);
    pen.setWidth(1);
    painter.setPen(pen);

    painter.setBrush(Qt::NoBrush);
    opacity = 1.0;
  }

  const int delta = 3 * iconSize / 4;
  QRect rect(0, 0, delta, delta);
  rect.moveCenter(QPoint(iconSize / 2, iconSize / 2));
  painter.drawPie(rect, 0, 5760 * opacity);
  return pixmap;
}

//-----------------------------------------------------------------------------
/**
 * pqPropertyLinksConnection doesn't handle vtkSMDoubleMapProperty (since it
 * uses pqSMAdaptor which doesn't support vtkSMDoubleMapProperty). pqSMAdaptor
 * needs to die, hence I am not adding any new code to it. Instead we use a
 * custom pqPropertyLinksConnection to support getting/setting values from/on a
 * vtkSMDoubleMapProperty.
 */
//-----------------------------------------------------------------------------
class CConnectionType : public pqPropertyLinksConnection
{
  typedef pqPropertyLinksConnection Superclass;

public:
  CConnectionType(QObject* qobject, const char* qproperty, const char* qsignal, vtkSMProxy* smproxy,
    vtkSMProperty* smproperty, int smindex, bool use_unchecked_modified_event,
    QObject* parentObject = nullptr)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
  }
  ~CConnectionType() override {}
protected:
  void setServerManagerValue(bool vtkNotUsed(use_unchecked), const QVariant& value) override
  {
    if (vtkSMDoubleMapProperty* dmp = vtkSMDoubleMapProperty::SafeDownCast(this->propertySM()))
    {
      const QList<QVariant> curValue = value.toList();

      const int numComps = static_cast<int>(dmp->GetNumberOfComponents());
      dmp->ClearElements();
      std::vector<double> comp_value(numComps);
      for (int cc = 0; (cc + numComps) < curValue.size(); cc += (numComps + 1))
      {
        vtkIdType key = curValue[cc].value<vtkIdType>();
        for (int comp = 0; comp < numComps; ++comp)
        {
          comp_value[comp] = curValue[cc + comp + 1].toDouble();
        }
        dmp->SetElements(key, &comp_value[0], numComps);
      }
    }
  }
  QVariant currentServerManagerValue(bool vtkNotUsed(use_unchecked)) const override
  {
    QList<QVariant> curValue;
    if (vtkSMDoubleMapProperty* dmp = vtkSMDoubleMapProperty::SafeDownCast(this->propertySM()))
    {
      const unsigned int numComps = dmp->GetNumberOfComponents();
      vtkSMDoubleMapPropertyIterator* iter = dmp->NewIterator();
      for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
        curValue.push_back(iter->GetKey());
        for (unsigned int cc = 0; cc < numComps; ++cc)
        {
          curValue.push_back(iter->GetElementComponent(cc));
        }
      }
      iter->Delete();
    }
    return curValue;
  }

private:
  Q_DISABLE_COPY(CConnectionType);
};

//-----------------------------------------------------------------------------
/**
 * MultiBlockInspectorModel is a QIdentityProxyModel that converts data stored
 * in a pqCompositeDataInformationTreeModel's custom columns for color and
 * opacity to nice swatches to represent to the user.
 *
 * It also handles showing colors from a vtkScalarsToColors if no color is
 * provided by pqCompositeDataInformationTreeModel.
 */
//-----------------------------------------------------------------------------
class MultiBlockInspectorModel : public QIdentityProxyModel
{
  typedef QIdentityProxyModel Superclass;

public:
  MultiBlockInspectorModel(QWidget* p)
    : Superclass(p)
    , OpacityColumn(1)
    , ColorColumn(2)
    , LUT()
    , WrapCount(0)
    , IconSize(16)
    , RootLabel()
  {
    this->IconSize = std::max(p->style()->pixelMetric(QStyle::PM_SmallIconSize), 16);
  }
  ~MultiBlockInspectorModel() override {}

  void setRootLabel(const QString& str)
  {
    if (str != this->RootLabel)
    {
      this->RootLabel = str;
      QModelIndex rootIdx = this->index(0, 0, QModelIndex());
      emit this->dataChanged(rootIdx, rootIdx);
    }
  }
  inline const int& iconSize() const { return this->IconSize; }
  void setOpacityColumn(int col) { this->OpacityColumn = col; }
  int opacityColumn() { return this->OpacityColumn; }
  void setColorColumn(int col) { this->ColorColumn = col; }
  int colorColumn() const { return this->ColorColumn; }

  QColor color(const QModelIndex& idx) const
  {
    assert(idx.column() == this->ColorColumn);

    const QModelIndex srcIdx = this->mapToSource(idx);
    QColor dcolor = this->sourceModel()->data(srcIdx, Qt::DisplayRole).value<QColor>();
    if (!dcolor.isValid() && this->LUT)
    {
      QVariant vLeafIndex =
        this->sourceModel()->data(srcIdx, pqCompositeDataInformationTreeModel::LeafIndexRole);
      if (vLeafIndex.isValid())
      {
        unsigned int lidx = vLeafIndex.value<unsigned int>();
        const unsigned char* rgb =
          this->LUT->MapValue(this->WrapCount > 1 ? (lidx % this->WrapCount) : lidx);
        dcolor = QColor::fromRgb(rgb[0], rgb[1], rgb[2]);
      }
    }
    return dcolor;
  }

  QPixmap colorPixmap(const QModelIndex& idx) const
  {
    assert(idx.column() == this->ColorColumn);

    const QModelIndex srcIdx = this->mapToSource(idx);
    QColor dcolor = this->sourceModel()->data(srcIdx, Qt::DisplayRole).value<QColor>();
    bool inherited = this->sourceModel()
                       ->data(srcIdx, pqCompositeDataInformationTreeModel::ValueInheritedRole)
                       .toBool();
    if (!dcolor.isValid() && this->LUT)
    {
      QVariant vLeafIndex =
        this->sourceModel()->data(srcIdx, pqCompositeDataInformationTreeModel::LeafIndexRole);
      if (vLeafIndex.isValid())
      {
        unsigned int lidx = vLeafIndex.value<unsigned int>();
        const unsigned char* rgb =
          this->LUT->MapValue(this->WrapCount > 1 ? (lidx % this->WrapCount) : lidx);
        dcolor = QColor::fromRgb(rgb[0], rgb[1], rgb[2]);

        // since color-map value is never explicit, let's render it as if it was
        // inherited.
        inherited = true;
      }
    }
    return ColorPixmap(this->IconSize, dcolor, inherited);
  }

  void setColor(const QModelIndex& idx, const QColor& acolor)
  {
    assert(idx.column() == this->ColorColumn);
    this->sourceModel()->setData(this->mapToSource(idx), acolor, Qt::DisplayRole);
  }

  void setColor(const QModelIndex& idx, const QVariant& acolor)
  {
    assert(idx.column() == this->ColorColumn);
    this->sourceModel()->setData(this->mapToSource(idx), acolor, Qt::DisplayRole);
  }

  double opacity(const QModelIndex& idx) const
  {
    assert(idx.column() == this->OpacityColumn);
    const QModelIndex srcIdx = this->mapToSource(idx);
    const QVariant vval = this->sourceModel()->data(srcIdx, Qt::DisplayRole);
    return vval.isValid() ? vval.toDouble() : 1.0;
  }

  QPixmap opacityPixmap(const QModelIndex& idx) const
  {
    assert(idx.column() == this->OpacityColumn);
    const QModelIndex srcIdx = this->mapToSource(idx);
    const QVariant vval = this->sourceModel()->data(srcIdx, Qt::DisplayRole);
    double dval = vval.isValid() ? vval.toDouble() : -1.0;
    bool inherited = this->sourceModel()
                       ->data(srcIdx, pqCompositeDataInformationTreeModel::ValueInheritedRole)
                       .toBool();
    return OpacityPixmap(this->IconSize, dval, inherited);
  }

  void setOpacity(const QModelIndex& idx, double aopacity)
  {
    assert(idx.column() == this->OpacityColumn);
    this->sourceModel()->setData(this->mapToSource(idx), aopacity, Qt::DisplayRole);
  }

  void setOpacity(const QModelIndex& idx, const QVariant& aopacity)
  {
    assert(idx.column() == this->OpacityColumn);
    this->sourceModel()->setData(this->mapToSource(idx), aopacity, Qt::DisplayRole);
  }

  QVariant data(const QModelIndex& idx, int role) const override
  {
    if (idx.column() == this->OpacityColumn)
    {
      switch (role)
      {
        case Qt::DecorationRole:
          return this->opacityPixmap(idx);

        case Qt::ToolTipRole:
          return "Double-click to change block opacity";

        default:
          return QVariant();
      }
    }

    if (idx.column() == this->ColorColumn)
    {
      switch (role)
      {
        case Qt::DecorationRole:
          return this->colorPixmap(idx);
        case Qt::ToolTipRole:
          return "Double-click to change block color";
        default:
          return QVariant();
      }
    }
    if (this->RootLabel.isEmpty() == false && idx.column() == 0 && idx.parent().isValid() == false)
    {
      switch (role)
      {
        case Qt::DisplayRole:
          return this->RootLabel;

        case Qt::FontRole:
        {
          QFont font;
          font.setBold(true);
          return font;
        }
        default:
          break;
      }
    }

    return this->sourceModel()->data(this->mapToSource(idx), role);
  }

  void setScalarColoring(vtkScalarsToColors* lut, int wrap_count)
  {
    if (this->LUT != lut || this->WrapCount != wrap_count)
    {
      this->LUT = lut;
      this->WrapCount = wrap_count;
      if (this->ColorColumn >= 0 && this->ColorColumn < this->columnCount())
      {
        this->emitDataChanged(this->ColorColumn);
      }
    }
  }

private:
  void emitDataChanged(const int& col, const QModelIndex& idx = QModelIndex())
  {
    const int numChildren = this->rowCount(idx);
    if (numChildren > 0)
    {
      emit this->dataChanged(this->index(0, col, idx), this->index(numChildren, col, idx));
      for (int cc = 0; cc < numChildren; ++cc)
      {
        this->emitDataChanged(col, this->index(cc, col, idx));
      }
    }
  }

private:
  Q_DISABLE_COPY(MultiBlockInspectorModel);
  int OpacityColumn;
  int ColorColumn;
  vtkSmartPointer<vtkScalarsToColors> LUT;
  int WrapCount;
  int IconSize;
  QString RootLabel;
};

//-----------------------------------------------------------------------------
/**
 * MultiBlockInspectorSelectionModel extends QItemSelectionModel. When users
 * selects a node in the QTreeView, we want to select corresponding blocks and
 * vice-versa. This class helps us do that.
 */
//-----------------------------------------------------------------------------
class MultiBlockInspectorSelectionModel : public QItemSelectionModel
{
  typedef QItemSelectionModel Superclass;

public:
  MultiBlockInspectorSelectionModel(QAbstractProxyModel* amodel,
    pqCompositeDataInformationTreeModel* cdtModel, pqMultiBlockInspectorWidget* aparent = nullptr)
    : Superclass(amodel, aparent)
    , BlockSelectionPropagation(false)
    , MBWidget(aparent)
    , CDTModel(cdtModel)
  {
  }

  ~MultiBlockInspectorSelectionModel() override {}

  using Superclass::select;

  /**
   * used to disable propagation between Qt and ParaView (or vice-versa).
   */
  bool blockSelectionPropagation(bool newval)
  {
    bool ret = this->BlockSelectionPropagation;
    this->BlockSelectionPropagation = newval;
    return ret;
  }

  bool selectionPropagationBlocked() const { return this->BlockSelectionPropagation; }

  /**
   * Overridden to propagate selection to ParaView. If user selected blocks, create a block-based
   * selection
   * to select (and hence show) the selected blocks.
   */
  void select(
    const QItemSelection& aselection, QItemSelectionModel::SelectionFlags command) override
  {
    this->Superclass::select(aselection, command);
    if (!this->selectionPropagationBlocked())
    {
      const QAbstractItemModel* amodel = this->model();
      const QModelIndexList sRows = this->selectedRows();
      std::vector<vtkIdType> composite_ids;
      composite_ids.reserve(sRows.size());
      foreach (const QModelIndex& idx, sRows)
      {
        const QVariant val =
          amodel->data(idx, pqCompositeDataInformationTreeModel::CompositeIndexRole);
        if (val.isValid())
        {
          const vtkIdType cid = val.value<vtkIdType>();
          if (cid != 0)
          {
            // let's skip root node. Selecting all blocks doesn't make a whole lot of sense IMHO.
            composite_ids.push_back(cid);
          }
        }
      }
      this->selectBlocks(composite_ids);
    }
  }

  /**
   * Called when ParaView notifies us that the application selected something.
   * We update the selection, as appropriate.
   */
  void selected(pqOutputPort* port)
  {
    if (this->selectionPropagationBlocked())
    {
      return;
    }

    QScopedValueRollback<bool> r(this->BlockSelectionPropagation, true);

    QItemSelection aselection;
    const QAbstractProxyModel* amodel = qobject_cast<const QAbstractProxyModel*>(this->model());
    // amodel's source model may be null if the data is not a composite dataset
    // BUG #18939.
    if (amodel->sourceModel() != nullptr)
    {
      vtkSMSourceProxy* selSource = port ? port->getSelectionInput() : nullptr;
      if (selSource && strcmp(selSource->GetXMLName(), "BlockSelectionSource") == 0)
      {
        vtkSMPropertyHelper blocksHelper(selSource, "Blocks");
        for (unsigned int cc = 0, max = blocksHelper.GetNumberOfElements(); cc < max; ++cc)
        {
          const QModelIndex midx = this->CDTModel->find(blocksHelper.GetAsIdType(cc));
          if (midx.isValid())
          {
            const QModelIndex idx = amodel->mapFromSource(midx);
            aselection.select(idx, idx);
          }
        }
      }
    }
    this->select(aselection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  }

private:
  void selectBlocks(const std::vector<vtkIdType>& ids)
  {
    assert(this->BlockSelectionPropagation == false);
    QScopedValueRollback<bool> r(this->BlockSelectionPropagation, true);

    pqOutputPort* port = this->MBWidget->outputPort();
    if (port == nullptr)
    {
      return;
    }
    if (ids.size() == 0)
    {
      // clear selection if no blocks were selected.
      pqSelectionManager* selectionManager = qobject_cast<pqSelectionManager*>(
        pqApplicationCore::instance()->manager("SelectionManager"));
      if (selectionManager)
      {
        selectionManager->select(nullptr);
      }
      return;
    }

    SCOPED_UNDO_EXCLUDE();

    // This is bad. We should avoid creating new selection sources, if we can
    // and reuse existing one. This can be cleaned up when we refactor selection
    // to move most of the control logic to ServerManager layer.
    vtkSMSessionProxyManager* pxm = port->getServer()->proxyManager();
    vtkSmartPointer<vtkSMSourceProxy> selSource;
    selSource.TakeReference(
      vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "BlockSelectionSource")));
    if (!selSource)
    {
      return;
    }

    // set selected blocks
    if (ids.size() > 0)
    {
      vtkSMPropertyHelper(selSource, "Blocks")
        .Set(&ids.front(), static_cast<unsigned int>(ids.size()));
    }
    else
    {
      vtkSMPropertyHelper(selSource, "Blocks").SetNumberOfElements(0);
    }
    selSource->UpdateVTKObjects();
    port->setSelectionInput(selSource, 0);

    // update the selection manager
    // (unfortunate code!!! can't wait for the selection refactor!!!)
    pqSelectionManager* selectionManager =
      qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SelectionManager"));
    if (selectionManager)
    {
      selectionManager->select(port);
    }
  }

private:
  Q_DISABLE_COPY(MultiBlockInspectorSelectionModel);
  bool BlockSelectionPropagation;
  QPointer<pqMultiBlockInspectorWidget> MBWidget;
  QPointer<pqCompositeDataInformationTreeModel> CDTModel;
};
}

//=============================================================================
class pqMultiBlockInspectorWidget::pqInternals
{
public:
  Ui::MultiBlockInspectorWidget Ui;
  QPointer<pqCompositeDataInformationTreeModel> CDTModel;
  QPointer<MultiBlockInspectorModel> ProxyModel;
  QPointer<MultiBlockInspectorSelectionModel> SelectionModel;
  QPointer<pqView> View;
  QPointer<pqOutputPort> OutputPort;
  QPointer<pqDataRepresentation> Representation;
  void* RepresentationVoidPtr; // used to check if the ptr changed.

  QList<QPair<unsigned int, bool> > BlockVisibilities;
  QList<QPair<unsigned int, QVariant> > BlockColors;
  QList<QPair<unsigned int, QVariant> > BlockOpacities;

  // These are set when this->Representation is setup to match
  // the capabilities available on the representation.
  bool UserCheckable;
  bool HasColors;
  bool HasOpacities;

  pqPropertyLinks Links;
  pqTimer ColoringTimer;
  pqTimer ResetTimer;

  pqInternals(pqMultiBlockInspectorWidget* self)
    : CDTModel(new pqCompositeDataInformationTreeModel(self))
    , ProxyModel(new MultiBlockInspectorModel(self))
    , SelectionModel(new MultiBlockInspectorSelectionModel(this->ProxyModel, this->CDTModel, self))
    , RepresentationVoidPtr(nullptr)
    , UserCheckable(false)
    , HasColors(false)
    , HasOpacities(false)
  {
    this->Ui.setupUi(self);
    this->Ui.treeView->header()->setDefaultSectionSize(this->ProxyModel->iconSize() + 4);
    this->Ui.treeView->header()->setMinimumSectionSize(this->ProxyModel->iconSize() + 4);
    this->Ui.treeView->setModel(this->ProxyModel);
    this->Ui.treeView->setSelectionModel(this->SelectionModel);
    this->Ui.treeView->expand(this->Ui.treeView->rootIndex());

    this->ColoringTimer.setSingleShot(true);
    this->ColoringTimer.setInterval(0);

    this->ResetTimer.setSingleShot(true);
    this->ResetTimer.setInterval(0);

    if (pqSettings* settings = pqApplicationCore::instance()->settings())
    {
      bool checked = settings->value("pqMultiBlockInspectorWidget/ShowHints", true).toBool();
      this->Ui.showHints->setChecked(checked);
    }
  }
  ~pqInternals()
  {
    if (pqSettings* settings = pqApplicationCore::instance()->settings())
    {
      settings->setValue("pqMultiBlockInspectorWidget/ShowHints", this->Ui.showHints->isChecked());
    }
  }

  void updateRootLabel()
  {
    QString label;
    if (pqOutputPort* port = this->OutputPort)
    {
      pqPipelineSource* src = port->getSource();
      if (src->getNumberOfOutputPorts() > 1)
      {
        label = QString("%1:%2").arg(src->getSMName()).arg(port->getPortName());
      }
      else
      {
        label = QString("%1").arg(src->getSMName());
      }
    }
    this->ProxyModel->setRootLabel(label);
  }

  void resetModel()
  {
    vtkVLogScopeFunction(PARAVIEW_LOG_APPLICATION_VERBOSITY());

    pqTreeViewExpandState expandState;
    expandState.save(this->Ui.treeView);

    bool prev = this->SelectionModel->blockSelectionPropagation(true);
    pqOutputPort* port = this->OutputPort;
    this->CDTModel->clearColumns();
    this->CDTModel->setUserCheckable(this->UserCheckable);
    if (this->HasColors && this->Representation != nullptr)
    {
      this->ProxyModel->setColorColumn(this->CDTModel->addColumn("color"));
    }
    else
    {
      this->HasColors = false;
    }
    if (this->HasOpacities && this->Representation != nullptr)
    {
      this->ProxyModel->setOpacityColumn(this->CDTModel->addColumn("opacity"));
    }
    else
    {
      this->HasOpacities = false;
    }
    this->updateRootLabel();
    bool is_composite =
      this->CDTModel->reset(port != nullptr ? port->getDataInformation() : nullptr);
    if (!is_composite)
    {
      this->ProxyModel->setSourceModel(nullptr);
    }
    else
    {
      this->ProxyModel->setSourceModel(this->CDTModel);
      this->Ui.treeView->expandToDepth(1);

      QHeaderView* header = this->Ui.treeView->header();
      if (header->count() == 3 && header->logicalIndex(2) != 0)
      {
        header->moveSection(0, 2);
      }
    }

    expandState.restore(this->Ui.treeView);
    this->SelectionModel->blockSelectionPropagation(prev);
  }

  // Called when the representation changes. We remove cached values for various
  // block-based properties.
  void clearCache()
  {
    this->BlockVisibilities.clear();
    this->BlockColors.clear();
    this->BlockOpacities.clear();
  }

  void restoreCachedValues()
  {
    vtkVLogScopeFunction(PARAVIEW_LOG_APPLICATION_VERBOSITY());
    if (this->Representation)
    {
      // restore check-state, property state, if possible.
      if (this->UserCheckable)
      {
        assert(this->CDTModel->userCheckable());
        this->CDTModel->setCheckStates(this->BlockVisibilities);
      }
      if (this->HasColors)
      {
        assert(this->CDTModel->columnIndex("color") != -1);
        this->CDTModel->setColumnStates("color", this->BlockColors);
      }
      if (this->HasOpacities)
      {
        assert(this->CDTModel->columnIndex("opacity") != -1);
        this->CDTModel->setColumnStates("opacity", this->BlockOpacities);
      }
    }
  }
};

//-----------------------------------------------------------------------------
pqMultiBlockInspectorWidget::pqMultiBlockInspectorWidget(
  QWidget* parentObject, Qt::WindowFlags f, bool arg_autotracking)
  : Superclass(parentObject, f)
  , Internals(new pqMultiBlockInspectorWidget::pqInternals(this))
  , AutoTracking(arg_autotracking)
{
  pqInternals& internals = (*this->Internals);

  // Hookups for timers.
  this->connect(&internals.ColoringTimer, SIGNAL(timeout()), SLOT(updateScalarColoring()));
  this->connect(&internals.ResetTimer, SIGNAL(timeout()), SLOT(resetNow()));

  // Hookups for user interactions.
  this->connect(internals.Ui.treeView, SIGNAL(doubleClicked(const QModelIndex&)),
    SLOT(itemDoubleClicked(const QModelIndex&)));

  // Hookup for selection changes
  if (pqSelectionManager* selectionManager = qobject_cast<pqSelectionManager*>(
        pqApplicationCore::instance()->manager("SelectionManager")))
  {
    this->connect(
      selectionManager, SIGNAL(selectionChanged(pqOutputPort*)), SLOT(selected(pqOutputPort*)));
  }

  // Hookup for context menu.
  this->connect(internals.Ui.treeView, SIGNAL(customContextMenuRequested(const QPoint&)),
    SLOT(contextMenu(const QPoint&)));

  // When check-state changes, we need to fire signals.
  this->connect(internals.CDTModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
    SLOT(modelDataChanged(const QModelIndex&, const QModelIndex&)));

  // hookups for auto-tracking.
  if (this->AutoTracking)
  {
    this->connect(&pqActiveObjects::instance(), SIGNAL(portChanged(pqOutputPort*)),
      SLOT(setOutputPortInternal(pqOutputPort*)));
    this->connect(
      &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), SLOT(setViewInternal(pqView*)));
    this->setOutputPortInternal(pqActiveObjects::instance().activePort());
    this->setViewInternal(pqActiveObjects::instance().activeView());
  }
}

//-----------------------------------------------------------------------------
pqMultiBlockInspectorWidget::~pqMultiBlockInspectorWidget()
{
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::selected(pqOutputPort* port)
{
  pqInternals& internals = (*this->Internals);
  internals.SelectionModel->selected(port == this->outputPort() ? port : nullptr);
}

//-----------------------------------------------------------------------------
pqOutputPort* pqMultiBlockInspectorWidget::outputPort() const
{
  const pqInternals& internals = (*this->Internals);
  return internals.OutputPort;
}

//-----------------------------------------------------------------------------
pqView* pqMultiBlockInspectorWidget::view() const
{
  const pqInternals& internals = (*this->Internals);
  return internals.View;
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqMultiBlockInspectorWidget::representation() const
{
  const pqInternals& internals = (*this->Internals);
  return internals.Representation;
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setOutputPort(pqOutputPort* port)
{
  if (this->isAutoTrackingEnabled())
  {
    qDebug("`setOutputPort` called when auto-tracking is enabled. Ignored.");
  }
  else
  {
    this->setOutputPortInternal(port);
  }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setView(pqView* aview)
{
  if (this->isAutoTrackingEnabled())
  {
    qDebug("`setView` called when auto-tracking is enabled. Ignored.");
  }
  else
  {
    this->setViewInternal(aview);
  }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::updateRepresentation()
{
  // determine "current representation" from current view and current port.
  pqInternals& internals = (*this->Internals);
  pqDataRepresentation* repr =
    internals.OutputPort ? internals.OutputPort->getRepresentation(internals.View) : nullptr;
  this->setRepresentation(repr);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setViewInternal(pqView* aview)
{
  pqInternals& internals = (*this->Internals);
  if (internals.View == aview)
  {
    return;
  }

  if (internals.View)
  {
    this->disconnect(internals.View);
  }
  internals.View = aview;
  if (internals.View)
  {
    // to keep things consistent when auto-tracking is enabled & disabled, we
    // don't track active representation, instead we do this.
    this->connect(
      internals.View, SIGNAL(representationAdded(pqRepresentation*)), SLOT(updateRepresentation()));
  }

  this->updateRepresentation();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setOutputPortInternal(pqOutputPort* port)
{
  pqInternals& internals = (*this->Internals);
  if (internals.OutputPort == port)
  {
    return;
  }

  if (internals.OutputPort)
  {
    this->disconnect(internals.OutputPort->getSource());
  }
  internals.OutputPort = port;
  if (internals.OutputPort)
  {
    pqPipelineSource* src = internals.OutputPort->getSource();
    this->connect(src, SIGNAL(dataUpdated(pqPipelineSource*)), SLOT(resetEventually()));
    this->connect(src, SIGNAL(nameChanged(pqServerManagerModelItem*)), SLOT(nameChanged()));
  }

  this->updateRepresentation();
  this->resetEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setRepresentation(pqDataRepresentation* repr)
{
  pqInternals& internals = (*this->Internals);
  if (internals.RepresentationVoidPtr != repr)
  {
    if (internals.Representation)
    {
      internals.ColoringTimer.disconnect(internals.Representation);
      internals.Representation->disconnect(this);
    }
    internals.Links.clear();
    internals.clearCache();
    internals.UserCheckable = internals.HasColors = internals.HasOpacities = false;
    internals.Representation = repr;
    internals.RepresentationVoidPtr = repr;
    this->updateScalarColoring();
    if (repr)
    {
      internals.ColoringTimer.connect(repr, SIGNAL(colorArrayNameModified()), SLOT(start()));
      internals.ColoringTimer.connect(repr, SIGNAL(colorTransferFunctionModified()), SLOT(start()));
      repr->connect(this, SIGNAL(requestRender()), SLOT(renderViewEventually()));

      vtkSMProxy* reprProxy = repr->getProxy();
      if (vtkSMProperty* prop = reprProxy->GetProperty("BlockColor"))
      {
        internals.HasColors = true;
        internals.Links.addPropertyLink<CConnectionType>(
          this, "blockColors", SIGNAL(blockColorsChanged()), reprProxy, prop);
      }
      if (vtkSMProperty* prop = reprProxy->GetProperty("BlockOpacity"))
      {
        internals.HasOpacities = true;
        internals.Links.addPropertyLink<CConnectionType>(
          this, "blockOpacities", SIGNAL(blockOpacitiesChanged()), reprProxy, prop);
      }
      if (vtkSMProperty* prop = reprProxy->GetProperty("BlockVisibility"))
      {
        internals.UserCheckable = true;
        if (internals.CDTModel->defaultCheckState() == false)
        {
          internals.CDTModel->setDefaultCheckState(true);
          // init check states to ensure the `setBlockVisibilities()` gets
          // called appropriately when the link is added.
          internals.CDTModel->setCheckStates(QList<QPair<unsigned int, bool> >());
        }
        internals.Links.addPropertyLink(
          this, "blockVisibilities", SIGNAL(blockVisibilitiesChanged()), reprProxy, prop);
      }
      else if (vtkSMProperty* prop2 = reprProxy->GetProperty("CompositeDataSetIndex"))
      {
        internals.UserCheckable = true;
        if (internals.CDTModel->defaultCheckState() == true)
        {
          internals.CDTModel->setDefaultCheckState(false);
          // int check states to ensure the `setVisibleBlocks()` gets
          // called appropriately when the link is added.
          internals.CDTModel->setChecked(QList<unsigned int>());
        }
        internals.Links.addPropertyLink(
          this, "visibleBlocks", SIGNAL(blockVisibilitiesChanged()), reprProxy, prop2);
      }
    }
    this->resetEventually();
  }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::updateScalarColoring()
{
  pqInternals& internals = (*this->Internals);
  if (vtkSMProxy* repr = internals.Representation ? internals.Representation->getProxy() : nullptr)
  {
    const char* arrayName =
      vtkSMPropertyHelper(repr, "ColorArrayName", true).GetInputArrayNameToProcess();
    int wrap_count = vtkSMPropertyHelper(repr, "BlockColorsDistinctValues", true).GetAsInt();
    vtkSMProxy* lutProxy = vtkSMPropertyHelper(repr, "LookupTable", true).GetAsProxy();
    if (arrayName != nullptr && strcmp(arrayName, "vtkBlockColors") == 0 && lutProxy != nullptr)
    {
      internals.ProxyModel->setScalarColoring(
        vtkScalarsToColors::SafeDownCast(lutProxy->GetClientSideObject()), wrap_count);
      return;
    }
  }
  internals.ProxyModel->setScalarColoring(nullptr, 0);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::nameChanged()
{
  pqInternals& internals = (*this->Internals);
  internals.updateRootLabel();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::resetEventually()
{
  pqInternals& internals = (*this->Internals);
  internals.ResetTimer.start();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::resetNow()
{
  vtkVLogScopeFunction(PARAVIEW_LOG_APPLICATION_VERBOSITY());

  QSignalBlocker b(this);
  pqInternals& internals = (*this->Internals);
  internals.ResetTimer.stop();
  internals.resetModel();
  internals.restoreCachedValues();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqMultiBlockInspectorWidget::blockVisibilities() const
{
  QList<QVariant> retval;
  pqInternals& internals = (*this->Internals);
  internals.BlockVisibilities = internals.CDTModel->checkStates();

  const QList<QPair<unsigned int, bool> >& states = internals.BlockVisibilities;
  for (auto iter = states.begin(); iter != states.end(); ++iter)
  {
    retval.push_back(iter->first);
    retval.push_back(iter->second ? 1 : 0);
  }
  return retval;
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setBlockVisibilities(const QList<QVariant>& bvs)
{
  QSignalBlocker b(this);

  pqInternals& internals = (*this->Internals);
  QList<QPair<unsigned int, bool> >& states = internals.BlockVisibilities;
  states.clear();

  for (int cc = 0, max = bvs.size(); (cc + 1) < max; cc += 2)
  {
    states.push_back(
      QPair<unsigned int, bool>(bvs[cc].value<unsigned int>(), bvs[cc + 1].toBool()));
  }

  internals.CDTModel->setCheckStates(states);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqMultiBlockInspectorWidget::visibleBlocks() const
{
  QList<QVariant> retval;
  pqInternals& internals = (*this->Internals);
  auto checkedNodes = internals.CDTModel->checkedNodes();
  for (const auto& val : checkedNodes)
  {
    retval.push_back(val);
  }
  return retval;
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setVisibleBlocks(const QList<QVariant>& vbs)
{
  QSignalBlocker b(this);

  QList<unsigned int> indices;
  for (const auto& vval : vbs)
  {
    indices.push_back(vval.value<unsigned int>());
  }

  pqInternals& internals = (*this->Internals);
  internals.CDTModel->setChecked(indices);
  internals.BlockVisibilities = internals.CDTModel->checkStates();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqMultiBlockInspectorWidget::blockColors() const
{
  QList<QVariant> retval;

  pqInternals& internals = (*this->Internals);
  if (internals.CDTModel->columnIndex("color") != -1)
  {
    internals.BlockColors = internals.CDTModel->columnStates("color");

    const QList<QPair<unsigned int, QVariant> >& colors = internals.BlockColors;
    for (auto iter = colors.begin(); iter != colors.end(); ++iter)
    {
      retval.push_back(iter->first);
      QColor color = iter->second.value<QColor>();
      retval.push_back(color.redF());
      retval.push_back(color.greenF());
      retval.push_back(color.blueF());
    }
  }
  return retval;
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setBlockColors(const QList<QVariant>& bcs)
{
  QSignalBlocker b(this);

  pqInternals& internals = (*this->Internals);
  QList<QPair<unsigned int, QVariant> >& colors = internals.BlockColors;
  colors.clear();

  for (int cc = 0, max = bcs.size(); (cc + 3) < max; cc += 4)
  {
    unsigned int idx = bcs[cc].value<unsigned int>();
    QColor color =
      QColor::fromRgbF(bcs[cc + 1].toDouble(), bcs[cc + 2].toDouble(), bcs[cc + 3].toDouble());
    colors.push_back(QPair<unsigned int, QVariant>(idx, color));
  }

  if (internals.CDTModel->columnIndex("color") != -1)
  {
    internals.CDTModel->setColumnStates("color", colors);
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqMultiBlockInspectorWidget::blockOpacities() const
{
  QList<QVariant> retval;

  pqInternals& internals = (*this->Internals);
  if (internals.CDTModel->columnIndex("opacity") != -1)
  {
    internals.BlockOpacities = internals.CDTModel->columnStates("opacity");

    const QList<QPair<unsigned int, QVariant> >& opacities = internals.BlockOpacities;
    for (auto iter = opacities.begin(); iter != opacities.end(); ++iter)
    {
      retval.push_back(iter->first);
      retval.push_back(iter->second);
    }
  }
  return retval;
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setBlockOpacities(const QList<QVariant>& bos)
{
  QSignalBlocker b(this);

  pqInternals& internals = (*this->Internals);
  QList<QPair<unsigned int, QVariant> >& opacities = internals.BlockOpacities;
  opacities.clear();

  for (int cc = 0, max = bos.size(); (cc + 1) < max; cc += 2)
  {
    unsigned int idx = bos[cc].value<unsigned int>();
    double opacity = bos[cc + 1].toDouble();
    opacities.push_back(QPair<unsigned int, QVariant>(idx, opacity));
  }

  if (internals.CDTModel->columnIndex("opacity") != -1)
  {
    internals.CDTModel->setColumnStates("opacity", opacities);
  }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::itemDoubleClicked(const QModelIndex& idx)
{
  pqInternals& internals = (*this->Internals);
  if (idx.column() == 1)
  {
    QColor color = internals.ProxyModel->color(idx);
    QColor newColor =
      QColorDialog::getColor(color, this, "Select Color", QColorDialog::DontUseNativeDialog);
    if (newColor.isValid())
    {
      this->setColor(idx, newColor);
    }
  }
  else if (idx.column() == 2)
  {
    double opacity = internals.ProxyModel->opacity(idx);
    pqDoubleRangeDialog dialog("Opacity:", 0.0, 1.0, this);
    dialog.setWindowTitle("Select Opacity");
    dialog.setValue(opacity);
    if (dialog.exec() == QDialog::Accepted)
    {
      this->setOpacity(idx, qBound(0.0, dialog.value(), 1.0));
    }
  }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::modelDataChanged(const QModelIndex& start, const QModelIndex& end)
{
  if (start.column() <= 0 && end.column() >= 0)
  {
    SCOPED_UNDO_SET("Change Block Visibilities");
    emit this->blockVisibilitiesChanged();
    emit this->requestRender();
  }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::contextMenu(const QPoint& pos)
{
  QMenu menu;

  pqInternals& internals = (*this->Internals);
  if (internals.Representation == nullptr || internals.SelectionModel->hasSelection() == false)
  {
    const QAction* expandAll = menu.addAction("Expand All");
    if (QAction* selAction = menu.exec(internals.Ui.treeView->mapToGlobal(pos)))
    {
      if (selAction == expandAll)
      {
        internals.Ui.treeView->expandAll();
      }
    }
    return;
  }

  const QAction* showBlocks = menu.addAction("Show Block(s)");
  const QAction* hideBlocks = menu.addAction("Hide Block(s)");
  menu.addSeparator();
  const QAction* setColors = menu.addAction("Set Block Color(s) ...");
  const QAction* resetColors = menu.addAction("Reset Block Color(s)");
  menu.addSeparator();
  const QAction* setOpacities = menu.addAction("Set Block Opacities ...");
  const QAction* resetOpacities = menu.addAction("Reset Block Opacities");
  menu.addSeparator();
  const QAction* expandAll = menu.addAction("Expand All");

  if (QAction* selAction = menu.exec(internals.Ui.treeView->mapToGlobal(pos)))
  {
    QModelIndex idx = internals.SelectionModel->currentIndex();
    if (selAction == showBlocks || selAction == hideBlocks)
    {
      const QModelIndexList sRows = internals.SelectionModel->selectedRows(0);
      const QVariant val(selAction == showBlocks ? Qt::Checked : Qt::Unchecked);
      for (auto iter = sRows.begin(); iter != sRows.end(); ++iter)
      {
        internals.ProxyModel->setData(*iter, val, Qt::CheckStateRole);
      }
      emit this->blockVisibilitiesChanged();
      emit this->requestRender();
    }
    else if (selAction == setColors)
    {
      QColor color =
        internals.ProxyModel->color(idx.sibling(idx.row(), internals.ProxyModel->colorColumn()));
      QColor newColor =
        QColorDialog::getColor(color, this, "Select Color", QColorDialog::DontUseNativeDialog);
      if (newColor.isValid())
      {
        this->setColor(idx, newColor);
      }
    }
    else if (selAction == resetColors)
    {
      this->setColor(idx, QColor());
    }
    else if (selAction == setOpacities)
    {
      double opacity = internals.ProxyModel->opacity(
        idx.sibling(idx.row(), internals.ProxyModel->opacityColumn()));
      pqDoubleRangeDialog dialog("Opacity:", 0.0, 1.0, this);
      dialog.setWindowTitle("Select Opacity");
      dialog.setValue(opacity);
      if (dialog.exec() == QDialog::Accepted)
      {
        this->setOpacity(idx, qBound(0.0, dialog.value(), 1.0));
      }
    }
    else if (selAction == resetOpacities)
    {
      // -ve opacity causes the value be cleared.
      this->setOpacity(idx, -1.0);
    }
    else if (selAction == expandAll)
    {
      internals.Ui.treeView->expandAll();
    }
  }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setColor(const QModelIndex& idx, const QColor& newcolor)
{
  pqInternals& internals = (*this->Internals);

  QModelIndexList sRows =
    internals.SelectionModel->selectedRows(internals.ProxyModel->colorColumn());
  if (idx.isValid() && !sRows.contains(idx))
  {
    // if idx not part of active selection, we don't update the all selected
    // nodes, only the current item.
    sRows.clear();
    sRows.push_back(idx.sibling(idx.row(), internals.ProxyModel->colorColumn()));
  }

  const QVariant val = newcolor.isValid() ? QVariant::fromValue(newcolor) : QVariant();
  SCOPED_UNDO_SET(newcolor.isValid() ? "Set Block Colors" : "Reset Block Colors");
  for (const QModelIndex& itemIdx : sRows)
  {
    internals.ProxyModel->setColor(itemIdx, val);
  }
  emit this->blockColorsChanged();
  emit this->requestRender();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setOpacity(const QModelIndex& idx, double opacity)
{
  pqInternals& internals = (*this->Internals);
  const QVariant val = (opacity >= 0 && opacity <= 1.0) ? QVariant(opacity) : QVariant();
  QModelIndexList sRows =
    internals.SelectionModel->selectedRows(internals.ProxyModel->opacityColumn());
  if (idx.isValid() && !sRows.contains(idx))
  {
    // if idx not part of active selection, we don't update the all selected
    // nodes, only the current item.
    sRows.clear();
    sRows.push_back(idx.sibling(idx.row(), internals.ProxyModel->opacityColumn()));
  }

  SCOPED_UNDO_SET(val.isValid() ? "Set Block Opacities" : "Reset Block Opacities");
  for (const QModelIndex& itemIdx : sRows)
  {
    internals.ProxyModel->setOpacity(itemIdx, val);
  }
  emit this->blockOpacitiesChanged();
  emit this->requestRender();
}
