// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqDelimitedTextParser_h
#define pqDelimitedTextParser_h

#include "pqWidgetsModule.h"
#include <QObject>

class QIODevice;

/**
  Parses a delimited text file (e.g. a CSV or tab-delimited file), and emits signals that represent
  data series from the file.

  To use it, create an instance of pqDelimitedTextParser, passing the delimiter character in the
  constructor.
  Then, connect the startParsing(), parseSeries(), and finishParsing() signals to slots.  Call
  parse() with the
  filename of the file to be parsed, and the parseSeries() signal will be emitted for each series of
  values
  contained within the file.
*/

class PQWIDGETS_EXPORT pqDelimitedTextParser : public QObject
{
  Q_OBJECT

public:
  enum SeriesT
  {
    /**
     * Data series are organized in columns
     */
    COLUMN_SERIES
  };

  /**
   * Initializes the parser with the delimiter that will be used to separate fields on the same line
   * within parsed files.
   */
  pqDelimitedTextParser(SeriesT series, char delimiter);

  /**
   * Call this to parse a filesystem file.
   */
  void parse(const QString& path);

Q_SIGNALS:
  /**
   * Signal emitted when parsing begins.
   */
  void startParsing();
  /**
   * Signal that will be emitted once for each data series contained in the parsed file.
   */
  void parseSeries(const QStringList&);
  /**
   * Signal emitted when parsing ends.
   */
  void finishParsing();

private:
  const SeriesT Series;
  const char Delimiter;

  void parseColumns(QIODevice& stream);
};

#endif
