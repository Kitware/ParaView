/*=========================================================================

   Program: ParaView
   Module:  pqSingleLogViewerWidget.cxx

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
