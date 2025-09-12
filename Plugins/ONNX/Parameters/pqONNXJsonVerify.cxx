// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqONNXJsonVerify.h"

#include <QJsonArray>
#include <QJsonObject>

/**
 * All "check" method implements a check for a given JSON element.
 * A check method returns false if an element is ill-formed (wrong type or missing sub element), and
 * return true if no error found.
 */
namespace pqONNXJsonVerify
{
namespace NumericAttribute
{
static bool check(const QJsonObject& value, const QString& name)
{
  if (!value.contains(name))
  {
    qWarning() << "Missing \"" << name << "\" value for object " << value;
    return false;
  }
  else if (!value[name].isDouble())
  {
    qWarning() << "\"" << name << "\" should be a numeric value" << value;
    return false;
  }

  return true;
}
};

namespace Version
{
static bool check(const QJsonObject& version)
{
  if (version.isEmpty())
  {
    return false;
  }
  for (auto numericAttr : { "Major", "Minor", "Patch" })
  {
    if (!NumericAttribute::check(version, numericAttr))
    {
      qWarning() << "\"Version\" object is ill-formed";
      return false;
    }
  }

  return isSupportedVersion(version);
}
};

//-----------------------------------------------------------------------------
namespace Input
{
//-----------------------------------------------------------------------------
namespace Value
{
static bool check(const QJsonObject& value)
{
  if (!value.contains("Name"))
  {
    qWarning() << "Missing \"Name\" value for object " << value;
    return false;
  }

  for (auto numericAttr : { "Min", "Max", "Default" })
  {
    if (!NumericAttribute::check(value, numericAttr))
    {
      return false;
    }
  }

  return true;
}
};

//-----------------------------------------------------------------------------
namespace Time
{
static bool check(const QJsonObject& time)
{
  if (!time.contains("Name"))
  {
    qWarning() << "Missing \"Name\" value for object " << time;
    return false;
  }

  if (time.contains("TimeValues"))
  {
    if (!time["TimeValues"].isArray())
    {
      return false;
    }
    auto timeArray = time["TimeValues"].toArray();
    if (timeArray.isEmpty())
    {
      qWarning() << "TimeValues list should not be empty";
      return false;
    }
    for (const auto& element : timeArray)
    {
      if (!element.isDouble())
      {
        qWarning() << "TimeValues list should contains only numeric values, but has " << element;
        return false;
      }
    }
  }
  else if (time.contains("NumSteps"))
  {
    for (auto numericAttr : { "Min", "Max", "NumSteps" })
    {
      if (!NumericAttribute::check(time, numericAttr))
      {
        return false;
      }
    }
  }
  else
  {
    return false;
  }

  return true;
}
};

//-----------------------------------------------------------------------------
static bool check(const QJsonArray& input)
{
  if (input.empty())
  {
    qWarning() << "empty Input array.";
    return false;
  }

  bool hasTime = false;
  for (const auto& element : input)
  {
    if (element.isObject())
    {
      QJsonObject obj = element.toObject();
      if (isTime(obj))
      {
        if (!Time::check(obj))
        {
          qWarning() << "Invalid Time parameter.";
          return false;
        }
        if (hasTime)
        {
          qWarning() << "Input supports only on time parameter, but more were found.";
          return false;
        }
        hasTime = true;
      }
      else
      {
        if (!Value::check(obj))
        {
          qWarning() << "Invalid input parameter.";
          return false;
        }
      }
    }
    else
    {
      qWarning() << "Input should be a valid Json Object";
      return false;
    }
  }

  return true;
};
};

//-----------------------------------------------------------------------------
namespace Output
{
static bool check(const QJsonObject& output)
{
  if (output.empty())
  {
    return false;
  }

  if (output.contains("OnCellData"))
  {
    if (!output["OnCellData"].isBool())
    {
      qWarning() << "\"OnCellData\" should be a boolean.";
      return false;
    }
  }
  else
  {
    qWarning() << "\"Output\" is expected to contain a \"OnCellData\" object";
    return false;
  }

  if (output.contains("Dimension"))
  {
    if (!output["Dimension"].isDouble())
    {
      qWarning() << "\"Dimension\" should be a numeric value";
      return false;
    }

    if (output["Dimension"].toInt(0) <= 0)
    {
      qWarning() << "\"Dimension\" should be a strict positive interger";
      return false;
    }
  }
  else
  {
    qWarning() << "\"Output is expected to contain a \"Dimension\" object";
    return false;
  }

  return true;
}
}
};

//-----------------------------------------------------------------------------
bool pqONNXJsonVerify::check(const QJsonObject& root)
{
  if (root.contains("Version") && root["Version"].isObject())
  {
    if (!Version::check(root["Version"].toObject()))
    {
      qWarning() << "Invalid \"Version\" object";
      return false;
    }
  }
  else
  {
    qWarning() << "ONNX Json file should contain a \"Version\" object.";
    return false;
  }

  if (root.contains("Input") && root["Input"].isArray())
  {
    if (!Input::check(root["Input"].toArray()))
    {
      qWarning() << "Invalid \"Input\" element";
      return false;
    }
  }
  else
  {
    qWarning() << "ONNX Json file should contain an \"Input\" array.";
    return false;
  }

  if (root.contains("Output") && root["Output"].isObject())
  {
    if (!Output::check(root["Output"].toObject()))
    {
      qWarning() << "Invalid \"Output\" element.";
      return false;
    }
  }
  else
  {
    qWarning() << "ONNX Json file should contain an \"Output\" object.";
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool pqONNXJsonVerify::isTime(const QJsonObject& parameter)
{
  return (parameter.contains("IsTime") && parameter["IsTime"].toBool());
}

//-----------------------------------------------------------------------------
bool pqONNXJsonVerify::isSupportedVersion(const QJsonObject& version)
{
  QString fileVersion = QString{ "%1.%2.%3" }
                          .arg(version["Major"].toInt())
                          .arg(version["Minor"].toInt())
                          .arg(version["Patch"].toInt());
  QString pluginSupport = QString{ "%1.%2.%3" }
                            .arg(pqONNXJsonVerify::Major)
                            .arg(pqONNXJsonVerify::Minor)
                            .arg(pqONNXJsonVerify::Patch);

  if (version["Major"].toInt(0) > pqONNXJsonVerify::Major)
  {
    qWarning() << "ONNX Json was generated for the version " << fileVersion
               << ") but your plugin supports only version " << pluginSupport
               << ". Please update your plugin.";
    return false;
  }
  if (version["Major"].toInt(0) == pqONNXJsonVerify::Major &&
    version["Minor"].toInt(0) > pqONNXJsonVerify::Minor)
  {
    qWarning() << "ONNX Json was generated for the version " << fileVersion
               << "). As your plugin support version " << pluginSupport
               << ", some features may be ignored.";
  }

  return true;
}
