// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqDelimitedTextParser.h"

#include <QFile>
#include <QStringList>
#include <QVector>

pqDelimitedTextParser::pqDelimitedTextParser(SeriesT series, char delimiter)
  : Series(series)
  , Delimiter(delimiter)
{
}

void pqDelimitedTextParser::parse(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  switch (this->Series)
  {
    case COLUMN_SERIES:
      this->parseColumns(file);
      break;
  }
}

void pqDelimitedTextParser::parseColumns(QIODevice& stream)
{
  QVector<QStringList> series;

  Q_EMIT startParsing();

  for (;;)
  {
    QByteArray line = stream.readLine();

    int column = 0;
    int begin = 0;

    for (int i = 0; i < line.size(); ++i)
    {
      if (line[i] == this->Delimiter || i == line.size() - 1)
      {
        while (series.size() <= column)
          series.push_back(QStringList());

        series[column].push_back(QString(line.mid(begin, i - begin)));

        ++column;
        begin = i + 1;
      }
    }

    if (stream.atEnd())
      break;
  }

  for (int i = 0; i != series.size(); ++i)
    Q_EMIT parseSeries(series[i]);

  Q_EMIT finishParsing();
}
