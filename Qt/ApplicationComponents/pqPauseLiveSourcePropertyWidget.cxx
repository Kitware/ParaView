// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPauseLiveSourcePropertyWidget.h"
#include "ui_pqPauseLiveSourcePropertyWidget.h"

#include "pqLiveSourceItem.h"
#include "pqLiveSourceManager.h"
#include "pqPVApplicationCore.h"
#include "vtkPVXMLElement.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

class pqPauseLiveSourcePropertyWidget::pqInternals
{
public:
  Ui::PauseLiveSourcePropertyWidget Ui;

  void updatePauseButtonState()
  {
    bool isPaused = pqPVApplicationCore::instance()->liveSourceManager()->isEmulatedTimePaused();
    QString iconName = isPaused ? "pqVcrPlay" : "pqVcrPause";
    this->Ui.EmulatedTimePauseButton->setIcon(QIcon(":/pqWidgets/Icons/" + iconName + ".svg"));
    QString toolTip = isPaused ? tr("Play") : tr("Pause");
    this->Ui.EmulatedTimePauseButton->setToolTip(toolTip + tr(" emulated time sources."));
  }
};

//-----------------------------------------------------------------------------
pqPauseLiveSourcePropertyWidget::pqPauseLiveSourcePropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqPauseLiveSourcePropertyWidget::pqInternals())

{
  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);

  internals.Ui.PauseButton->setText(
    QCoreApplication::translate("ServerManagerXML", smproperty->GetXMLLabel()));

  this->setShowLabel(false);

  QObject::connect(internals.Ui.PauseButton, &QPushButton::clicked, this,
    &pqPauseLiveSourcePropertyWidget::onClicked);

  pqLiveSourceManager* lvManager = pqPVApplicationCore::instance()->liveSourceManager();
  pqLiveSourceItem* lvItem = lvManager->getLiveSourceItem(smproxy);
  QObject::connect(
    lvItem, &pqLiveSourceItem::stateChanged, internals.Ui.PauseButton, &QPushButton::setChecked);
  internals.Ui.PauseButton->setChecked(lvItem->isPaused());

  if (lvItem->isEmulatedTimeAlgorithm())
  {
    QObject::connect(internals.Ui.EmulatedTimePauseButton, &QPushButton::clicked, []() {
      auto manager = pqPVApplicationCore::instance()->liveSourceManager();
      bool paused = manager->isEmulatedTimePaused();
      if (paused)
      {
        manager->resumeEmulatedTime();
      }
      else
      {
        manager->pauseEmulatedTime();
      }
    });

    auto updateState = [&internals]() { internals.updatePauseButtonState(); };
    QObject::connect(internals.Ui.EmulatedTimePauseButton, &QPushButton::clicked, updateState);
    QObject::connect(lvManager, &pqLiveSourceManager::emulatedTimeStateChanged,
      internals.Ui.EmulatedTimePauseButton, updateState);
    internals.updatePauseButtonState();

    QObject::connect(internals.Ui.EmulatedTimeResetButton, &QPushButton::clicked, []() {
      auto manager = pqPVApplicationCore::instance()->liveSourceManager();
      manager->setEmulatedCurrentTime(0.);
    });
  }
  else
  {
    internals.Ui.EmulatedTimeControlsGroupBox->setVisible(false);
  }
}

//-----------------------------------------------------------------------------
pqPauseLiveSourcePropertyWidget::~pqPauseLiveSourcePropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqPauseLiveSourcePropertyWidget::onClicked(bool checked)
{
  pqLiveSourceManager* manager = pqPVApplicationCore::instance()->liveSourceManager();
  pqLiveSourceItem* lvItem = manager->getLiveSourceItem(this->proxy());
  lvItem->blockSignals(true);
  if (checked)
  {
    lvItem->pause();
  }
  else
  {
    lvItem->resume();
  }
  lvItem->blockSignals(false);
}
