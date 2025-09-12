// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqONNXOutputParameters.h"

#include <QJsonObject>

pqONNXOutputParameters::pqONNXOutputParameters(const QJsonObject& json)
{
  this->OnCell = json["OnCellData"].toBool(true);
  this->Dimension = json["Dimension"].toInt(1);
}
