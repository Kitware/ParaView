/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ClientAttributeView.h"
#include "ui_ClientAttributeView.h"

#include <vtkConvertSelection.h>
#include <vtkDataArray.h>
#include <vtkDataArrayTemplate.h>
#include <vtkDataObjectToTable.h>
#include <vtkDataObjectTypes.h>
#include <vtkDataSetAttributes.h>
#include <vtkGraph.h>
#include <vtkIdList.h>
#include <vtkIdTypeArray.h>
#include <vtkPVDataInformation.h>
#include <vtkQtTableModelAdapter.h>
#include <vtkSMSelectionDeliveryRepresentationProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMSourceProxy.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>

#include <pqDataRepresentation.h>
#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqSelectionManager.h>
#include <pqServer.h>

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include <vtkstd/algorithm>
#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtksys/ios/sstream>
#include <vtksys/stl/set>

typedef vtkstd::multimap<vtkVariant, vtkIdType, vtkVariantLessThan > ValueToOriginalIndexMultiMap;
typedef vtkstd::multimap<vtkVariant, vtkIdType, vtkVariantLessThan >::iterator ValueToOriginalIndexMultiMapIterator;
typedef vtkstd::map<vtkVariant, int, vtkVariantLessThan > PedigreeIdToRowMap;
typedef vtkstd::map<vtkVariant, int, vtkVariantLessThan >::iterator PedigreeIdToRowMapIterator;

////////////////////////////////////////////////////////////////////////////////////
// ClientAttributeView::implementation

class ClientAttributeView::implementation
{
public:
  implementation() :
    Table(vtkTable::New()),
    ValuesArray(vtkSmartPointer<vtkVariantArray>::New()),
    UpdatingSelection(false)
  {
    this->TableModel.setColumnCount(4);
    this->TableModel.setRowCount(0);
    this->TableModel.setHorizontalHeaderLabels(QStringList() << "Values" << "Counts" << "Selected Counts" << "Selected");
    this->TableSort.setSourceModel(&this->TableModel);
  }

  ~implementation()
  {
    this->Table->Delete();
  }

  vtkTable* const Table;
  vtkSmartPointer<vtkVariantArray> ValuesArray;
  QStandardItemModel TableModel;
  QSortFilterProxyModel TableSort;
  int CurrentAttributeType;
  QString CurrentAttributeName;
  ValueToOriginalIndexMultiMap ValueToOriginalIndex;
  PedigreeIdToRowMap PedigreeIdToRow;
  bool UpdatingSelection;

  Ui::ClientAttributeView Widgets;
  QWidget Widget;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientAttributeView

ClientAttributeView::ClientAttributeView(
    const QString& viewmoduletype, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p) :
  pqSingleInputView(viewmoduletype, group, name, viewmodule, server, p),
  Implementation(new implementation())
{
  this->Implementation->Widgets.setupUi(&this->Implementation->Widget);
  this->Implementation->Widgets.tableView->setModel(&this->Implementation->TableSort);
  this->Implementation->Widgets.tableView->verticalHeader()->setVisible(false);  

  this->Implementation->Widgets.tableView->setColumnWidth(0, 150);
  this->Implementation->Widgets.tableView->setColumnWidth(1, 80);
  this->Implementation->Widgets.tableView->setColumnWidth(2, 100);

  this->Implementation->Widgets.tableView->sortByColumn(1, Qt::DescendingOrder);
  this->Implementation->Widgets.tableView->setColumnHidden(2, true);
  this->Implementation->Widgets.tableView->setColumnHidden(3, true);

  this->connect(this->Implementation->Widgets.tableView->selectionModel(), 
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), 
    this, SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));
}

ClientAttributeView::~ClientAttributeView()
{
  delete this->Implementation;
}

QWidget* ClientAttributeView::getWidget()
{
  return &this->Implementation->Widget;
}

