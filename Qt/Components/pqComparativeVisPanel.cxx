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
#include "vtkProcessModule.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMDoubleVectorProperty.h"

// Qt Includes.
#include <QHeaderView>
#include <QPointer>
#include <QScrollArea>

// ParaView Includes.
#include "pqActiveView.h"
#include "pqAnimationCue.h"
#include "pqAnimationKeyFrame.h"
#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"
#include "pqApplicationCore.h"
#include "pqComparativeChartView.h"
#include "pqComparativeRenderView.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"

class pqComparativeVisPanel::pqInternal : public Ui::ComparativeView
{
public:
  QPointer<pqView> View;
  pqPropertyLinks Links;
  pqSignalAdaptorComboBox* ModeAdaptor;
  pqSignalAdaptorComboBox* ViewUpdateModeAdaptor;
};

//-----------------------------------------------------------------------------
pqComparativeVisPanel::pqComparativeVisPanel(QWidget* p):Superclass(p)
{
  this->Internal = new pqInternal();

  /* TODO: Why does this never work for me :( ?
  QVBoxLayout* vboxlayout = new QVBoxLayout(this);
  vboxlayout->setSpacing(0);
  vboxlayout->setMargin(0);
  vboxlayout->setObjectName("vboxLayout");

  QWidget* container = new QWidget(this);
  container->setObjectName("scrollWidget");
  container->setSizePolicy(QSizePolicy::MinimumExpanding,
    QSizePolicy::MinimumExpanding);

  QScrollArea* s = new QScrollArea(this);
  s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setWidgetResizable(true);
  s->setObjectName("scrollArea");
  s->setFrameShape(QFrame::NoFrame);
  s->setWidget(container);
  vboxlayout->addWidget(s);
  this->Internal->setupUi(container);
  */

  this->Internal->setupUi(this);
  this->Internal->ModeAdaptor = 
    new pqSignalAdaptorComboBox(this->Internal->Mode);
  this->Internal->ViewUpdateModeAdaptor = 
    new pqSignalAdaptorComboBox(this->Internal->ViewUpdateMode);
  this->Internal->Links.setUseUncheckedProperties(false);

  // When mode changes we want to hide the non-related GUI components.
  QObject::connect(
    this->Internal->ModeAdaptor, SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(modeChanged(const QString&)), Qt::QueuedConnection);
  this->Internal->YAxisGroup->setVisible(false);

  // Call updateView when "Update" pushbutton is clicked.
  QObject::connect(this->Internal->Update, SIGNAL(clicked()),
    this, SLOT(updateView()), Qt::QueuedConnection);

  // FIXME: move connection to pqMainWindowCore.
  QObject::connect(&pqActiveView::instance(), SIGNAL(changed(pqView*)),
    this, SLOT(setView(pqView*)));

  //this->Internal->XObject->setUpdateCurrentWithSelection(true);
  //this->Internal->YObject->setUpdateCurrentWithSelection(true);
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();

  // Add source to pqSourceComboBox when source is added to pipeline
  QObject::connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)),
    this->Internal->XObject, SLOT(addSource(pqPipelineSource*)));
  QObject::connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)),
    this->Internal->YObject, SLOT(addSource(pqPipelineSource*)));

  // Remove source from pqSourceComboBox when source is remove from pipeline
  QObject::connect(smmodel, SIGNAL(preSourceRemoved(pqPipelineSource*)),
    this->Internal->XObject, SLOT(removeSource(pqPipelineSource*)));
  QObject::connect(smmodel, SIGNAL(preSourceRemoved(pqPipelineSource*)),
    this->Internal->YObject, SLOT(removeSource(pqPipelineSource*)));

  // Set pqAnimatablePropertiesComboBox when pqSourceComboBox changes
  QObject::connect(
    this->Internal->XObject, SIGNAL(currentIndexChanged(vtkSMProxy*)),
    this->Internal->XProperty, SLOT(setSource(vtkSMProxy*)));
  QObject::connect(
    this->Internal->YObject, SIGNAL(currentIndexChanged(vtkSMProxy*)),
    this->Internal->YProperty, SLOT(setSource(vtkSMProxy*)));

  // Call propertyChanged() when pqAnimatablePropertiesComboBox changes
  QObject::connect(
    this->Internal->XProperty, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(xpropertyChanged()));
  QObject::connect(
    this->Internal->YProperty, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(ypropertyChanged()));

  this->Internal->XProperty->setUseBlankEntry(true);
  this->Internal->YProperty->setUseBlankEntry(true);
  this->setEnabled(false);
}

