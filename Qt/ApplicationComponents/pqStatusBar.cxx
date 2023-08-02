// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqStatusBar.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqProgressManager.h"
#include "pqProgressWidget.h"
#include "vtkPVMemoryUseInformation.h"
#include "vtkPVSystemConfigInformation.h"
#include "vtkSMSession.h"

#include <QProgressBar>
#include <QStyleFactory>
#include <QToolButton>

#include <iostream>

//-----------------------------------------------------------------------------
pqStatusBar::pqStatusBar(QWidget* parentObject)
  : Superclass(parentObject)
{
  pqProgressManager* progress_manager = pqApplicationCore::instance()->getProgressManager();

  // Progress bar/button management
  pqProgressWidget* const progress_bar = new pqProgressWidget(this);
  progress_manager->addNonBlockableObject(progress_bar);
  progress_manager->addNonBlockableObject(progress_bar->abortButton());

  QObject::connect(
    progress_manager, SIGNAL(enableProgress(bool)), progress_bar, SLOT(enableProgress(bool)));

  QObject::connect(progress_manager, SIGNAL(progress(const QString&, int)), progress_bar,
    SLOT(setProgress(const QString&, int)));

  QObject::connect(
    progress_manager, SIGNAL(enableAbort(bool)), progress_bar, SLOT(enableAbort(bool)));

  QObject::connect(progress_bar, SIGNAL(abortPressed()), progress_manager, SLOT(triggerAbort()));

  this->MemoryProgressBar = new QProgressBar(this);
  this->MemoryProgressBar->setAlignment(Qt::AlignCenter);

  // Force to fusion or cleanlooks style
  // to make sure that the formatted text is displayed
  QStyle* style = QStyleFactory::create("fusion");
  if (!style)
  {
    style = QStyleFactory::create("cleanlooks");
  }
  if (style)
  {
    this->MemoryProgressBar->setStyle(style);
  }

  auto& activeObjects = pqActiveObjects::instance();
  // Update the server configuration information only on server connection
  this->connect(&activeObjects, SIGNAL(serverChanged(pqServer*)), SLOT(updateServerConfigInfo()));

  // Update the memory bar only when needed, not faster than once every 300ms.
  QTimer* timer = new QTimer(this);
  timer->setSingleShot(true);
  timer->setInterval(300);
  this->connect(timer, SIGNAL(timeout()), SLOT(updateMemoryProgressBar()));

  this->connect(&activeObjects, SIGNAL(dataUpdated()), timer, SLOT(start()));
  this->connect(&activeObjects, SIGNAL(viewUpdated()), timer, SLOT(start()));

  // Final ui setup
  this->addPermanentWidget(progress_bar);
  this->addPermanentWidget(this->MemoryProgressBar);
}

//-----------------------------------------------------------------------------
pqStatusBar::~pqStatusBar() = default;

//-----------------------------------------------------------------------------
void pqStatusBar::updateServerConfigInfo()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    return;
  }
  auto session = server->session();
  if (!session)
  {
    return;
  }
  session->GatherInformation(vtkPVSession::SERVERS, this->ServerConfigsInfo, 0);
  this->updateMemoryProgressBar();
}

//-----------------------------------------------------------------------------
void pqStatusBar::updateMemoryProgressBar()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    return;
  }
  auto session = server->session();
  if (!session)
  {
    return;
  }
  if (session->GetPendingProgress() || server->isProcessingPending())
  {
    return;
  }

  vtkNew<vtkPVMemoryUseInformation> infos;
  session->GatherInformation(vtkPVSession::SERVERS, infos, 0);

  assert(infos->GetSize() == this->ServerConfigsInfo->GetSize());

  struct HostData
  {
    long long HostMemoryUse = 0;
    long long HostMemoryAvailable = 0;
    std::string HostName = "undefined";
  };

  auto worstFraction = -1.0;
  HostData worstServer;

  for (size_t rank = 0; rank < infos->GetSize(); ++rank)
  {
    long long const hostMemoryUse = infos->GetHostMemoryUse(rank);
    long long const hostMemoryAvailable = this->ServerConfigsInfo->GetHostMemoryAvailable(rank);
    if (hostMemoryAvailable <= 0)
    {
      continue;
    }
    double const fraction = static_cast<double>(hostMemoryUse) / hostMemoryAvailable;
    if (fraction > worstFraction)
    {
      worstServer.HostMemoryUse = hostMemoryUse;
      worstServer.HostMemoryAvailable = hostMemoryAvailable;
      worstServer.HostName = this->ServerConfigsInfo->GetHostName(rank);
      worstFraction = fraction;
    }
  }

  this->MemoryProgressBar->setMinimum(0);
  this->MemoryProgressBar->setMaximum(worstServer.HostMemoryAvailable);
  this->MemoryProgressBar->setValue(worstServer.HostMemoryUse);
  this->MemoryProgressBar->setFormat(
    QString{ "%1: %2/%3 %4%" }
      .arg(QString::fromStdString(worstServer.HostName))
      .arg(pqCoreUtilities::formatMemoryFromKiBValue(worstServer.HostMemoryUse, 1))
      .arg(pqCoreUtilities::formatMemoryFromKiBValue(worstServer.HostMemoryAvailable, 1))
      .arg(worstFraction * 100.0, 0, 'f', 1));
  auto palette = this->MemoryProgressBar->palette();
  if (worstFraction > 0.9)
  {
    pqCoreUtilities::setPaletteHighlightToCritical(palette);
  }
  else if (worstFraction > 0.75)
  {
    pqCoreUtilities::setPaletteHighlightToWarning(palette);
  }
  else
  {
    pqCoreUtilities::setPaletteHighlightToOk(palette);
  }
  this->MemoryProgressBar->setPalette(palette);
}