bool ClientAttributeView::canDisplay(pqOutputPort* output_port) const
{
  if(!output_port)
    return false;

  pqPipelineSource* const source = output_port->getSource();
  if(!source)
    return false;

  if(this->getServer()->GetConnectionID() != source->getServer()->GetConnectionID())
    return false;

  vtkSMSourceProxy* source_proxy =
    vtkSMSourceProxy::SafeDownCast(source->getProxy());
  if (!source_proxy ||
     source_proxy->GetOutputPortsCreated() == 0)
    {
    return false;
    }

  const char* name = output_port->getDataClassName();
  int type = vtkDataObjectTypes::GetTypeIdFromClassName(name);
  switch(type)
    {    
    case VTK_DIRECTED_ACYCLIC_GRAPH:
    case VTK_DIRECTED_GRAPH:
    case VTK_GRAPH:
    case VTK_TREE:
    case VTK_UNDIRECTED_GRAPH:
    case VTK_TABLE:
      return true;
    }

  return false;
}

void ClientAttributeView::showRepresentation(pqRepresentation* representation)
{
  this->updateRepresentation(representation);
}

void ClientAttributeView::updateRepresentation(pqRepresentation* representation)
{
  vtkSMClientDeliveryRepresentationProxy* const proxy = representation?
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(representation->getProxy()) : NULL;

  if(!proxy)
    {
    return;
    }

  proxy->Update();

  int attributeType = QString(vtkSMPropertyHelper(proxy, "Attribute").GetAsString(3)).toInt();

  QString attributeTypeAsString;
  int fieldType;
  QString attributeName;
  vtkGraph *graph = vtkGraph::SafeDownCast(proxy->GetOutput());
  vtkTable *inputTable = vtkTable::SafeDownCast(proxy->GetOutput());
  if(graph)
    {
    if (attributeType == vtkDataObject::FIELD_ASSOCIATION_VERTICES)
      {
      fieldType = vtkDataObjectToTable::VERTEX_DATA;
      attributeName = vtkSMPropertyHelper(proxy, "Attribute").GetAsString(4);
      attributeTypeAsString = "Vertex Data";
      }
    else if(attributeType == vtkDataObject::FIELD_ASSOCIATION_EDGES)
      {
      fieldType = vtkDataObjectToTable::EDGE_DATA;
      attributeName = vtkSMPropertyHelper(proxy, "Attribute").GetAsString(4);
      attributeTypeAsString = "Edge Data";
      }
    else
      {
      return;
      }
    }
  else if(inputTable && attributeType == vtkDataObject::FIELD_ASSOCIATION_ROWS)
    {
    fieldType = vtkDataObjectToTable::FIELD_DATA;
    attributeName = vtkSMPropertyHelper(proxy, "Attribute").GetAsString(4);
    attributeTypeAsString = "Row Data";
    }
  else
    {
    return;
    }

  vtkDataObjectToTable *dataObjectToTable = vtkDataObjectToTable::New();
  dataObjectToTable->SetFieldType(fieldType);
  dataObjectToTable->SetInput(proxy->GetOutput());
  dataObjectToTable->Update();

  vtkTable *table = dataObjectToTable->GetOutput();
  if (!table)
    {
    return;
    }

  this->Implementation->Table->ShallowCopy(table);
  
  // Get pedigree ids for the table, if any
  vtkAbstractArray *pedigreeIds = this->Implementation->Table->GetRowData()->GetPedigreeIds();

/*
  bool countVis = true;
  bool selectedCountVis = false;
  if(this->Implementation->Widgets.tableView->horizontalHeader()->count())
    {
    countVis = !this->Local->UI.Table->isColumnHidden(1);
    selectedCountVis = !this->Local->UI.Table->isColumnHidden(2);
    }
*/
  QStandardItemModel& model = this->Implementation->TableModel;
  // Create the table model ...
  model.clear();
  model.setColumnCount(4);
  model.setHorizontalHeaderLabels(QStringList() << "Values" << "Counts" << "Selected Counts" << "Selected");
  const char *columnName = vtkSMPropertyHelper(proxy, "Attribute").GetAsString(4);

  if(vtkAbstractArray* const array = this->Implementation->Table->GetColumnByName(columnName))
    {    
    vtkIdList* list = vtkIdList::New();
    this->Implementation->ValuesArray->SetNumberOfTuples(0);
    this->Implementation->ValueToOriginalIndex.clear();
    this->Implementation->PedigreeIdToRow.clear();
    vtkIdType numTuples = array->GetNumberOfTuples();
    int row = 0;
    vtksys_stl::set<vtkVariant, vtkVariantLessThan> s;
    for(vtkIdType i = 0; i < numTuples; i++)
      {
      vtkVariant v, pid;
      switch(array->GetDataType())
        {
        vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(array->GetVoidPointer(i)));
        }
      this->Implementation->ValueToOriginalIndex.insert(vtkstd::pair< vtkVariant, vtkIdType >(v, i));

      if(pedigreeIds)
        {
        switch(pedigreeIds->GetDataType())
          {
          vtkExtraExtendedTemplateMacro(pid = *static_cast<VTK_TT*>(pedigreeIds->GetVoidPointer(i)));
          }
        this->Implementation->PedigreeIdToRow.insert(vtkstd::pair< vtkVariant, int >(pid, row));
        }

      if (s.find(v) == s.end())
        {
        s.insert(v);
        this->Implementation->ValuesArray->InsertNextValue(v);
        QStandardItem* countItem = new QStandardItem();
        array->LookupValue(v, list);
        qulonglong binc = list->GetNumberOfIds();
        countItem->setData(QVariant(binc),Qt::DisplayRole);
        model.appendRow(QList<QStandardItem*>() 
          << new QStandardItem(QString(v.ToString().c_str())) 
          << countItem 
          << new QStandardItem("0")
          << new QStandardItem(""));
        ++row;
        }
      }
    list->Delete();
    }

  //this->updateSelection(sel);

  this->Implementation->CurrentAttributeType = attributeType;
  this->Implementation->CurrentAttributeName = columnName;

  dataObjectToTable->Delete();

  this->Implementation->TableSort.setSourceModel(&this->Implementation->TableModel);

  this->Implementation->Widgets.tableView->sortByColumn(1, Qt::DescendingOrder);
  this->Implementation->Widgets.tableView->setColumnHidden(2, true);
  this->Implementation->Widgets.tableView->setColumnHidden(3, true);

  //this->Implementation->TableAdapter.reset();
  this->Implementation->Widgets.attributeTypeLabel->setText(attributeTypeAsString + QString(": "));
  this->Implementation->Widgets.attributeNameLabel->setText(columnName);
}

