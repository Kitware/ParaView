// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqWidgetUtilities_h
#define pqWidgetUtilities_h

#include "pqWidgetsModule.h"

#include <QString>

class PQWIDGETS_EXPORT pqWidgetUtilities
{
public:
  /**
   * Convert a rich-text QString into an HTML, formatted with wrapping if necessary
   */
  static QString formatTooltip(const QString& rawText);

  ///@{
  /**
   * Convert proxy documentation from RST to HTML (so that it can be used in Qt)
   */
  static std::string rstToHtml(const char* rstStr);
  static QString rstToHtml(const QString& rstStr);
  ///@}
};

#endif // pqWidgetUtilities_h