//-----------------------------------------------------------------------------
pqComparativeVisPanel::~pqComparativeVisPanel()
{
  delete this->Internal->ModeAdaptor;
  delete this->Internal->ViewUpdateModeAdaptor;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::setView(pqView* view)
{

  if (this->Internal->View == view)
    {
    return;
    }

  this->Internal->Links.removeAllPropertyLinks();
  this->Internal->View = view;
  this->Internal->AnimationWidget->setComparativeView(
    view? view->getProxy() : 0);

  // View must be a comparative render/plot view
  if (   !qobject_cast<pqComparativeRenderView*>(view)
      && !qobject_cast<pqComparativeChartView*>(view))
    {
    this->Internal->View = 0;
    this->setEnabled(false);
    return;
    }

  vtkSMComparativeViewProxy* viewProxy =
    vtkSMComparativeViewProxy::SafeDownCast(view->getProxy());

  this->setEnabled(true);

  // Connect XFrames spinbox value to vtkSMComparativeViewProxy's "Dimensions" property
  this->Internal->Links.addPropertyLink(
    this->Internal->XFrames, "value", SIGNAL(valueChanged(int)),
    viewProxy, viewProxy->GetProperty("Dimensions"), 0);

  // Connect YFrames spinbox value to vtkSMComparativeViewProxy's "Dimensions" property
  this->Internal->Links.addPropertyLink(
    this->Internal->YFrames, "value", SIGNAL(valueChanged(int)),
    viewProxy, viewProxy->GetProperty("Dimensions"), 1);

  // Connect show timesteps check button
  this->Internal->Links.addPropertyLink(
    this->Internal->ShowTimeStepsCheck, "checked", SIGNAL(stateChanged(int)),
    viewProxy, viewProxy->GetProperty("ShowTimeSteps"), 1);

  // Connect mode combobox to vtkSMComparativeViewProxy's "Mode" property
  this->Internal->Links.addPropertyLink(
    this->Internal->ModeAdaptor, "currentText", 
    SIGNAL(currentTextChanged(const QString&)),
    viewProxy, viewProxy->GetProperty("Mode"));

  // Connect combobox to vtkSMComparativeViewProxy's "ViewUpdateMode" property
  this->Internal->Links.addPropertyLink(
    this->Internal->ViewUpdateModeAdaptor, "currentText", 
    SIGNAL(currentTextChanged(const QString&)),
    viewProxy, viewProxy->GetProperty("ViewUpdateMode"));
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::updateView()
{

  if (this->Internal->View)
    {
    this->Internal->Links.accept();

    // This could be handled differently, but for now lets
    // set the timerange to the currently selected source object's
    // TimestepValues (if it has them)
    if (this->Internal->XObject->currentSource())
      {
      this->setTimeRangeFromSource(this->Internal->XObject->currentSource()->getProxy());
      }

    vtkSMComparativeViewProxy* viewProxy =
      vtkSMComparativeViewProxy::SafeDownCast(this->Internal->View->getProxy());

    // Call UpdateVisualization with 1 to force the update
    viewProxy->UpdateVisualization(1);
    }
}


//-----------------------------------------------------------------------------
void pqComparativeVisPanel::modeChanged(const QString& mode)
{
  if (mode == "Film Strip")
    {
    this->Internal->YAxisGroup->setVisible(false);
    }
  else
    {
    this->Internal->YAxisGroup->setVisible(true);
    }
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::xpropertyChanged()
{
  if (this->Internal->View)
    {
    // Locate the X animation cue for this property (if none exists, a new one will
    // be created) and make it the only enabled cue in the XCues property.
    vtkSMProxy* proxy = this->Internal->XProperty->getCurrentProxy();
    QString pname = this->Internal->XProperty->getCurrentPropertyName();
    int index = this->Internal->XProperty->getCurrentIndex();

    // Locate cue for the selected property.
    this->activateCue(
      this->Internal->View->getProxy()->GetProperty("XCues"),
      proxy, pname, index);
    this->Internal->View->getProxy()->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::ypropertyChanged()
{
  if (this->Internal->View)
    {
    // Locate the Y animation cue for this property (if none exists, a new one will
    // be created) and make it the only enabled cue in the YCues property.
    vtkSMProxy* proxy = this->Internal->YProperty->getCurrentProxy();
    QString pname = this->Internal->YProperty->getCurrentPropertyName();
    int index = this->Internal->YProperty->getCurrentIndex();

    // Locate cue for the selected property.
    this->activateCue(
      this->Internal->View->getProxy()->GetProperty("YCues"),
      proxy, pname, index);
    this->Internal->View->getProxy()->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

