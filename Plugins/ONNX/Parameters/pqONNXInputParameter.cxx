// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqONNXInputParameter.h"

#include <QJsonObject>

pqONNXInputParameter::pqONNXInputParameter(const QJsonObject& json)
{
  this->setName(json["Name"].toString());
  this->setMin(json["Min"].toDouble());
  this->setMax(json["Max"].toDouble());
  this->setDefault(json["Default"].toDouble());
}
