/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewModel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#include "pqSpreadSheetViewModel.h"

#include "vtkArrayDispatch.h"
#include "vtkCellType.h"
#include "vtkDataArrayAccessor.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVCompositeDataInformationIterator.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSpreadSheetView.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkUnsignedIntArray.h"
#include "vtkVariant.h"
#include "vtkWeakPointer.h"

// Qt Includes.
#include <QItemSelectionModel>
#include <QPointer>
#include <QtDebug>

// ParaView Includes.
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqTimer.h"

#include <cassert>

static uint qHash(pqSpreadSheetViewModel::vtkIndex index)
{
  return qHash(index.Tuple[2]);
}

//-----------------------------------------------------------------------------
class pqSpreadSheetViewModel::pqInternal
{
public:
  pqInternal(pqSpreadSheetViewModel* svmodel)
    : SelectionModel(svmodel)
  {
    this->Dirty = true;
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->DecimalPrecision = 6;
    this->FixedRepresentation = false;
    this->ActiveRegion[0] = this->ActiveRegion[1] = -1;
    this->VTKView = NULL;

    this->LastColumnCount = 0;
    this->LastRowCount = 0;
  }

  QItemSelectionModel SelectionModel;
  pqTimer Timer;
  pqTimer SelectionTimer;
  int DecimalPrecision;
  bool FixedRepresentation;
  vtkIdType LastRowCount;
  vtkIdType LastColumnCount;

  int ActiveRegion[2];
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<pqDataRepresentation> ActiveRepresentation;
  vtkWeakPointer<vtkSMProxy> ActiveRepresentationProxy;
  vtkSpreadSheetView* VTKView;
  bool Dirty;
};

