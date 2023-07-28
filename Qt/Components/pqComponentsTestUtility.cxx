// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
    if (!views.empty())
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
