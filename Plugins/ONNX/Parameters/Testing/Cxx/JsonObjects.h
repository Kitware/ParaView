// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef JsonObjects_h
#define JsonObjects_h

#include "pqONNXJsonVerify.h"

#include <QJsonArray>
#include <QJsonObject>

namespace Json
{
namespace Version
{
static const QJsonObject current{ { "Major", pqONNXJsonVerify::Major },
  { "Minor", pqONNXJsonVerify::Minor }, { "Patch", pqONNXJsonVerify::Patch } };
static const QJsonObject previous_major{ { "Major", pqONNXJsonVerify::Major - 1 }, { "Minor", 9 },
  { "Patch", 2 } };
static const QJsonObject previous_minor{ { "Major", pqONNXJsonVerify::Major },
  { "Minor", pqONNXJsonVerify::Minor - 1 }, { "Patch", 2 } };
static const QJsonObject previous_patch{ { "Major", pqONNXJsonVerify::Major },
  { "Minor", pqONNXJsonVerify::Minor }, { "Patch", pqONNXJsonVerify::Patch - 1 } };
static const QJsonObject next_patch{ { "Major", pqONNXJsonVerify::Major },
  { "Minor", pqONNXJsonVerify::Minor }, { "Patch", pqONNXJsonVerify::Patch + 1 } };
static const QJsonObject next_minor{ { "Major", pqONNXJsonVerify::Major },
  { "Minor", pqONNXJsonVerify::Minor + 1 }, { "Patch", 0 } };
static const QJsonObject next_major{ { "Major", pqONNXJsonVerify::Major + 1 }, { "Minor", 0 },
  { "Patch", 0 } };
static const QJsonObject greater{ { "Major", pqONNXJsonVerify::Major + 2 },
  { "Minor", pqONNXJsonVerify::Minor - 1 }, { "Patch", pqONNXJsonVerify::Patch - 1 } };

static const QString string_version{ "1.2.3" };
}

namespace Parameters
{
static const QJsonObject correct{ { "Name", "Amperage" }, { "Min", 0 }, { "Max", 2 },
  { "Default", 1 } };
static const QJsonObject correct2{ { "Name", "Resistance" }, { "Min", -10 }, { "Max", 10 },
  { "Default", 3 } };
static const QJsonObject wrong_type{ { "Name", "Amperage" }, { "Min", 0 }, { "Max", "10" },
  { "Default", 1 } };
static const QJsonObject unnamed{ { "Min", 0 }, { "Max", 10 }, { "Default", 1 } };
static const QJsonObject missing_attr{ { "Name", "Amperage" }, { "Max", 10 }, { "Default", 1 } };
}

namespace Time
{
static const QJsonArray _values{ 0, 1, 2, 2.5 };
static const QJsonObject list{
  { "Name", "Time" },
  { "IsTime", true },
  { "TimeValues", _values },
};

static const QJsonObject range{ { "Name", "Time" }, { "IsTime", true }, { "Min", 1.1 },
  { "Max", 9.9 }, { "NumSteps", 9 } };

static const QJsonObject unbound_range{ { "Name", "Time" }, { "IsTime", true }, { "Min", 1.1 },
  { "NumSteps", 9 } };

static const QJsonObject mixed{ { "Name", "Time" }, { "IsTime", true }, { "TimeValues", _values },
  { "NumSteps", 4 } };

static const QJsonObject not_time{ { "Name", "Time" }, { "IsTime", false }, { "Min", 1.1 },
  { "Max", 9.9 }, { "NumSteps", 9 } };

static const QJsonArray _misc_types{ 0, "1", false, 2.5 };
static const QJsonObject non_numeric{
  { "Name", "Time" },
  { "IsTime", true },
  { "TimeValues", _misc_types },
};
}

namespace Output
{
static const QJsonObject correct{ { "OnCellData", true }, { "Dimension", 3 } };
static const QJsonObject bad_dim{ { "OnCellData", true }, { "Dimension", -1.2 } };
static const QJsonObject missing_attr{ { "Dimension", 3 } };
}

static const QJsonObject unparsed{ { "NoImpact", true } };

}

#endif
