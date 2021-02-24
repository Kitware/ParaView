/*=========================================================================

   Program: ParaView
   Module:    pqXMLEventObserver.cxx

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
                      << "object=\"" << textToXML(widget).toLocal8Bit().data() << "\" "
                      << "baseline=\"" << textToXML(arguments).toLocal8Bit().data() << "\" "
                      << "threshold=\"5\" "
                      << "/>\n";
      }
      else
      {
        *this->Stream << "  <pqcheck "
                      << "object=\"" << textToXML(widget).toLocal8Bit().data() << "\" "
                      << "property=\"" << textToXML(command).toLocal8Bit().data() << "\" "
                      << "arguments=\"" << textToXML(arguments).toLocal8Bit().data() << "\" "
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
                    << "object=\"" << textToXML(widget).toLocal8Bit().data() << "\" "
                    << "command=\"" << textToXML(command).toLocal8Bit().data() << "\" "
                    << "arguments=\"" << textToXML(arguments).toLocal8Bit().data() << "\" "
                    << "/>\n";
    }
  }
  Q_EMIT eventRecorded(widget, command, arguments, eventType);
}