void ClientAttributeView::hideRepresentation(pqRepresentation* representation)
{
  this->Implementation->Table->Initialize();
  this->Implementation->TableModel.clear();
  this->Implementation->Widgets.attributeNameLabel->setText("None");
}

typedef vtkstd::map<vtkstd::string, unsigned long> BinsT;

template<typename value_type>
void ClientAttributeViewValue(BinsT& bins, vtkDataArray* array)
{
  for(int i = 0; i != array->GetNumberOfTuples(); ++i)
    {
    vtkstd::ostringstream buffer;
    buffer << array->GetTuple1(i);
    bins[buffer.str()] += 1;
    }
}

void ClientAttributeView::renderInternal()
{
  pqRepresentation* representation = this->visibleRepresentation();
  vtkSMSelectionDeliveryRepresentationProxy* const proxy = representation?
    vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(representation->getProxy()) : NULL;

  if(!proxy)
    {
    return;
    }

  proxy->Update();

  int attributeType = QString(vtkSMPropertyHelper(proxy, "Attribute").GetAsString(3)).toInt();
  QString attributeName = vtkSMPropertyHelper(proxy, "Attribute").GetAsString(4);

  if(attributeType != this->Implementation->CurrentAttributeType ||
     attributeName != this->Implementation->CurrentAttributeName)
    {
    this->updateRepresentation(representation);
    }

  proxy->GetSelectionRepresentation()->Update();
  vtkSelection* sel = vtkSelection::SafeDownCast(
    proxy->GetSelectionRepresentation()->GetOutput());

  this->updateSelection(sel);
}


