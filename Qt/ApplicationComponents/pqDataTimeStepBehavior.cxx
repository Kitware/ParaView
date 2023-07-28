// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDataTimeStepBehavior.h"

#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTimeKeeper.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSettings.h"

//-----------------------------------------------------------------------------
pqDataTimeStepBehavior::pqDataTimeStepBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getObjectBuilder(),
    SIGNAL(readerCreated(pqPipelineSource*, const QStringList&)), this,
    SLOT(onReaderCreated(pqPipelineSource*)), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
void pqDataTimeStepBehavior::onReaderCreated(pqPipelineSource* reader)
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  int defaultTimeStep = settings->GetSettingAsInt(
    ".settings.GeneralSettings.DefaultTimeStep", vtkPVGeneralSettings::DEFAULT_TIME_STEP_UNCHANGED);
  if (defaultTimeStep == vtkPVGeneralSettings::DEFAULT_TIME_STEP_UNCHANGED)
  {
    return;
  }

  pqAnimationScene* scene =
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqAnimationScene*>(
      reader->getServer())[0];
  vtkSMProxy* readerProxy = reader->getProxy();
  if (readerProxy->GetProperty("TimestepValues"))
  {
    vtkSMPropertyHelper helper(readerProxy, "TimestepValues");
    unsigned int num_timesteps = helper.GetNumberOfElements();
    if (num_timesteps > 0)
    {
      std::vector<double> timesteps = helper.GetDoubleArray();
      unsigned int newTimeStep = (defaultTimeStep == vtkPVGeneralSettings::DEFAULT_TIME_STEP_FIRST)
        ? 0
        : (num_timesteps - 1);
      scene->setAnimationTime(timesteps[newTimeStep]);
    }
  }
  else if (readerProxy->GetProperty("TimeRange"))
  {
    vtkSMPropertyHelper helper(readerProxy, "TimeRange");
    std::vector<double> timeRange = helper.GetDoubleArray();
    if (!timeRange.empty())
    {
      double newTime = (defaultTimeStep == vtkPVGeneralSettings::DEFAULT_TIME_STEP_FIRST)
        ? timeRange[0]
        : timeRange[1];
      scene->setAnimationTime(newTime);
    }
  }
}
