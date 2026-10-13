// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqWidgetUtilities_h
#define pqWidgetUtilities_h

#include "pqWidgetsModule.h"

#include <QString>
#include <QWidget>

/**
 * Set of utility functions designed to create wider tooltips, since Qt does
 * not allow us to set this directly.
 */
class PQWIDGETS_EXPORT pqWidgetUtilities
{
public:
  /**
   * Convert a rich-text QString into an HTML, formatted with wrapping if necessary
   */
  static QString formatTooltip(const QString& rawText);

  /**
   * Convert a Markdown QString into HTML suitable for a tooltip. A short
   * tooltip stays on one line and a long one is wrapped at a maximum
   * width, the same way formatTooltip() does for rich-text. Returns an
   * empty string if the Markdown is empty.
   */
  static QString formatMarkdownTooltip(const QString& markdown);

  ///@{
  /**
   * Convert proxy documentation from RST to HTML (so that it can be used in Qt)
   */
  static std::string rstToHtml(const char* rstStr);
  static QString rstToHtml(const QString& rstStr);
  ///@}

  static void formatChildTooltips(QWidget* parentObject);
};

#endif // pqWidgetUtilities_h