template <typename T>
T vtkVariantNumericValue(const vtkVariant& v, T*)
{
  if (v.IsFloat())
    {
    return static_cast<T>(v.ToFloat());
    }
  if (v.IsDouble())
    {
    return static_cast<T>(v.ToDouble());
    }
  if (v.IsChar())
    {
    return static_cast<T>(v.ToChar());
    }
  if (v.IsUnsignedChar())
    {
    return static_cast<T>(v.ToUnsignedChar());
    }
  if (v.IsSignedChar())
    {
    return static_cast<T>(v.ToSignedChar());
    }
  if (v.IsShort())
    {
    return static_cast<T>(v.ToShort());
    }
  if (v.IsUnsignedShort())
    {
    return static_cast<T>(v.ToUnsignedShort());
    }
  if (v.IsInt())
    {
    return static_cast<T>(v.ToInt());
    }
  if (v.IsUnsignedInt())
    {
    return static_cast<T>(v.ToUnsignedInt());
    }
  if (v.IsLong())
    {
    return static_cast<T>(v.ToLong());
    }
  if (v.IsUnsignedLong())
    {
    return static_cast<T>(v.ToUnsignedLong());
    }
#if defined(VTK_TYPE_USE___INT64)
  if (v.Is__Int64())
    {
    return static_cast<T>(v.To__Int64());
    }
  if (v.IsUnsigned__Int64())
    {
    return static_cast<T>(static_cast<__int64>(v.ToUnsigned__Int64()));
    }
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
  if (v.IsLongLong())
    {
    return static_cast<T>(v.ToLongLong());
    }
  if (v.IsUnsignedLongLong())
    {
    return static_cast<T>(v.ToUnsignedLongLong());
    }
#endif
  return static_cast<T>(0);
}


void ClientAttributeView::onSelectionChanged(const QItemSelection&, const QItemSelection&)
{
  vtkAbstractArray* const values = this->Implementation->Table->GetColumnByName(this->Implementation->CurrentAttributeName.toAscii().data());
  if(!values)
    {
    return;
    }

  vtkAbstractArray *pedigreeIdArray = this->Implementation->Table->GetRowData()->GetPedigreeIds();
  if(!pedigreeIdArray)
    {
    return;
    }

  // Make a selection of values
  vtkSelection* selection = vtkSelection::New();
  selection->SetContentType(vtkSelection::SELECTIONS);
  selection->SetFieldType(vtkSelection::ROW);
  
  const QModelIndexList selectedIndices = this->Implementation->Widgets.tableView->selectionModel()->selectedRows();
  vtkAbstractArray *domainArray = this->Implementation->Table->GetColumnByName("domain");
  for (int i = 0; i < selectedIndices.size(); i++)
    {
    QModelIndex index = this->Implementation->TableSort.mapToSource(selectedIndices.at(i));
    vtkVariant  v = this->Implementation->ValuesArray->GetValue(static_cast<vtkIdType>(index.row()));
    vtkstd::pair<ValueToOriginalIndexMultiMapIterator, ValueToOriginalIndexMultiMapIterator> itp = this->Implementation->ValueToOriginalIndex.equal_range(v);

    for (ValueToOriginalIndexMultiMapIterator it = itp.first; it != itp.second; ++it)
      {
      QString domain;
      if(domainArray)
        {
        vtkVariant d;
        switch(domainArray->GetDataType())
          {
          vtkExtraExtendedTemplateMacro(d = *static_cast<VTK_TT*>(domainArray->GetVoidPointer(it->second)));
          }
        domain = d.ToString();
        }
      else
        {
        domain = pedigreeIdArray->GetName();
        }

      vtkSelection *childSelection = NULL;
      for(unsigned int j=0; j<selection->GetNumberOfChildren(); ++j)
        {
        vtkSelection *sel = selection->GetChild(j);
        if(domain == sel->GetSelectionList()->GetName())
          {
          childSelection = sel;
          break;
          }
        }

      if(!childSelection)
        {
        childSelection = vtkSelection::New();
        childSelection->SetContentType(vtkSelection::PEDIGREEIDS);
        childSelection->SetFieldType(vtkSelection::ROW);
        vtkVariantArray* childSelectionList = vtkVariantArray::New();
        childSelectionList->SetName(domain.toAscii().data());
        childSelection->SetSelectionList(childSelectionList);
        childSelectionList->Delete();
        selection->AddChild(childSelection);
        childSelection->Delete();
        }

      vtkVariant id;
      switch(pedigreeIdArray->GetDataType())
        {
        vtkExtraExtendedTemplateMacro(id = *static_cast<VTK_TT*>(pedigreeIdArray->GetVoidPointer(it->second)));
        }
      vtkVariantArray::SafeDownCast(childSelection->GetSelectionList())->InsertNextValue(id);
      }
    }

  this->Implementation->UpdatingSelection = true;

  // Get the representaion's source
  pqDataRepresentation* pqRepr =
    qobject_cast<pqDataRepresentation*>(this->visibleRepresentation());
  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* repSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());

  // Fill the selection source with the selection from the view
  vtkSMSourceProxy* selectionSource = pqSelectionManager::createSelectionSource(
    selection, repSource->GetConnectionID());

  // Set the selection on the representation's source
  repSource->SetSelectionInput(opPort->getPortNumber(),
    selectionSource, 0);

  selectionSource->Delete();
  selection->Delete();
}

