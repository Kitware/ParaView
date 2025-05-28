// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTestUtilities.h"

#include "JsonObjects.h"
#include "pqONNXTimeParameter.h"

#include "vtkLogger.h"

#include <QJsonArray>
#include <QJsonObject>

namespace
{
bool TestTime()
{
  vtkLogScopeF(INFO, "Test Time parameter.");

  pqONNXTimeParameter timeRangeParam(Json::Time::range);
  if (timeRangeParam.getMin() != 1.1)
  {
    qWarning() << "Wrong min detected";
    return false;
  }
  if (timeRangeParam.getMax() != 9.9)
  {
    qWarning() << "Wrong max detected";
    return false;
  }
  if (timeRangeParam.getTimes().size() != 9)
  {
    qWarning() << "Wrong number of time values";
    return false;
  }

  pqONNXTimeParameter timeListParam(Json::Time::list);
  if (timeListParam.getMin() != 0)
  {
    qWarning() << "Wrong min detected";
    return false;
  }
  if (timeListParam.getMax() != 2.5)
  {
    qWarning() << "Wrong max detected";
    return false;
  }
  if (timeListParam.getTimes().size() != 4)
  {
    qWarning() << "Wrong number of time values";
    return false;
  }

  pqONNXTimeParameter mixedParam(Json::Time::mixed);
  if (timeListParam.getMin() != 0)
  {
    qWarning() << "Wrong min detected";
    return false;
  }
  if (timeListParam.getMax() != 2.5)
  {
    qWarning() << "Wrong max detected";
    return false;
  }
  if (timeListParam.getTimes().size() != 4)
  {
    qWarning() << "Wrong number of time values";
    return false;
  }

  return true;
}
}

//-----------------------------------------------------------------------------
extern int TestJsonParameters(int, char*[])
{
  if (!::TestTime())
  {
    vtkLog(ERROR, "Time Parameter test failed.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
