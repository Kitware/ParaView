// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqTimelineView.h"

#include "pqActiveObjects.h"
#include "pqAnimatablePropertiesComboBox.h"
#include "pqAnimatableProxyComboBox.h"
#include "pqAnimationCue.h"
#include "pqRenderView.h"
#include "pqView.h"
#include "vtkPVCameraCueManipulator.h"
#include "vtkSMRenderViewProxy.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QToolButton>
#include <QVBoxLayout>

struct pqTimelineView::pqInternals
{
  pqTimelineView* Self;

  pqAnimatableProxyComboBox* ProxiesBox;
  pqAnimatablePropertiesComboBox* PropertiesBox;
  QToolButton* AddTrackButton;

  pqInternals(pqTimelineView* self)
    : Self(self)
  {
  }

  // create the toolbutton to reset Start and End time
  QWidget* createResetStartEndWidget()
  {
    auto resetBtn = new QToolButton(this->Self);
    resetBtn->setObjectName("resetStartEndTime");
    resetBtn->setToolTip(tr("Reset Start and End Time to default values"));
    resetBtn->setIcon(QIcon(":/pqWidgets/Icons/pqResetRange.svg"));

    QObject::connect(
      resetBtn, &QToolButton::pressed, this->Self, &pqTimelineView::resetStartEndTimeRequested);

    return resetBtn;
  }

  // create the widgets to add animation track
  QWidget* createAddTrackWidget()
  {
    auto addTrackWidget = new QWidget(this->Self);
    addTrackWidget->setObjectName("addTrackWidget");
    auto hLayout = new QHBoxLayout(addTrackWidget);
    hLayout->setContentsMargins(0, 0, 0, 0);

    this->ProxiesBox = new pqAnimatableProxyComboBox(addTrackWidget);
    this->ProxiesBox->setObjectName("proxiesBox");
    this->ProxiesBox->setToolTip(tr("Select proxy to animate"));
    this->ProxiesBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
#if VTK_MODULE_ENABLE_ParaView_pqPython
    this->ProxiesBox->addProxy(0, tr("Python"), nullptr);
#endif
    hLayout->addWidget(this->ProxiesBox);

    this->PropertiesBox = new pqAnimatablePropertiesComboBox(addTrackWidget);
    this->PropertiesBox->setObjectName("propertiesBox");
    this->PropertiesBox->setToolTip(tr("Select property to animate"));
    hLayout->addWidget(this->PropertiesBox);

    this->AddTrackButton = new QToolButton(addTrackWidget);
    this->AddTrackButton->setObjectName("addButton");
    this->AddTrackButton->setIcon(QIcon(":/QtWidgets/Icons/pqPlus.svg"));
    hLayout->addWidget(this->AddTrackButton);
    hLayout->addStretch();

    // Update proxy box with active view camera.
    QObject::connect(
      &pqActiveObjects::instance(), &pqActiveObjects::viewChanged, [&](pqView* view) {
        pqRenderView* rview = qobject_cast<pqRenderView*>(view);
        this->ProxiesBox->removeProxy(tr("Camera"));
        if (rview && this->ProxiesBox->findText(tr("Camera")) == -1)
        {
          this->ProxiesBox->addProxy(0, tr("Camera"), rview->getProxy());
        }
      });

    // set new pipeline source as current value for proxy box.
    QObject::connect(
      &pqActiveObjects::instance(), &pqActiveObjects::sourceChanged, [&](pqPipelineSource* source) {
        if (source)
        {
          int idx = this->ProxiesBox->findProxy(source->getProxy());
          if (idx != -1)
          {
            this->ProxiesBox->setCurrentIndex(idx);
          }
        }
      });

    // update properties box when proxy change. (do this after "Python" was added)
    QObject::connect(
      this->ProxiesBox, &pqAnimatableProxyComboBox::currentProxyChanged, [&](vtkSMProxy* proxy) {
        if (vtkSMRenderViewProxy::SafeDownCast(proxy))
        {
          this->PropertiesBox->setSourceWithoutProperties(proxy);
          // add camera animation modes as properties for creating the camera
          // animation track.
          this->PropertiesBox->addSMProperty(tr("Follow Path"),
            pqAnimationCue::getCameraModeName(vtkPVCameraCueManipulator::PATH), 0);
          this->PropertiesBox->addSMProperty(tr("Follow Data"),
            pqAnimationCue::getCameraModeName(vtkPVCameraCueManipulator::FOLLOW_DATA), 0);
          this->PropertiesBox->addSMProperty(tr("Interpolate cameras"),
            pqAnimationCue::getCameraModeName(vtkPVCameraCueManipulator::CAMERA), 0);
        }
        else
        {
          this->PropertiesBox->setSource(proxy);
        }
      });

    QObject::connect(this->ProxiesBox, &pqAnimatableProxyComboBox::currentTextChanged, [&]() {
      if (this->ProxiesBox->count() > 0)
      {
        Q_EMIT this->Self->validateTrackRequested(this->currentTrackCandidate());
      }
    });

    QObject::connect(
      this->PropertiesBox, &pqAnimatablePropertiesComboBox::currentTextChanged, [&]() {
        if (this->PropertiesBox->count() > 0)
        {
          Q_EMIT this->Self->validateTrackRequested(this->currentTrackCandidate());
        }
      });

    QObject::connect(AddTrackButton, &QToolButton::released,
      [&]() { Q_EMIT this->Self->newTrackRequested(this->currentTrackCandidate()); });

    return addTrackWidget;
  }

