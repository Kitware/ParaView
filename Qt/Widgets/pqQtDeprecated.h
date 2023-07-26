// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#ifndef pqQtDeprecated_h
#define pqQtDeprecated_h

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define PV_QT_SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#else
#define PV_QT_SKIP_EMPTY_PARTS QString::SkipEmptyParts
#endif

#endif // pqQtDeprecated_h
