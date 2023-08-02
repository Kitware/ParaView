// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqSingleLogViewerWidget.h"

#include "QCloseEvent"

#include "vtkPVLogInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"

//----------------------------------------------------------------------------
pqSingleLogViewerWidget::pqSingleLogViewerWidget(
  QWidget* parent, vtkSmartPointer<vtkSMProxy> logRecorderProxy, int rank)
  : Superclass(parent)
  , LogRecorderProxy(logRecorderProxy)
  , Rank(rank)
{
  vtkSMPropertyHelper(logRecorderProxy, "RankEnabled").Set(rank);
  logRecorderProxy->UpdateProperty("RankEnabled", 1);
}

//----------------------------------------------------------------------------
void pqSingleLogViewerWidget::refresh()
{
  vtkNew<vtkPVLogInformation> logInformation;
  logInformation->SetRank(this->Rank);
  this->LogRecorderProxy->GatherInformation(logInformation);
  pqLogViewerWidget::setLog(QString::fromStdString(logInformation->GetLogs()));
}

//----------------------------------------------------------------------------
const vtkSmartPointer<vtkSMProxy>& pqSingleLogViewerWidget::getLogRecorderProxy() const
{
  return this->LogRecorderProxy;
}

//----------------------------------------------------------------------------
int pqSingleLogViewerWidget::getRank() const
{
  return this->Rank;
}

//----------------------------------------------------------------------------
void pqSingleLogViewerWidget::closeEvent(QCloseEvent* event)
{
  vtkSMPropertyHelper(this->LogRecorderProxy, "RankDisabled").Set(this->Rank);
  this->LogRecorderProxy->UpdateProperty("RankDisabled", 1);
  event->accept();
}
