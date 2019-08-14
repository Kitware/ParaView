/*=========================================================================

   Program: ParaView
   Module:  pqCompositeTreePropertyWidget.cxx

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
#include "pqCompositeTreePropertyWidget.h"

#include "pqCompositeDataInformationTreeModel.h"
#include "pqTreeView.h"
#include "pqTreeViewSelectionHelper.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMIntVectorProperty.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QSignalBlocker>

#include <cassert>

namespace
{
template <class L, class T>
void push_back(L& list, const T& item)
{
  list.push_back(item);
}

template <class L, class T1, class T2>
void push_back(L& list, const QPair<T1, T2>& item)
{
  list.push_back(item.first);
  list.push_back(item.second);
}

template <class T>
QList<QVariant> convertToVariantList(const QList<T>& val)
{
  QList<QVariant> ret;
  foreach (const T& avalue, val)
  {
    push_back(ret, avalue);
  }
  return ret;
}

QList<unsigned int> convertToUIntList(const QList<QVariant>& val)
{
  QList<unsigned int> ret;
  foreach (const QVariant& avalue, val)
  {
    ret.push_back(avalue.value<unsigned int>());
  }
  return ret;
}

QList<QPair<unsigned int, unsigned int> > convertToUIntPairList(const QList<QVariant>& val)
{
  QList<QPair<unsigned int, unsigned int> > ret;
  for (int cc = 0, max = val.size(); cc + 1 < max; cc += 2)
  {
    ret.push_back(QPair<unsigned int, unsigned int>(
      val[cc].value<unsigned int>(), val[cc + 1].value<unsigned int>()));
  }
  return ret;
}
}

//-----------------------------------------------------------------------------
pqCompositeTreePropertyWidget::pqCompositeTreePropertyWidget(
  vtkSMIntVectorProperty* smproperty, vtkSMProxy* smproxy, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Property(smproperty)
{
  this->setShowLabel(false);
  this->setChangeAvailableAsChangeFinished(true);

  auto ctd = smproperty->FindDomain<vtkSMCompositeTreeDomain>();
  assert(ctd);
  this->Domain = ctd;

  this->VTKConnect->Connect(ctd, vtkCommand::DomainModifiedEvent, &this->Timer, SLOT(start()));
  this->connect(&this->Timer, SIGNAL(timeout()), SLOT(domainModified()));
  this->Timer.setSingleShot(true);
  this->Timer.setInterval(0);

  pqTreeView* treeView = new pqTreeView(this);
  this->TreeView = treeView;
  treeView->setObjectName("TreeWidget");

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->setMargin(0);

  pqCompositeDataInformationTreeModel* dmodel = new pqCompositeDataInformationTreeModel(this);
  dmodel->setUserCheckable(true);
  dmodel->setHeaderData(0, Qt::Horizontal, smproperty->GetXMLLabel());

  // If property takes multiple values, then user can select multiple items at a
  // time. If not, then tell the model that it should uncheck other subtrees
  // when a user clicks on an item.
  dmodel->setExclusivity(smproperty->GetRepeatCommand() == 0);
  if (dmodel->exclusivity())
  {
    // if exclusive, are we limiting to selecting 1 leaf node?
    dmodel->setOnlyLeavesAreUserCheckable(ctd->GetMode() == vtkSMCompositeTreeDomain::LEAVES);
  }

  if (ctd->GetMode() == vtkSMCompositeTreeDomain::AMR &&
    ((smproperty->GetRepeatCommand() == 1 && smproperty->GetNumberOfElementsPerCommand() == 2) ||
        (smproperty->GetRepeatCommand() == 0 && smproperty->GetNumberOfElements() == 2)))
  {
    dmodel->setExpandMultiPiece(true);
  }
  this->Model = dmodel;
  this->domainModified();

  treeView->setUniformRowHeights(true);
  treeView->header()->setStretchLastSection(true);
  treeView->setRootIsDecorated(true);
  treeView->setModel(dmodel);

  if (vtkPVXMLElement* hints = smproperty->GetHints())
  {
    if (vtkPVXMLElement* elem = hints->FindNestedElementByName("WidgetHeight"))
    {
      int row_count = 0;
      if (elem->GetScalarAttribute("number_of_rows", &row_count))
      {
        treeView->setMaximumRowCountBeforeScrolling(row_count);
        vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
          "widget height limited to %d rows using `WidgetHeight` hint.", row_count);
      }
    }
    if (vtkPVXMLElement* elem = hints->FindNestedElementByName("Expansion"))
    {
      elem->GetScalarAttribute("depth", &this->DepthExpansion);
    }
  }
  treeView->expandToDepth(this->DepthExpansion);

  pqTreeViewSelectionHelper* helper = new pqTreeViewSelectionHelper(treeView);
  helper->setObjectName("CompositeTreeSelectionHelper");
  hbox->addWidget(treeView);

  this->addPropertyLink(this, "values", SIGNAL(valuesChanged()), smproperty);
  this->connect(
    dmodel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), SIGNAL(valuesChanged()));
}

//-----------------------------------------------------------------------------
pqCompositeTreePropertyWidget::~pqCompositeTreePropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqCompositeTreePropertyWidget::domainModified()
{
  if (this->Model && this->Domain)
  {
    const QSignalBlocker blocker(this);

    // calling reset will change the check state for all node. Hence, we do
    // this.
    QList<QVariant> oldValue = this->values();
    bool isComposite = this->Model->reset(this->Domain->GetInformation());
    this->setValues(oldValue);
    this->TreeView->expandToDepth(this->DepthExpansion);

    // this ensures that the widget is hidden if the data is not a composite
    // dataset.
    this->TreeView->setVisible(isComposite);
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqCompositeTreePropertyWidget::values() const
{
  assert(this->Model && this->Property && this->Domain);
  switch (this->Domain->GetMode())
  {
    case vtkSMCompositeTreeDomain::ALL:
    case vtkSMCompositeTreeDomain::NON_LEAVES:
      return convertToVariantList(this->Model->checkedNodes());

    case vtkSMCompositeTreeDomain::LEAVES:
      return convertToVariantList(this->Model->checkedLeaves());

    case vtkSMCompositeTreeDomain::AMR:
      if ((this->Property->GetRepeatCommand() == 1 &&
            this->Property->GetNumberOfElementsPerCommand() == 2) ||
        (this->Property->GetRepeatCommand() == 0 && this->Property->GetNumberOfElements() == 2))
      {
        return convertToVariantList(this->Model->checkedLevelDatasets());
      }
      else
      {
        return convertToVariantList(this->Model->checkedLevels());
      }
  }
  return QList<QVariant>();
}

//-----------------------------------------------------------------------------
void pqCompositeTreePropertyWidget::setValues(const QList<QVariant>& values)
{
  assert(this->Model && this->Property && this->Domain);
  switch (this->Domain->GetMode())
  {
    case vtkSMCompositeTreeDomain::ALL:
    case vtkSMCompositeTreeDomain::NON_LEAVES:
    case vtkSMCompositeTreeDomain::LEAVES:
      this->Model->setChecked(convertToUIntList(values));
      break;

    case vtkSMCompositeTreeDomain::AMR:
      if ((this->Property->GetRepeatCommand() &&
            this->Property->GetNumberOfElementsPerCommand() == 2) ||
        this->Property->GetNumberOfElements() == 2)
      {
        this->Model->setCheckedLevelDatasets(convertToUIntPairList(values));
      }
      else
      {
        this->Model->setCheckedLevels(convertToUIntList(values));
      }
      break;
  }
}
