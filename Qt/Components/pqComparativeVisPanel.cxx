/*=========================================================================

   Program: ParaView
   Module:    pqComparativeVisPanel.cxx

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
#include "pqComparativeVisPanel.h"
#include "ui_pqComparativeVisPanel.h"

// Server Manager Includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkPVComparativeAnimationCue.h"
#include "vtkProcessModule.h"
#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMVectorProperty.h"
#include "vtkSmartPointer.h"

// Qt Includes.
#include <QHeaderView>
#include <QPointer>
#include <QScrollArea>
#include <QtDebug>

// ParaView Includes.
#include "pqActiveObjects.h"
#include "pqAnimationCue.h"
#include "pqAnimationKeyFrame.h"
#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptors.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"
#include "pqView.h"

#include <cassert>

class pqComparativeVisPanel::pqInternal : public Ui::pqComparativeVisPanel
{
public:
  QPointer<pqView> View;
  pqPropertyLinks Links;
};

namespace pqComparativeVisPanelNS
{
enum
{
  PROXY = Qt::UserRole,
  PROPERTY_NAME = PROXY + 1,
  PROPERTY_INDEX = PROPERTY_NAME + 1,
  CUE_PROXY = PROPERTY_INDEX + 1
};

QString getName(vtkSMProxy* proxy)
{
  pqServerManagerModel* smmmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqProxy* pq_proxy = smmmodel->findItem<pqProxy*>(proxy);
  if (pq_proxy)
  {
    return pq_proxy->getSMName();
  }

  // proxy could be a helper proxy of some other proxy
  QString helper_key;
  pq_proxy = pqProxy::findProxyWithHelper(proxy, helper_key);
  if (pq_proxy)
  {
    vtkSMProperty* prop = pq_proxy->getProxy()->GetProperty(helper_key.toLocal8Bit().data());
    if (prop)
    {
      return QString("%1 - %2").arg(pq_proxy->getSMName()).arg(prop->GetXMLLabel());
    }
    return pq_proxy->getSMName();
  }
  return "<unrecognized-proxy>";
}

QString getName(vtkSMProxy* proxy, const char* pname, int index)
{
  vtkSMVectorProperty* smproperty = vtkSMVectorProperty::SafeDownCast(proxy->GetProperty(pname));
  if (!smproperty)
  {
    return "<unrecognized-property>";
  }

  unsigned int num_elems = smproperty->GetNumberOfElements();
  if (smproperty->GetRepeatCommand())
  {
    num_elems = 1;
  }

  if (num_elems == 1 || index == -1)
  {
    return smproperty->GetXMLLabel();
  }
  return QString("%1 (%2)").arg(smproperty->GetXMLLabel()).arg(index);
}

QTableWidgetItem* newItem(vtkSMProxy* proxy, const char* pname, int index)
{
  QTableWidgetItem* item = new QTableWidgetItem();
  item->setData(PROXY, QVariant::fromValue<pqSMProxy>(pqSMProxy(proxy)));
  item->setData(PROPERTY_NAME, pname);
  item->setData(PROPERTY_INDEX, index);
  if (proxy)
  {
    item->setText(QString("%1:%2").arg(getName(proxy), getName(proxy, pname, index)));
  }
  else
  {
    item->setText("Time");
  }
  return item;
}

vtkSMProxy* newCue(vtkSMProxy* proxy, const char* pname, int index)
{
  pqServer* activeServer = pqActiveObjects::instance().activeServer();
  vtkSMSessionProxyManager* pxm = activeServer->proxyManager();

  // Create a new cueProxy.
  vtkSMProxy* cueProxy = pxm->NewProxy("animation", "ComparativeAnimationCue");

  vtkSMPropertyHelper(cueProxy, "AnimatedPropertyName").Set(pname);
  vtkSMPropertyHelper(cueProxy, "AnimatedElement").Set(index);
  vtkSMPropertyHelper(cueProxy, "AnimatedProxy").Set(proxy);

  // Setup default value.
  if (proxy)
  {
    QList<QVariant> domain = pqSMAdaptor::getMultipleElementPropertyDomain(
      proxy->GetProperty(pname), index >= 0 ? index : 0);
    double curValue = 0.0;
    if (index != -1)
    {
      curValue = vtkSMPropertyHelper(proxy, pname).GetAsDouble(index);
    }
    else if (vtkSMPropertyHelper(proxy, pname).GetNumberOfElements() > 0)
    {
      curValue = vtkSMPropertyHelper(proxy, pname).GetAsDouble(0);
    }
    double minValue = curValue;
    double maxValue = curValue;
    if (domain.size() >= 1 && domain[0].isValid())
    {
      minValue = domain[0].toDouble();
    }
    if (domain.size() >= 2 && domain[1].isValid())
    {
      maxValue = domain[1].toDouble();
    }
    vtkSMComparativeAnimationCueProxy::SafeDownCast(cueProxy)->UpdateWholeRange(minValue, maxValue);
  }
  if (!proxy)
  {
    pqTimeKeeper* timekeeper = activeServer->getTimeKeeper();
    QPair<double, double> range = timekeeper->getTimeRange();
    // this is a "Time" animation cue. Use the range provided by the time
    // keeper.
    vtkSMComparativeAnimationCueProxy::SafeDownCast(cueProxy)->UpdateWholeRange(
      range.first, range.second);
  }
  cueProxy->UpdateVTKObjects();
  pxm->RegisterProxy("comparative_cues", cueProxy->GetGlobalIDAsString(), cueProxy);
  return cueProxy;
}
};

//-----------------------------------------------------------------------------
pqComparativeVisPanel::pqComparativeVisPanel(QWidget* p)
  : Superclass(p)
{
  this->VTKConnect = vtkEventQtSlotConnect::New();

  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->activeParameters->horizontalHeader()->setSectionResizeMode(
    QHeaderView::ResizeToContents);

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(setView(pqView*)));
  this->setView(pqActiveObjects::instance().activeView());

  QObject::connect(this->Internal->addParameter, SIGNAL(clicked()), this, SLOT(addParameter()));

  QObject::connect(this->Internal->proxyCombo, SIGNAL(currentProxyChanged(vtkSMProxy*)),
    this->Internal->propertyCombo, SLOT(setSource(vtkSMProxy*)));
  this->Internal->propertyCombo->setSource(this->Internal->proxyCombo->getCurrentProxy());

  this->Internal->proxyCombo->addProxy(0, "Time", NULL);

  QObject::connect(this->Internal->activeParameters,
    SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)), this,
    SLOT(parameterSelectionChanged()));

  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()), this, SLOT(sizeUpdated()));
  QObject::connect(&this->Internal->Links, SIGNAL(smPropertyChanged()), this, SLOT(sizeUpdated()));
  QObject::connect(this->Internal->activeParameters->verticalHeader(), SIGNAL(sectionClicked(int)),
    this, SLOT(removeParameter(int)));

  // for now, no automatic labelling.
  this->Internal->showParameterLabels->hide();
}

//-----------------------------------------------------------------------------
pqComparativeVisPanel::~pqComparativeVisPanel()
{
  this->VTKConnect->Disconnect();
  this->VTKConnect->Delete();
  this->VTKConnect = 0;
  delete this->Internal;
  this->Internal = 0;
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::setView(pqView* _view)
{
  if (this->Internal->View == _view)
  {
    return;
  }

  if (this->Internal->View)
  {
    QObject::disconnect(
      this->Internal->cueWidget, SIGNAL(valuesChanged()), this->Internal->View, SLOT(render()));
  }

  this->Internal->Links.removeAllPropertyLinks();
  this->VTKConnect->Disconnect();
  this->Internal->View = _view;
  this->Internal->activeParameters->clearContents();

  vtkSMComparativeViewProxy* viewProxy =
    _view ? vtkSMComparativeViewProxy::SafeDownCast(_view->getProxy()) : NULL;

  // View must be a comparative render/plot view
  if (viewProxy == NULL)
  {
    this->Internal->View = 0;
    this->setEnabled(false);
    return;
  }

  QObject::connect(
    this->Internal->cueWidget, SIGNAL(valuesChanged()), this->Internal->View, SLOT(render()));

  this->setEnabled(true);

  // Connect layoutX spinbox value to vtkSMComparativeViewProxy's "Dimensions" property
  this->Internal->Links.addPropertyLink(this->Internal->layoutX, "value", SIGNAL(editingFinished()),
    viewProxy, viewProxy->GetProperty("Dimensions"), 0);

  // Connect layoutY spinbox value to vtkSMComparativeViewProxy's "Dimensions" property
  this->Internal->Links.addPropertyLink(this->Internal->layoutY, "value", SIGNAL(editingFinished()),
    viewProxy, viewProxy->GetProperty("Dimensions"), 1);

  // Connect layoutY spinbox value to vtkSMComparativeViewProxy's "Dimensions" property
  this->Internal->Links.addPropertyLink(this->Internal->overlay, "checked", SIGNAL(toggled(bool)),
    viewProxy, viewProxy->GetProperty("OverlayAllComparisons"), 1);

  this->VTKConnect->Connect(
    viewProxy->GetProperty("Cues"), vtkCommand::ModifiedEvent, this, SLOT(updateParametersList()));

  this->updateParametersList();
}

//-----------------------------------------------------------------------------
pqView* pqComparativeVisPanel::view() const
{
  return this->Internal->View;
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::updateParametersList()
{
  this->Internal->activeParameters->clearContents();

  vtkSMPropertyHelper cues(this->view()->getProxy(), "Cues");
  this->Internal->activeParameters->setRowCount(static_cast<int>(cues.GetNumberOfElements()));

  for (unsigned int cc = 0; cc < cues.GetNumberOfElements(); cc++)
  {
    vtkSMPropertyHelper animatedProxyHelper(cues.GetAsProxy(cc), "AnimatedProxy");
    vtkSMProxy* curProxy =
      animatedProxyHelper.GetNumberOfElements() > 0 ? animatedProxyHelper.GetAsProxy() : NULL;
    const char* pname =
      vtkSMPropertyHelper(cues.GetAsProxy(cc), "AnimatedPropertyName").GetAsString();
    int pindex = vtkSMPropertyHelper(cues.GetAsProxy(cc), "AnimatedElement").GetAsInt();

    QTableWidgetItem* item = pqComparativeVisPanelNS::newItem(curProxy, pname, pindex);
    item->setData(pqComparativeVisPanelNS::CUE_PROXY,
      QVariant::fromValue<pqSMProxy>(pqSMProxy(cues.GetAsProxy(cc))));
    this->Internal->activeParameters->setItem(static_cast<int>(cc), 0, item);

    QTableWidgetItem* headerItem =
      new QTableWidgetItem(QIcon(":/QtWidgets/Icons/pqDelete.svg"), "");
    this->Internal->activeParameters->setVerticalHeaderItem(static_cast<int>(cc), headerItem);
  }

  this->Internal->activeParameters->setCurrentItem(
    this->Internal->activeParameters->item(cues.GetNumberOfElements() - 1, 0),
    QItemSelectionModel::ClearAndSelect);
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::addParameter()
{
  // Need to create new cue for this property.
  // vtkSMProxy* curProxy = this->Internal->proxyCombo->getCurrentProxy();
  vtkSMProxy* realProxy = this->Internal->propertyCombo->getCurrentProxy();
  // Note: curProxy may not be same as realProxy!  This happens for cases when
  // we're animating a "helper" proxy e.g. "Slice Type - Origin" for the
  // Slice filter.
  QString pname = this->Internal->propertyCombo->getCurrentPropertyName();
  int pindex = this->Internal->propertyCombo->getCurrentIndex();

  int row = this->findRow(realProxy, pname, pindex);
  if (row != -1)
  {
    // already exists, select it.
    this->Internal->activeParameters->setCurrentItem(
      this->Internal->activeParameters->item(row, 0), QItemSelectionModel::ClearAndSelect);
    return;
  }

  if (realProxy)
  {
    BEGIN_UNDO_SET(
      QString("Add parameter %1 : %2")
        .arg(pqComparativeVisPanelNS::getName(realProxy))
        .arg(pqComparativeVisPanelNS::getName(realProxy, pname.toLocal8Bit().data(), pindex)));
  }
  else
  {
    BEGIN_UNDO_SET("Add parameter Time");
  }

  // Add new cue.
  vtkSMProxy* cueProxy =
    pqComparativeVisPanelNS::newCue(realProxy, pname.toLocal8Bit().data(), pindex);
  vtkSMPropertyHelper(this->view()->getProxy(), "Cues").Add(cueProxy);
  cueProxy->Delete();
  this->view()->getProxy()->UpdateVTKObjects();
  END_UNDO_SET();

  this->Internal->View->render();
}

//-----------------------------------------------------------------------------
int pqComparativeVisPanel::findRow(
  vtkSMProxy* animatedProxy, const QString& animatedPName, int animatedIndex)
{
  for (int cc = 0; cc < this->Internal->activeParameters->rowCount(); cc++)
  {
    QTableWidgetItem* item = this->Internal->activeParameters->item(cc, 0);
    if (item->data(pqComparativeVisPanelNS::PROXY).value<pqSMProxy>().GetPointer() ==
        animatedProxy &&
      item->data(pqComparativeVisPanelNS::PROPERTY_NAME) == animatedPName &&
      item->data(pqComparativeVisPanelNS::PROPERTY_INDEX) == animatedIndex)
    {
      return cc;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::parameterSelectionChanged()
{
  QTableWidgetItem* activeItem = this->Internal->activeParameters->currentItem();
  if (activeItem)
  {
    vtkSMProxy* cue =
      activeItem->data(pqComparativeVisPanelNS::CUE_PROXY).value<pqSMProxy>().GetPointer();
    this->Internal->cueGroup->setTitle(activeItem->text());
    this->Internal->cueWidget->setCue(cue);
    this->Internal->multivalueHint->setVisible(this->Internal->cueWidget->acceptsMultipleValues());
  }
  else
  {
    this->Internal->cueGroup->setTitle("[Select Parameter]");
    this->Internal->cueWidget->setCue(NULL);
    this->Internal->multivalueHint->hide();
  }
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::sizeUpdated()
{
  this->Internal->cueWidget->setSize(
    this->Internal->layoutX->value(), this->Internal->layoutY->value());

  this->Internal->View->render();
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::removeParameter(int index)
{
  if (index < 0 || index >= this->Internal->activeParameters->rowCount())
  {
    qWarning() << "Invalid index: " << index;
    return;
  }

  QTableWidgetItem* item = this->Internal->activeParameters->item(index, 0);
  assert(item);

  BEGIN_UNDO_SET("Remove Parameter");

  vtkSMSessionProxyManager* pxm = this->view()->proxyManager();
  vtkSmartPointer<vtkSMProxy> cue =
    item->data(pqComparativeVisPanelNS::CUE_PROXY).value<pqSMProxy>().GetPointer();
  item = NULL;

  vtkSMPropertyHelper(this->view()->getProxy(), "Cues").Remove(cue);
  this->view()->getProxy()->UpdateVTKObjects();

  const char* proxy_name = pxm->GetProxyName("comparative_cues", cue);
  if (proxy_name)
  {
    pxm->UnRegisterProxy("comparative_cues", proxy_name, cue);
  }
  END_UNDO_SET();

  this->Internal->View->render();
}

//-----------------------------------------------------------------------------
/*
void pqComparativeVisPanel::setTimeRangeFromSource(vtkSMProxy* source)
{
  if (!source || !this->Internal->View)
    {
    return;
    }

  // Get TimeRange property
  vtkSMDoubleVectorProperty* timeRangeProp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Internal->View->getProxy()->GetProperty("TimeRange"));

  // Try to get TimestepValues property from the source proxy
  vtkSMDoubleVectorProperty* tsv = vtkSMDoubleVectorProperty::SafeDownCast(
    source->GetProperty("TimestepValues"));

  // Set the TimeRange to the first and last of TimestepValues.
  if (tsv && timeRangeProp && tsv->GetNumberOfElements())
    {
    double tBegin = tsv->GetElement(0);
    double tEnd = tsv->GetElement(tsv->GetNumberOfElements()-1);
    timeRangeProp->SetElement(0, tBegin);
    timeRangeProp->SetElement(1, tEnd);
    this->Internal->View->getProxy()->UpdateProperty("TimeRange");
    }
}
*/

