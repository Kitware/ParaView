// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTestUtilities.h"

#include "JsonObjects.h"

#include "pqONNXJsonVerify.h"

#include "vtkLogger.h"

#include <QJsonArray>
#include <QJsonObject>

namespace
{
//-----------------------------------------------------------------------------
bool TestSupportedVersion()
{
  vtkLogScopeF(INFO, "Test isSuppertedVersion");
  if (!pqONNXJsonVerify::isSupportedVersion(Json::Version::current))
  {
    return false;
  }

  if (!pqONNXJsonVerify::isSupportedVersion(Json::Version::previous_patch))
  {
    return false;
  }

  if (!pqONNXJsonVerify::isSupportedVersion(Json::Version::previous_minor))
  {
    return false;
  }

  if (!pqONNXJsonVerify::isSupportedVersion(Json::Version::previous_major))
  {
    return false;
  }

  if (!pqONNXJsonVerify::isSupportedVersion(Json::Version::next_patch))
  {
    return false;
  }

  if (!pqONNXJsonVerify::isSupportedVersion(Json::Version::next_minor))
  {
    return false;
  }

  if (pqONNXJsonVerify::isSupportedVersion(Json::Version::next_major))
  {
    return false;
  }

  if (pqONNXJsonVerify::isSupportedVersion(Json::Version::greater))
  {
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool TestIsTime()
{
  vtkLogScopeF(INFO, "Test isTime utility method.");
  if (!pqONNXJsonVerify::isTime(Json::Time::range))
  {
    return false;
  }

  if (!pqONNXJsonVerify::isTime(Json::Time::list))
  {
    return false;
  }

  if (!pqONNXJsonVerify::isTime(Json::Time::unbound_range))
  {
    return false;
  }

  if (!pqONNXJsonVerify::isTime(Json::Time::mixed))
  {
    return false;
  }

  if (pqONNXJsonVerify::isTime(Json::Time::not_time))
  {
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool TestStructure()
{
  vtkLogScopeF(INFO, "TestStructure");

  const QJsonObject extra_entry{ { "Version", Json::Version::current },
    { "Input",
      QJsonArray{ Json::Parameters::correct, Json::Parameters::correct2, Json::Time::list } },
    { "Output", Json::Output::correct }, { "UnParsed", Json::unparsed } };
  if (!pqONNXJsonVerify::check(extra_entry))
  {
    vtkLog(ERROR, "Extra object should not be an error.");
    return false;
  }

  const QJsonObject no_input{ { "Version", Json::Version::current },
    { "Output", Json::Output::correct }, { "UnParsed", Json::unparsed } };
  if (pqONNXJsonVerify::check(no_input))
  {
    vtkLog(ERROR, "Json without an Input array should be invalid");
    return false;
  }

  const QJsonObject bad_type{
    { "Version", Json::Version::current },
    { "Input", Json::Parameters::correct },
    { "Output", Json::Output::correct },
  };
  if (pqONNXJsonVerify::check(bad_type))
  {
    vtkLog(ERROR, "Input should be an array and not an object");
    return false;
  }

  const QJsonObject no_time{
    { "Version", Json::Version::current },
    { "Input", QJsonArray{ Json::Parameters::correct, Json::Parameters::correct2 } },
    { "Output", Json::Output::correct },
  };
  if (!pqONNXJsonVerify::check(no_time))
  {
    vtkLog(ERROR, "Input list should without a time parameter should be valid");
    return false;
  }

  const QJsonObject extra_time{
    { "Version", Json::Version::current },
    { "Input", QJsonArray{ Json::Parameters::correct, Json::Time::list, Json::Time::range } },
    { "Output", Json::Output::correct },
  };
  if (pqONNXJsonVerify::check(extra_time))
  {
    vtkLog(ERROR, "Multiple Time parameters are not allowed");
    return false;
  }

  const QJsonObject no_version{
    { "Input", QJsonArray{ Json::Parameters::correct, Json::Parameters::correct2 } },
    { "Output", Json::Output::correct }
  };
  if (pqONNXJsonVerify::check(no_version))
  {
    vtkLog(ERROR, "Json should have a version object.");
    return false;
  }

  const QJsonObject no_output{
    { "Version", Json::Version::current },
    { "Input", QJsonArray{ Json::Parameters::correct, Json::Parameters::correct2 } },
  };
  if (pqONNXJsonVerify::check(no_output))
  {
    vtkLog(ERROR, "Json parameter without a Name should be invalid");
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool TestInputs()
{
  QJsonObject root{
    { "Version", Json::Version::current },
    { "Input", QJsonArray{ Json::Parameters::correct, Json::Time::range } },
    { "Output", Json::Output::correct },
  };

  QList<QJsonObject> invalid_inputs = { Json::Parameters::unnamed, Json::Parameters::wrong_type,
    Json::Parameters::missing_attr, QJsonObject() };
  for (auto& param : invalid_inputs)
  {
    root["Input"] = QJsonArray{ param, Json::Time::range };
    if (pqONNXJsonVerify::check(root))
    {
      qWarning() << "Invalid Input was not detected: " << root;
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool TestTimes()
{
  QJsonObject root{
    { "Version", Json::Version::current },
    { "Input", QJsonArray{ Json::Parameters::correct, Json::Time::range } },
    { "Output", Json::Output::correct },
  };

  QList<QJsonObject> invalid_times = { Json::Time::unbound_range, Json::Time::non_numeric };
  for (auto& param : invalid_times)
  {
    root["Input"] = QJsonArray{ Json::Parameters::correct, param };
    if (pqONNXJsonVerify::check(root))
    {
      qWarning() << "Invalid Time was not detected: " << root;
      return false;
    }
  }

  root["Input"] = QJsonArray{ Json::Parameters::correct, Json::Time::mixed };
  if (!pqONNXJsonVerify::check(root))
  {
    qWarning() << "TimeValues list should override range and be a valid definition: " << root;
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool TestOutput()
{
  QJsonObject root{
    { "Version", Json::Version::current },
    { "Input", QJsonArray{ Json::Parameters::correct, Json::Time::range } },
    { "Output", Json::Output::correct },
  };

  QList<QJsonObject> invalid_outputs = { Json::Output::bad_dim, Json::Output::missing_attr,
    QJsonObject() };
  for (auto& param : invalid_outputs)
  {
    root["Output"] = param;
    if (pqONNXJsonVerify::check(root))
    {
      qWarning() << "Invalid Output was not detected: " << root;
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool TestVersion()
{
  QJsonObject root{
    { "Version", Json::Version::current },
    { "Input", QJsonArray{ Json::Parameters::correct, Json::Time::range } },
    { "Output", Json::Output::correct },
  };

  QList<QJsonObject> invalid_versions = { Json::Version::next_major, QJsonObject() };
  for (auto& param : invalid_versions)
  {
    root["Version"] = param;
    if (pqONNXJsonVerify::check(root))
    {
      qWarning() << "Invalid version was not detected: " << root;
      return false;
    }
  }

  root["Version"] = Json::Version::string_version;
  if (pqONNXJsonVerify::check(root))
  {
    qWarning() << "Invalid version was not detected: " << root;
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool TestContent()
{
  vtkLogScopeF(INFO, "Test Json content combinations");

  QJsonObject root{
    { "Version", Json::Version::current },
    { "Input", QJsonArray{ Json::Parameters::correct, Json::Time::range } },
    { "Output", Json::Output::correct },
  };
  if (!pqONNXJsonVerify::check(root))
  {
    qWarning() << "Checking valid json returned false";
    return false;
  }

  if (!::TestVersion())
  {
    return false;
  }

  if (!::TestInputs())
  {
    return false;
  }

  if (!::TestTimes())
  {
    return false;
  }

  if (!::TestOutput())
  {
    return false;
  }

  return true;
}
}

//-----------------------------------------------------------------------------
extern int TestJsonVerify(int, char*[])
{
  if (!::TestSupportedVersion())
  {
    vtkLog(ERROR, "Version check failed.");
    return EXIT_FAILURE;
  }

  if (!::TestIsTime())
  {
    vtkLog(ERROR, "Time check failed");
    return EXIT_FAILURE;
  }

  if (!::TestStructure())
  {
    vtkLog(ERROR, "Error when checking Json");
    return EXIT_FAILURE;
  }

  if (!::TestContent())
  {
    vtkLog(ERROR, "Error when checking json content");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
