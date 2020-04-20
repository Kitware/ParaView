/*=========================================================================

   Program: ParaView
   Module:    pqDelimitedTextParser.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _pqDelimitedTextParser_h
#define _pqDelimitedTextParser_h

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
