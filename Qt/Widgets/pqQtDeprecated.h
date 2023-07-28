// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqQtDeprecated_h
#define pqQtDeprecated_h

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define PV_QT_SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#else
#define PV_QT_SKIP_EMPTY_PARTS QString::SkipEmptyParts
#endif

#endif // pqQtDeprecated_h