void ClientAttributeView::updateSelection(vtkSelection *origSelection)
{
  if (this->Implementation->UpdatingSelection)
    {
    this->Implementation->UpdatingSelection = false;
    return;
    }
  
  if(!origSelection)
    {
    return;
    }

  vtkAbstractArray *pedigreeIdArray = this->Implementation->Table->GetRowData()->GetPedigreeIds();
  if(!pedigreeIdArray)
    {
    return;
    }

  int selType = -1;
  switch (this->Implementation->CurrentAttributeType)
    {
    case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
      selType = vtkSelection::VERTEX;
      break;
    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
      selType = vtkSelection::EDGE;
      break;
    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      selType = vtkSelection::ROW;
      break;
    }

  if(selType < 0)
    return;
  
  // Does the selection have a compatible field type?
  vtkSelection* selection = 0;
  if (origSelection && origSelection->GetContentType() == vtkSelection::SELECTIONS)
    {
    vtkSelection* child = NULL;
    for (unsigned int i = 0; i < origSelection->GetNumberOfChildren(); i++)
      {
      child = origSelection->GetChild(i);
      if (child && selType != vtkSelection::SELECTIONS) 
        {
        selection = vtkSelection::New();
        selection->ShallowCopy(child);
        break;
        }
      }
    if(!selection && child)
      {
      /// Use the last valid child selection
      selection = vtkSelection::New();
      selection->ShallowCopy(child);
      }
    }
  else
    {
    selection = vtkSelection::New();
    selection->ShallowCopy(origSelection);
    }
  
  if(!selection || selection->GetContentType() != vtkSelection::PEDIGREEIDS)
    {
    // Did not find a selection with the same field type
    return;
    }

  // We need to convert this to a field selection since we are extracting from a table.
  selection->SetFieldType(vtkSelection::ROW);
  vtkSelection* valuesSel = vtkConvertSelection::ToValueSelection(
      selection, 
      this->Implementation->Table, 
      this->Implementation->CurrentAttributeName.toAscii().data());
  vtkAbstractArray* arr = valuesSel->GetSelectionList();
  
  // Clear the selection column
  int rows = this->Implementation->TableModel.rowCount();
  for (int r = 0; r < rows; r++)
    {
    this->Implementation->TableModel.setItem(r, 2, new QStandardItem("0"));
    this->Implementation->TableModel.setItem(r, 3, new QStandardItem(""));
    }
  
  QItemSelection list;
  for (vtkIdType i = 0; i < arr->GetNumberOfTuples(); i++)
    {
    vtkVariant v(0);
    switch (arr->GetDataType())
      {
      vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(arr->GetVoidPointer(i)));
      }
    
    vtkIdType row = this->Implementation->ValuesArray->LookupValue(v);

    if (row >= 0)
      {
      this->Implementation->TableModel.setItem(row, 3, new QStandardItem("1"));
      QModelIndex index = this->Implementation->TableModel.index(row, 3);
      QModelIndex sortIndex = this->Implementation->TableSort.mapFromSource(index);
      list.select(sortIndex, sortIndex);

      int selectedCount = this->Implementation->TableModel.item(row, 2)->text().toInt();
      this->Implementation->TableModel.setItem(row, 2, new QStandardItem(QString::number(++selectedCount)));
      }
    }
  
  // Clear the proxy model since we have updated data.
  this->Implementation->TableSort.clear();
  // Sort by the selected column to bring selected items to the top.
  this->Implementation->Widgets.tableView->sortByColumn(3, Qt::DescendingOrder);
  
  this->disconnect(this->Implementation->Widgets.tableView->selectionModel(), 
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), 
    this, SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));

  this->Implementation->Widgets.tableView->selectionModel()->select(list, 
    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

  this->connect(this->Implementation->Widgets.tableView->selectionModel(), 
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), 
    this, SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));

  if (list.size() > 0)
    {
    this->Implementation->Widgets.tableView->scrollToTop();
    }

  valuesSel->Delete();
  selection->Delete();

}
