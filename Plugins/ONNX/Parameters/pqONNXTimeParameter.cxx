// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqONNXTimeParameter.h"

#include "pqONNXJsonVerify.h"

#include <QJsonArray>
#include <QJsonObject>

pqONNXTimeParameter::pqONNXTimeParameter(const QJsonObject& json)
{
  this->Name = json["Name"].toString();

  if (!pqONNXJsonVerify::isTime(json))
  {
    qWarning() << "Json object does not represent a time parameter.";
    return;
  }

  if (json.contains("NumSteps"))
  {
    this->Min = json["Min"].toDouble();
    this->Max = json["Max"].toDouble();
    int numSteps = json.value("NumSteps").toInt(0);
    double deltaTime = (this->Max - this->Min) / numSteps;
    // Manually handle maximum to avoid rounding issues
    for (int idx = 0; idx < numSteps - 1; idx++)
    {
      this->Times.append(this->Min + idx * deltaTime);
    }
    this->Times.append(this->Max);
  }
  else if (json.contains("TimeValues"))
  {
    QJsonArray times = json["TimeValues"].toArray();

    for (const auto& time : times)
    {
      this->Times.append(time.toDouble());
    }

    assert(!this->Times.isEmpty());

    this->Min = this->Times.first();
    this->Max = this->Times.last();
  }
}
