// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqXMLEventObserver.h"
#include "pqCoreTestUtility.h"
#include "pqEventTypes.h"

#include <QTextStream>

/// Escapes strings so they can be embedded in an XML document
static QString textToXML(const QString& string)
{
  QString result = string;
  result.replace("&", "&amp;"); // keep first
  result.replace("<", "&lt;");
  result.replace(">", "&gt;");
  result.replace("'", "&apos;");
  result.replace("\"", "&quot;");
  result.replace("\n", "&#xA;");
  return result;
}

////////////////////////////////////////////////////////////////////////////////////
// pqXMLEventObserver

pqXMLEventObserver::pqXMLEventObserver(QObject* p)
  : pqEventObserver(p)
{
}

pqXMLEventObserver::~pqXMLEventObserver() = default;

void pqXMLEventObserver::setStream(QTextStream* stream)
{
  if (this->Stream)
  {
    *this->Stream << "</pqevents>\n";
  }
  pqEventObserver::setStream(stream);
  if (this->Stream)
  {
    *this->Stream << "<?xml version=\"1.0\" ?>\n";
    *this->Stream << "<pqevents>\n";
  }
}

void pqXMLEventObserver::onRecordEvent(
  const QString& widget, const QString& command, const QString& arguments, const int& eventType)
{
  // Check Event
  if (eventType == pqEventTypes::CHECK_EVENT)
  {
    if (this->Stream)
    {
      // save a pqcompareview event when command is PQ_COMPAREVIEW_PROPERTY_NAME, is it too sketchy
      // ?
      if (command == pqCoreTestUtility::PQ_COMPAREVIEW_PROPERTY_NAME)
      {
        *this->Stream << "  <pqcompareview "
                      << "object=\"" << textToXML(widget).toUtf8().data() << "\" "
                      << "baseline=\"" << textToXML(arguments).toUtf8().data() << "\" "
                      << "threshold=\"5\" "
                      << "/>\n";
      }
      else
      {
        *this->Stream << "  <pqcheck "
                      << "object=\"" << textToXML(widget).toUtf8().data() << "\" "
                      << "property=\"" << textToXML(command).toUtf8().data() << "\" "
                      << "arguments=\"" << textToXML(arguments).toUtf8().data() << "\" "
                      << "/>\n";
      }
    }
  }
  // Event
  else
  {
    if (this->Stream)
    {
      *this->Stream << "  <pqevent "
                    << "object=\"" << textToXML(widget).toUtf8().data() << "\" "
                    << "command=\"" << textToXML(command).toUtf8().data() << "\" "
                    << "arguments=\"" << textToXML(arguments).toUtf8().data() << "\" "
                    << "/>\n";
    }
  }
  Q_EMIT eventRecorded(widget, command, arguments, eventType);
}
