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

#include "ClientRecordView.h"
#include "ui_ClientRecordView.h"

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
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
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

////////////////////////////////////////////////////////////////////////////////////
// ClientRecordView::implementation

class ClientRecordView::implementation
{
public:
  implementation() :
    Table(vtkTable::New()),
    RowIndex(0)
  {
  }

  ~implementation()
  {
    this->Table->Delete();
  }

  vtkTable* const Table;
  int CurrentAttributeType;
  vtkIdType RowIndex;

  Ui::ClientRecordView Widgets;
  QWidget Widget;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientRecordView

ClientRecordView::ClientRecordView(
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
}

ClientRecordView::~ClientRecordView()
{
  delete this->Implementation;
}

QWidget* ClientRecordView::getWidget()
{
  return &this->Implementation->Widget;
}

bool ClientRecordView::canDisplay(pqOutputPort* output_port) const
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

void ClientRecordView::showRepresentation(pqRepresentation* representation)
{
  this->updateRepresentation(representation);
}

void ClientRecordView::updateRepresentation(pqRepresentation* representation)
{
  vtkSMSelectionDeliveryRepresentationProxy* const proxy = representation?
    vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(representation->getProxy()) : NULL;

  if(!proxy)
    {
    return;
    }

  int attributeType = QString(vtkSMPropertyHelper(proxy, "AttributeType").GetAsString(3)).toInt();

  int fieldType;
  vtkGraph *graph = vtkGraph::SafeDownCast(proxy->GetOutput());
  vtkTable *inputTable = vtkTable::SafeDownCast(proxy->GetOutput());
  if(graph)
    {
    if (attributeType == vtkDataObject::FIELD_ASSOCIATION_VERTICES)
      {
      fieldType = vtkDataObjectToTable::VERTEX_DATA;
      }
    else if (attributeType == vtkDataObject::FIELD_ASSOCIATION_EDGES)
      {
      fieldType = vtkDataObjectToTable::EDGE_DATA;
      }
    else
      {
      return;
      }
    }
  else if(inputTable && attributeType == vtkDataObject::FIELD_ASSOCIATION_ROWS)
    {
    fieldType = vtkDataObjectToTable::FIELD_DATA;
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
  this->Implementation->CurrentAttributeType = attributeType;

  proxy->GetSelectionRepresentation()->Update();
  vtkSelection* sel = vtkSelection::SafeDownCast(
    proxy->GetSelectionRepresentation()->GetOutput());

  this->updateSelection(sel);

  // Display text data ...
  const vtkIdType row_count = table->GetNumberOfRows();
  const vtkIdType column_count = table->GetNumberOfColumns();

  if(row_count && column_count && this->Implementation->RowIndex>=0)
    {
    vtkIdType row_index = this->Implementation->RowIndex % row_count;
    while(row_index < 0)
      row_index += row_count;

    QString html;
    for(vtkIdType i = 0; i != column_count; ++i)
      {
      html += "<b>" + QString(table->GetColumnName(i)) + ":</b> ";
      html += table->GetValue(row_index, i).ToString().c_str();
      html += "<br>\n";
      }
      
    this->Implementation->Widgets.body->setHtml(html);
    }
  else
    {
    this->Implementation->Widgets.body->setPlainText("");
    }

  dataObjectToTable->Delete();

}

void ClientRecordView::hideRepresentation(pqRepresentation* representation)
{
  this->Implementation->Table->Initialize();
  this->Implementation->Widgets.body->setPlainText("");
}

void ClientRecordView::renderInternal()
{
  pqRepresentation* representation = this->visibleRepresentation();
  vtkSMClientDeliveryRepresentationProxy* const proxy = representation?
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(representation->getProxy()) : NULL;

  if(!proxy)
    {
    return;
    }

  proxy->Update();

  this->updateRepresentation(representation);
}

void ClientRecordView::updateSelection(vtkSelection *origSelection)
{
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
      selType = vtkSelectionNode::VERTEX;
      break;
    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
      selType = vtkSelectionNode::EDGE;
      break;
    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      selType = vtkSelectionNode::ROW;
      break;
    }
  
  if(selType < 0)
    return;

  // Does the selection have a compatible field type?
  vtkSelectionNode* selection = 0;
  if (origSelection)
    {
    vtkSelectionNode* node = NULL;
    for (unsigned int i = 0; i < origSelection->GetNumberOfNodes(); i++)
      {
      node = origSelection->GetNode(i);
      if (node && selType == node->GetFieldType()) 
        {
        selection = vtkSelectionNode::New();
        selection->ShallowCopy(node);
        break;
        }
      }
    if(!selection && node)
      {
      /// Use the last valid selection node
      selection = vtkSelectionNode::New();
      selection->ShallowCopy(node);
      }
    }
  
  if(!selection || selection->GetContentType() != vtkSelectionNode::PEDIGREEIDS)
    {
    // Did not find a selection with the same field type
    return;
    }

  vtkAbstractArray *arr = selection->GetSelectionList();
  vtkVariant v;
  switch (arr->GetDataType())
    {
    vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(arr->GetVoidPointer(0)));
    }
  
  this->Implementation->RowIndex = pedigreeIdArray->LookupValue(v);

/*
  selection->SetFieldType(vtkSelectionNode::ROW);
  vtkSelection *indexSelection = 
      vtkConvertSelection::ToIndexSelection(selection, this->Implementation->Table);
  vtkIdTypeArray *idxList = 
      vtkIdTypeArray::SafeDownCast(indexSelection->GetSelectionList());

  if(idxList && idxList->GetNumberOfTuples()>=0)
    {
    this->Implementation->RowIndex = idxList->GetValue(0);
    }

  indexSelection->Delete();
*/

  selection->Delete();
}