//-----------------------------------------------------------------------------
/*
void pqComparativeVisPanel::activateCue(
  vtkSMProperty* cuesProperty,
  vtkSMProxy* animatedProxy, const QString& animatedPName, int animatedIndex)
{
  if (!cuesProperty || !animatedProxy || animatedPName.isEmpty())
    {
    return;
    }

  // Try to locate the cue, if already present.
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(cuesProperty);
  vtkSmartPointer<vtkSMAnimationCueProxy> cueProxy;

  for (unsigned int cc=0; cc < pp->GetNumberOfProxies(); ++cc)
    {
    vtkSMAnimationCueProxy* cur = vtkSMAnimationCueProxy::SafeDownCast(
      pp->GetProxy(cc));
    if (cur && cur->GetAnimatedProxy() == animatedProxy &&
      cur->GetAnimatedPropertyName() == animatedPName &&
      cur->GetAnimatedElement() == animatedIndex)
      {
      cueProxy = cur;
      }
    else if (cur)
      {
      pqSMAdaptor::setElementProperty(cur->GetProperty("Enabled"), 0);
      cur->UpdateVTKObjects();
      }
    }

  if (!cueProxy)
    {
    vtkSMProxyManager *pxm = vtkSMProxyManager::GetProxyManager();

    // Create a new cueProxy.
    cueProxy.TakeReference(
      vtkSMAnimationCueProxy::SafeDownCast(pxm->NewProxy("animation", "KeyFrameAnimationCue")));
    cueProxy->SetServers(vtkProcessModule::CLIENT);
    cueProxy->SetConnectionID(this->Internal->View->getProxy()->GetConnectionID());

    pqSMAdaptor::setElementProperty(cueProxy->GetProperty("AnimatedPropertyName"),
      animatedPName);
    pqSMAdaptor::setElementProperty(cueProxy->GetProperty("AnimatedElement"),
      animatedIndex);
    pqSMAdaptor::setProxyProperty(cueProxy->GetProperty("AnimatedProxy"),
        animatedProxy);

    // This cueProxy must be registered so that state works fine. For that
    // purpose we just make it an helper of the view.
    this->Internal->View->addHelperProxy("AnimationCues", cueProxy);

    // We want to add default keyframes to this cue.
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    pqAnimationCue* pqcue = smmodel->findItem<pqAnimationCue*>(cueProxy);
    pqcue->insertKeyFrame(0);
    pqcue->insertKeyFrame(1);
    }

  pqSMAdaptor::addProxyProperty(cuesProperty, cueProxy);
  pqSMAdaptor::setElementProperty(cueProxy->GetProperty("Enabled"), 1);
  cueProxy->UpdateVTKObjects();
}
*/
