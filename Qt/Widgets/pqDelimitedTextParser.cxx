/*=========================================================================

   Program: ParaView
   Module:    pqDelimitedTextParser.cxx

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
