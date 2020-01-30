/*=========================================================================

   Program: ParaView
   Module:    pqMultiQueryClauseWidget.cxx

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
#include "pqMultiQueryClauseWidget.h"

#include "pqEventDispatcher.h"
#include "pqOutputPort.h"
#include "pqQueryClauseWidget.h"
#include "pqServer.h"

#include "vtkDataObject.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"

#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqMultiQueryClauseWidget::pqMultiQueryClauseWidget(QWidget* parentObject, Qt::WindowFlags _flags)
  : Superclass(parentObject, _flags)
{
  this->setLayout(new QVBoxLayout());
  this->layout()->setContentsMargins(0, 0, 0, 0);

  this->ScrollArea = new QScrollArea(this);
  this->ScrollArea->setWidgetResizable(true);
  this->ScrollArea->setFrameShape(QFrame::NoFrame);

  this->Container = new QWidget(this->ScrollArea);
  this->Container->setLayout(new QVBoxLayout());
  this->Container->layout()->setContentsMargins(5, 5, 5, 5);

  this->ScrollArea->setWidget(this->Container);
  dynamic_cast<QVBoxLayout*>(this->layout())->addWidget(this->ScrollArea);

  this->ChildNextId = 0;
  this->NumberOfDependentClauseWidgets = 0;
  this->AddingClauseWidget = false;
}

//-----------------------------------------------------------------------------
bool pqMultiQueryClauseWidget::isAMR()
{
  vtkPVDataInformation* dataInfo = this->producer()->getDataInformation();
  switch (dataInfo->GetCompositeDataSetType())
  {
    case VTK_HIERARCHICAL_BOX_DATA_SET:
    case VTK_HIERARCHICAL_DATA_SET:
    case VTK_UNIFORM_GRID_AMR:
    case VTK_NON_OVERLAPPING_AMR:
    case VTK_OVERLAPPING_AMR:
      return true;
    default:
      return false;
  }
}

//-----------------------------------------------------------------------------
bool pqMultiQueryClauseWidget::isMultiBlock()
{
  vtkPVDataInformation* dataInfo = this->producer()->getDataInformation();
  return dataInfo->GetCompositeDataSetType() == VTK_MULTIBLOCK_DATA_SET;
}

//-----------------------------------------------------------------------------
void pqMultiQueryClauseWidget::initialize()
{
  if (this->AddingClauseWidget)
  {
    return;
  }

  for (pqQueryClauseWidget* child : this->Container->findChildren<pqQueryClauseWidget*>())
  {
    delete child;
  }
  this->ChildNextId = 0;
  this->updateDependentClauseWidgets();

  this->addQueryClauseWidget();
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqMultiQueryClauseWidget::newSelectionSource()
{
  // Find the proper Proxy manager
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  // Create a new selection source proxy based on the criteria_type.
  vtkSMProxy* selSource = pxm->NewProxy("sources", "SelectionQuerySource");

  // * Determine FieldType.
  int field_type = 0;
  switch (this->attributeType())
  {
    case vtkDataObject::POINT:
      field_type = vtkSelectionNode::POINT;
      break;

    case vtkDataObject::CELL:
      field_type = vtkSelectionNode::CELL;
      break;

    case vtkDataObject::ROW:
      field_type = vtkSelectionNode::ROW;
      break;

    case vtkDataObject::VERTEX:
      field_type = vtkSelectionNode::VERTEX;
      break;

    case vtkDataObject::EDGE:
      field_type = vtkSelectionNode::EDGE;
      break;
  }
  vtkSMPropertyHelper(selSource, "FieldType").Set(field_type);

  for (pqQueryClauseWidget* child : this->Container->findChildren<pqQueryClauseWidget*>())
  {
    child->addSelectionQualifiers(selSource);
  }

  std::string fieldTypeString;
  switch (field_type)
  {
    case vtkSelectionNode::CELL:
      fieldTypeString = "CELL";
      break;
    case vtkSelectionNode::POINT:
      fieldTypeString = "POINT";
      break;
    case vtkSelectionNode::FIELD:
      fieldTypeString = "FIELD";
      break;
    case vtkSelectionNode::VERTEX:
      fieldTypeString = "VERTEX";
      break;
    case vtkSelectionNode::EDGE:
      fieldTypeString = "EDGE";
      break;
    case vtkSelectionNode::ROW:
      fieldTypeString = "ROW";
      break;
    default:
      break;
  }

  SM_SCOPED_TRACE(CallFunction)
    .arg("QuerySelect")
    .arg("QueryString", vtkSMPropertyHelper(selSource, "QueryString").GetAsString())
    .arg("FieldType", fieldTypeString.c_str())
    .arg("InsideOut", vtkSMPropertyHelper(selSource, "InsideOut").GetAsInt())
    .arg("comment", "create a query selection");

  selSource->UpdateVTKObjects();
  return selSource;
}

//-----------------------------------------------------------------------------
void pqMultiQueryClauseWidget::addQueryClauseWidget()
{
  this->addQueryClauseWidget(pqQueryClauseWidget::ANY, false);
}

//-----------------------------------------------------------------------------
void pqMultiQueryClauseWidget::addQueryClauseWidget(int type, bool qualifier_mode)
{
  this->AddingClauseWidget = true;
  pqQueryClauseWidget* queryWidget = new pqQueryClauseWidget(this, this->Container);
  queryWidget->initialize(pqQueryClauseWidget::CriteriaType(type), qualifier_mode);
  queryWidget->setObjectName(QString("queryClause%1").arg(QString::number(this->ChildNextId)));
  dynamic_cast<QVBoxLayout*>(this->Container->layout())->addWidget(queryWidget);

  this->ChildNextId++;

  this->connect(queryWidget, SIGNAL(destroyed()), SLOT(onChildrenChanged()));
  this->connect(queryWidget, SIGNAL(addQueryRequested()), SLOT(addQueryClauseWidget()));

  this->onChildrenChanged();
  pqEventDispatcher::processEventsAndWait(1);

  this->ScrollArea->ensureWidgetVisible(queryWidget);
  this->AddingClauseWidget = false;
}

//-----------------------------------------------------------------------------
void pqMultiQueryClauseWidget::onChildrenChanged()
{
  auto queries = this->Container->findChildren<pqQueryClauseWidget*>();
  if (queries.empty())
  {
    return;
  }

  // always see 3 query max.
  auto firstWidget = queries.first();
  int h = std::min(queries.size(), 3) * (firstWidget->minimumSizeHint().height() + 10);
  this->ScrollArea->setMinimumSize(100, h);

  int firstQueryIndex = this->NumberOfDependentClauseWidgets;
  int lastQueryIndex = queries.size() - 1;

  if (lastQueryIndex < firstQueryIndex)
  {
    return;
  }

  queries.at(lastQueryIndex)->setAddButtonVisible(true);
  queries.at(firstQueryIndex)->setAndLabelVisible(false);

  if (firstQueryIndex == lastQueryIndex)
  {
    queries.at(firstQueryIndex)->setRemoveButtonVisible(false);
  }
  else
  {
    queries.at(firstQueryIndex)->setRemoveButtonVisible(true);
    queries.at(lastQueryIndex - 1)->setAddButtonVisible(false);
    queries.at(lastQueryIndex)->setAndLabelVisible(true);
  }
}

//-----------------------------------------------------------------------------
void pqMultiQueryClauseWidget::updateDependentClauseWidgets()
{
  QList<pqQueryClauseWidget::CriteriaTypes> dependentClausesTypes;

  if (this->isMultiBlock())
  {
    dependentClausesTypes.push_back(pqQueryClauseWidget::BLOCK);
  }

  if (this->isAMR())
  {
    dependentClausesTypes.push_back(pqQueryClauseWidget::AMR_LEVEL);
    dependentClausesTypes.push_back(pqQueryClauseWidget::AMR_BLOCK);
    dependentClausesTypes.push_back(pqQueryClauseWidget::AMR_BLOCK);
    dependentClausesTypes.push_back(pqQueryClauseWidget::AMR_LEVEL);
  }

  this->NumberOfDependentClauseWidgets = dependentClausesTypes.length();
  for (pqQueryClauseWidget::CriteriaTypes types : dependentClausesTypes)
  {
    this->addQueryClauseWidget(types, true);
  }
}

//-----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation* pqMultiQueryClauseWidget::getChosenAttributeInfo() const
{
  vtkPVDataInformation* dataInfo = this->producer()->getDataInformation();
  return dataInfo->GetAttributeInformation(this->attributeType());
}