//-----------------------------------------------------------------------------
pqSpreadSheetViewModel::pqSpreadSheetViewModel(vtkSMProxy* view, QObject* parentObject)
  : Superclass(parentObject)
{
  assert(view != NULL);
  this->ViewProxy = view;
  this->Internal = new pqInternal(this);
  this->Internal->VTKView = vtkSpreadSheetView::SafeDownCast(view->GetClientSideObject());

  this->Internal->VTKConnect->Connect(
    this->Internal->VTKView, vtkCommand::UpdateDataEvent, this, SLOT(forceUpdate()));

  this->Internal->VTKConnect->Connect(this->Internal->VTKView, vtkCommand::UpdateEvent, this,
    SLOT(onDataFetched(vtkObject*, unsigned long, void*, void*)));

  this->Internal->Timer.setSingleShot(true);
  this->Internal->Timer.setInterval(500); // milliseconds.
  QObject::connect(&this->Internal->Timer, SIGNAL(timeout()), this, SLOT(delayedUpdate()));

  this->Internal->SelectionTimer.setSingleShot(true);
  this->Internal->SelectionTimer.setInterval(100); // milliseconds.
  QObject::connect(
    &this->Internal->SelectionTimer, SIGNAL(timeout()), this, SLOT(triggerSelectionChanged()));

  QObject::connect(&this->Internal->SelectionModel,
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
    &this->Internal->SelectionTimer, SLOT(start()));

  // QueuedConnection is needed to avoid updating the UI before property has
  // been "pushed".
  this->Internal->VTKConnect->Connect(view->GetProperty("HiddenColumnLabels"),
    vtkCommand::ModifiedEvent, this, SLOT(hiddenColumnsChanged()), nullptr, 0.0,
    Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewModel::~pqSpreadSheetViewModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqSpreadSheetViewModel::activeRepresentation() const
{
  return this->Internal->ActiveRepresentation;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqSpreadSheetViewModel::activeRepresentationProxy() const
{
  return (this->Internal->ActiveRepresentation
      ? this->Internal->ActiveRepresentation->getProxy()
      : this->Internal->ActiveRepresentationProxy.GetPointer());
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::setActiveRepresentation(pqDataRepresentation* repr)
{
  this->Internal->ActiveRepresentation = repr;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::setActiveRepresentationProxy(vtkSMProxy* repr)
{
  this->Internal->ActiveRepresentationProxy = repr;
}

//-----------------------------------------------------------------------------
vtkSpreadSheetView* pqSpreadSheetViewModel::GetView() const
{
  return this->Internal->VTKView;
}

//-----------------------------------------------------------------------------
int pqSpreadSheetViewModel::rowCount(const QModelIndex&) const
{
  return this->Internal->VTKView->GetNumberOfRows();
}

//-----------------------------------------------------------------------------
int pqSpreadSheetViewModel::columnCount(const QModelIndex&) const
{
  return this->Internal->VTKView->GetNumberOfColumns();
}

//-----------------------------------------------------------------------------
int pqSpreadSheetViewModel::getFieldType() const
{
  return this->GetView()->GetFieldAssociation();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::forceUpdate()
{
  this->Internal->ActiveRegion[0] = -1;
  this->Internal->ActiveRegion[1] = -1;
  this->Internal->SelectionModel.clear();
  this->Internal->Timer.stop();
  this->Internal->SelectionTimer.stop();

  vtkIdType& rows = this->Internal->LastRowCount;
  vtkIdType& columns = this->Internal->LastColumnCount;

  // We force a reset only when the rows/columns counts have changed, otherwise
  // we simply say the data changed. This avoid flickers on the screen when user
  // clicks on the view or apply a filter change etc.
  if (this->rowCount() != rows || this->columnCount() != columns)
  {
    this->beginResetModel();
    rows = this->rowCount();
    columns = this->columnCount();
    this->endResetModel();
  }
  else
  {
    if (rows && columns)
    {
      // we always invalidate header data, just to be on a safe side.
      emit this->headerDataChanged(Qt::Horizontal, 0, columns - 1);
      emit this->dataChanged(this->index(0, 0), this->index(rows - 1, columns - 1));
    }
  }
  // this ensures that we update the selected based on the current state.
  this->Internal->SelectionTimer.start();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::delayedUpdate()
{
  if (this->Internal->ActiveRegion[0] >= 0)
  {
    this->Internal->VTKView->GetValue(this->Internal->ActiveRegion[0], 0);
  }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::triggerSelectionChanged()
{
  emit this->selectionChanged(this->Internal->SelectionModel.selection());
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::setActiveRegion(int row_top, int row_bottom)
{
  this->Internal->ActiveRegion[0] = row_top;
  this->Internal->ActiveRegion[1] = row_bottom;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::onDataFetched(vtkObject*, unsigned long, void*, void* call_data)
{
  vtkIdType block = *reinterpret_cast<vtkIdType*>(call_data);
  vtkIdType blockSize = vtkSMPropertyHelper(this->ViewProxy, "BlockSize").GetAsIdType();

  // We deliberately invalid 1 row extra on each sides to ensure that the
  // visible viewport is always updated.
  vtkIdType rowMin = blockSize * block - 1;
  vtkIdType rowMax = blockSize * (block + 1);
  if (rowMin < 0)
  {
    rowMin = 0;
  }

  if (rowMax >= this->rowCount())
  {
    rowMax = this->rowCount() - 1;
  }

  QModelIndex topLeft(this->index(rowMax, 0));
  QModelIndex bottomRight(this->index(rowMax, this->columnCount() - 1));

  this->dataChanged(topLeft, bottomRight);
  // we always invalidate header data, just to be on a safe side.
  this->headerDataChanged(Qt::Horizontal, 0, this->columnCount() - 1);
}
namespace
{
struct ArrayTupleToStringConvertor
{
private:
  QTextStream Stream;
  const char* Separator;
  bool Fixed;
  int Precision;

public:
  ArrayTupleToStringConvertor(QString& string, const char* sep, bool fixed, int precision)
    : Stream(&string, QIODevice::WriteOnly)
    , Separator(sep)
    , Fixed(fixed)
    , Precision(precision)
  {
  }

  template <typename ArrayType>
  void operator()(ArrayType* darray)
  {
    vtkDataArrayAccessor<ArrayType> accessor(darray);
    if (darray->GetNumberOfTuples() == 1)
    {
      for (int cc = 0; cc < darray->GetNumberOfComponents(); ++cc)
      {
        this->Stream << (cc > 0 ? this->Separator : "") << this->number(accessor.Get(0, cc));
      }
    }
  }

  template <typename ValueType>
  QString number(const ValueType& val) const
  {
    return QString::number(val);
  }

  QString number(const double& val) const
  {
    return QString::number(val, this->Fixed ? 'f' : 'g', this->Precision);
  }

  QString number(const float& val) const
  {
    return QString::number(val, this->Fixed ? 'f' : 'g', this->Precision);
  }
};
};
//-----------------------------------------------------------------------------
QVariant pqSpreadSheetViewModel::data(const QModelIndex& idx, int role /*=Qt::DisplayRole*/) const
{
  if (role != Qt::DisplayRole && role != Qt::EditRole)
  {
    return QVariant();
  }

  int row = idx.row();
  int column = idx.column();
  vtkSpreadSheetView* view = this->GetView();
  if (!view->IsAvailable(row))
  {
    this->Internal->Timer.start();
    return QVariant("...");
  }

  if (!this->isDataValid(idx))
  {
    // If displaying field data, check to make sure that the data is valid
    // since its arrays can be of different lengths
    return QVariant("");
  }

  vtkVariant value = view->GetValue(row, column);
  bool is_selected = view->IsRowSelected(row);
  if (is_selected)
  {
    this->Internal->SelectionModel.select(
      this->index(row, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (value.IsVTKObject() && !value.IsArray())
  {
    vtkObjectBase* obj = value.ToVTKObject();
    if (obj)
    {
      return QString("(%1)").arg(obj->GetClassName());
    }
    return QString("(null)");
  }

  QString str = value.ToString().c_str();
  if (value.IsChar() || value.IsUnsignedChar() || value.IsSignedChar())
  {
    // Don't show ASCII character for char arrays.
    const char* column_name = view->GetColumnName(column);
    if (column_name && strcmp(column_name, "Cell Type") == 0)
    {
      return vtkSMCoreUtilities::GetStringForCellType(value.ToInt());
    }
    else
    {
      str = QString::number(value.ToInt());
    }
  }
  else if (value.IsFloat() || value.IsDouble())
  {
    str = QString::number(value.ToDouble(), this->Internal->FixedRepresentation ? 'f' : 'g',
      this->Internal->DecimalPrecision);
  }
  else if (value.IsArray())
  {
    str = QString();
    ArrayTupleToStringConvertor convertor(
      str, ", ", this->Internal->FixedRepresentation, this->Internal->DecimalPrecision);
    vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>::Execute(
      vtkDataArray::SafeDownCast(value.ToArray()), convertor);
  }
  return str;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::sortSection(int section, Qt::SortOrder order)
{
  vtkSpreadSheetView* view = this->GetView();
  if (view->GetNumberOfColumns() > section)
  {
    this->beginResetModel();
    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", this->ViewProxy);
    vtkSMPropertyHelper(this->ViewProxy, "ColumnToSort").Set(view->GetColumnName(section));
    switch (order)
    {
      case Qt::AscendingOrder:
        vtkSMPropertyHelper(this->ViewProxy, "InvertOrder").Set(1);
        break;
      case Qt::DescendingOrder:
        vtkSMPropertyHelper(this->ViewProxy, "InvertOrder").Set(0);
        break;
    }
    this->ViewProxy->UpdateVTKObjects();
    this->endResetModel();
  }
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetViewModel::isSortable(int section)
{
  vtkSpreadSheetView* view = this->GetView();
  if (view->GetNumberOfColumns() > section)
  {
    const char* columnName = view->GetColumnName(section);
    return columnName && strcmp(columnName, "Structured Coordinates") != 0;
  }

  return false;
}

//-----------------------------------------------------------------------------
QVariant pqSpreadSheetViewModel::headerData(
  int section, Qt::Orientation orientation, int role /*=Qt::DisplayRole*/) const
{
  if (orientation == Qt::Horizontal)
  {
    // No need to get updated data, simply get the current data.
    vtkSpreadSheetView* view = this->Internal->VTKView;
    if (view->GetNumberOfColumns() > section)
    {
      switch (role)
      {
        case Qt::DisplayRole:
          return QString(view->GetColumnLabel(section).c_str());
        case SectionInternal:
          return view->IsColumnInternal(section);
        case SectionVisible:
          return view->GetColumnVisibility(section);
      }
    }
    return QVariant();
  }
  else if (orientation == Qt::Vertical && role == Qt::DisplayRole)
  {
    // Row number to start from 0.
    QVariant rowNo = this->Superclass::headerData(section, orientation, role);
    return QVariant(rowNo.toUInt() - 1);
  }
  return this->Superclass::headerData(section, orientation, role);
}

//-----------------------------------------------------------------------------
QSet<pqSpreadSheetViewModel::vtkIndex> pqSpreadSheetViewModel::getVTKIndices(
  const QModelIndexList& indexes)
{
  // each variant in the vtkindices is 3 tuple
  // - (-1, pid, id) or
  // - (cid, pid, id) or
  // - (hlevel, hindex, id)
  QSet<vtkIndex> vtkindices;

  vtkSpreadSheetView* view = this->GetView();
  assert(view->GetShowExtractedSelection() == 0);

  vtkIdType numrows = view->GetNumberOfRows();
  foreach (QModelIndex idx, indexes)
  {
    vtkIdType row = idx.row();
    if (row >= numrows)
    {
      continue;
    }
    vtkIndex value;
    vtkVariant processId = view->GetValueByName(row, "vtkOriginalProcessIds");

    int pid = processId.IsValid() ? processId.ToInt() : -1;
    value.Tuple[1] = pid;

    vtkVariant cid = view->GetValueByName(row, "vtkCompositeIndexArray");
    if (cid.IsValid())
    {
      // cid is either a single value (as uint) or a pair of values.
      // In the latter case the vtkVariant returns it as a vtkAbstractArray.
      if (cid.IsArray())
      {
        vtkUnsignedIntArray* array = vtkUnsignedIntArray::SafeDownCast(cid.ToArray());
        assert(array->GetNumberOfTuples() * array->GetNumberOfComponents() == 2);
        value.Tuple[0] = static_cast<vtkIdType>(array->GetValue(0));
        value.Tuple[1] = static_cast<vtkIdType>(array->GetValue(1));
      }
      else
      {
        value.Tuple[0] = cid.ToUnsignedInt();
      }
    }

    vtkVariant vtkindex = view->GetValueByName(row, "vtkOriginalIndices");
    value.Tuple[2] = static_cast<vtkIdType>(vtkindex.ToLongLong());
    vtkindices.insert(value);
  }

  return vtkindices;
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetViewModel::isDataValid(const QModelIndex& idx) const
{
  if (this->getFieldType() != vtkDataObject::FIELD_ASSOCIATION_NONE)
  {
    return true;
  }

  // First make sure the index itself is valid
  pqDataRepresentation* repr = this->activeRepresentation();
  if (!idx.isValid() || repr == NULL)
  {
    return false;
  }

  // Ensure that the row of this index is less than the length of the
  // data array associated with its column
  if (idx.row() < this->Internal->VTKView->GetNumberOfRows())
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::resetCompositeDataSetIndex()
{
  if (!this->activeRepresentation())
  {
    return;
  }

  vtkSMProxy* reprProxy = this->activeRepresentation()->getProxy();
  int cur_index = vtkSMPropertyHelper(reprProxy, "CompositeDataSetIndex").GetAsInt();

  pqOutputPort* input_port = this->activeRepresentation()->getOutputPortFromInput();
  vtkSMSourceProxy* inputProxy =
    vtkSMSourceProxy::SafeDownCast(input_port->getSource()->getProxy());
  vtkSMSourceProxy* extractSelection = inputProxy->GetSelectionOutput(input_port->getPortNumber());
  if (!extractSelection)
  {
    return;
  }

  vtkPVDataInformation* mbInfo = extractSelection->GetDataInformation();
  if (!mbInfo || !mbInfo->GetCompositeDataClassName())
  {
    return;
  }

  vtkPVDataInformation* blockInfo = mbInfo->GetDataInformationForCompositeIndex(cur_index);
  if (blockInfo && blockInfo->GetNumberOfPoints() > 0)
  {
    return;
  }

  // find first index with non-empty points.
  vtkPVCompositeDataInformationIterator* iter = vtkPVCompositeDataInformationIterator::New();
  iter->SetDataInformation(mbInfo);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkPVDataInformation* curInfo = iter->GetCurrentDataInformation();
    if (!curInfo || curInfo->GetCompositeDataClassName() != 0)
    {
      continue;
    }
    if (curInfo->GetDataSetType() != -1 && curInfo->GetNumberOfPoints() > 0)
    {
      cur_index = static_cast<int>(iter->GetCurrentFlatIndex());
      break;
    }
  }
  iter->Delete();

  vtkSMPropertyHelper(reprProxy, "CompositeDataSetIndex").Set(cur_index);
  reprProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::setDecimalPrecision(int dPrecision)
{
  if (this->Internal->DecimalPrecision != dPrecision)
  {
    this->Internal->DecimalPrecision = dPrecision;
    this->forceUpdate();
  }
}

//-----------------------------------------------------------------------------
int pqSpreadSheetViewModel::getDecimalPrecision()
{
  return this->Internal->DecimalPrecision;
}
//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::setFixedRepresentation(bool fixed)
{
  if (this->Internal->FixedRepresentation != fixed)
  {
    this->Internal->FixedRepresentation = fixed;
    this->forceUpdate();
  }
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetViewModel::getFixedRepresentation()
{
  return this->Internal->FixedRepresentation;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqSpreadSheetViewModel::flags(const QModelIndex& idx) const
{
  return QAbstractTableModel::flags(idx) | Qt::ItemIsEditable;
}
//-----------------------------------------------------------------------------
bool pqSpreadSheetViewModel::setData(const QModelIndex&, const QVariant&, int)
{
  // Do nothing, we are not supposed to change our data...
  return false;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::hiddenColumnsChanged()
{
  const int numCols = this->columnCount();
  if (numCols > 0)
  {
    emit this->headerDataChanged(Qt::Horizontal, 0, this->columnCount() - 1);
  }
}