  // Return info describing the current state of Proxy/Property box.
  pqAnimatedPropertyInfo currentTrackCandidate()
  {
    pqAnimatedPropertyInfo anim;
    anim.Name = this->PropertiesBox->getCurrentPropertyName();
    // get proxy from properties box, so we get the representation proxy for display properties.
    anim.Proxy = this->PropertiesBox->getCurrentProxy();
    anim.Index = this->PropertiesBox->getCurrentIndex();
    return anim;
  }
};

//-----------------------------------------------------------------------------
pqTimelineView::pqTimelineView(QWidget* parent)
  : Superclass(parent)
  , Internals(new pqTimelineView::pqInternals(this))
{
  this->connect(this, &pqTimelineView::doubleClicked, [&](const QModelIndex& index) {
    if (index.data(pqTimelineItemRole::TYPE) == pqTimelineTrack::ANIMATION)
    {
      Q_EMIT this->editTrackRequested(index.data(pqTimelineItemRole::REGISTRATION_NAME).toString());
    }
  });
}

//-----------------------------------------------------------------------------
pqTimelineView::~pqTimelineView() = default;

//-----------------------------------------------------------------------------
pqTimelineModel* pqTimelineView::timelineModel()
{
  return dynamic_cast<pqTimelineModel*>(this->model());
}

//-----------------------------------------------------------------------------
void pqTimelineView::createRowWidgets(const QModelIndexList& indexes)
{
  if (indexes.size() < pqTimelineColumn::COUNT)
  {
    return;
  }

  auto trackButtonIndex = indexes[pqTimelineColumn::WIDGET];
  auto timelineIndex = indexes[pqTimelineColumn::TIMELINE];
  auto nameIndex = indexes[pqTimelineColumn::NAME];

  auto registrationName = nameIndex.data(pqTimelineItemRole::REGISTRATION_NAME).toString();
  auto type = timelineIndex.data(pqTimelineItemRole::TYPE).toInt();
  QVariant proxyVariant = timelineIndex.data(pqTimelineItemRole::PROXY);
  vtkSMProxy* proxy = reinterpret_cast<vtkSMProxy*>(proxyVariant.value<void*>());

  if (type == pqTimelineTrack::ANIMATION && proxy &&
    QString("TimeAnimationCue") != proxy->GetXMLName())
  {
    auto delButton = new QToolButton(this);
    delButton->setObjectName(QString("remove%1Button").arg(registrationName));
    delButton->setIcon(QIcon(":/QtWidgets/Icons/pqDelete.svg"));
    this->setIndexWidget(trackButtonIndex, delButton);

    QObject::connect(delButton, &QToolButton::released,
      [&, registrationName]() { Q_EMIT this->deleteTrackRequested(registrationName); });
  }

  switch (type)
  {
    case pqTimelineTrack::ANIMATION_HEADER:
      this->setIndexWidget(timelineIndex, this->Internals->createAddTrackWidget());
      break;
    case pqTimelineTrack::TIME:
      this->setIndexWidget(trackButtonIndex, this->Internals->createResetStartEndWidget());
      break;
    case pqTimelineTrack::ANIMATION:
    case pqTimelineTrack::SOURCE:
      this->expand(nameIndex.parent());
      break;
    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void pqTimelineView::updateTimelines()
{
  auto model = this->timelineModel();
  if (!model)
  {
    return;
  }

  for (auto rowIndexes : model->rows(pqTimelineTrack::SOURCE))
  {
    this->update(model->indexFromItem(rowIndexes[pqTimelineColumn::TIMELINE]));
  }
  for (auto rowIndexes : model->rows(pqTimelineTrack::ANIMATION))
  {
    this->update(model->indexFromItem(rowIndexes[pqTimelineColumn::TIMELINE]));
  }
  auto timeRow = model->rows(pqTimelineTrack::TIME).first();
  this->update(model->indexFromItem(timeRow[pqTimelineColumn::TIMELINE]));
}

//-----------------------------------------------------------------------------
void pqTimelineView::validateTrackCreationWidget()
{
  Q_EMIT this->validateTrackRequested(this->Internals->currentTrackCandidate());
}

//-----------------------------------------------------------------------------
void pqTimelineView::enableTrackCreationWidget(bool enable)
{
  this->Internals->AddTrackButton->setEnabled(enable);
  if (enable)
  {
    this->Internals->AddTrackButton->setToolTip(
      tr("Add animation cue for selected proxy and property."));
  }
  else
  {
    this->Internals->AddTrackButton->setToolTip(tr("Animation cue already exists."));
  }
}
