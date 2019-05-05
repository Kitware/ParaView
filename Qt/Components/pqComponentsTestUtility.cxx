/*=========================================================================

   Program: ParaView
   Module:    pqComponentsTestUtility.cxx

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
#include "pqComponentsTestUtility.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqItemViewSearchWidgetEventPlayer.h"
#include "pqPluginTreeWidgetEventPlayer.h"
#include "pqPluginTreeWidgetEventTranslator.h"
#include "pqServerManagerModel.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqView.h"
#include "vtkImageData.h"

#include <QApplication>
#include <QDebug>
#include <QWidget>

pqComponentsTestUtility::pqComponentsTestUtility(QObject* parentObj)
  : Superclass(parentObj)
{
  this->eventTranslator()->addWidgetEventTranslator(new pqPluginTreeWidgetEventTranslator(this));

  this->eventPlayer()->addWidgetEventPlayer(new pqPluginTreeWidgetEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(new pqItemViewSearchWidgetEventPlayer(this));
}

//-----------------------------------------------------------------------------
bool pqComponentsTestUtility::CompareView(
  const QString& referenceImage, double threshold, const QString& tempDirectory)
{
  pqView* curView = pqActiveObjects::instance().activeView();
  if (curView == nullptr)
  {
    // let's find the first view.
    auto views = pqApplicationCore::instance()->getServerManagerModel()->findItems<pqView*>();
    if (views.size() > 0)
    {
      curView = views[0];
    }
  }

  if (!curView)
  {
    qCritical() << "ERROR: Could not locate the active view.";
    return false;
  }

  // All tests need a 300x300 render window size.
  return pqCoreTestUtility::CompareView(
    curView, referenceImage, threshold, tempDirectory, QSize(300, 300));
}
