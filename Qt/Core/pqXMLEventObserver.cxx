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

#include <QTextStream>

/// Escapes strings so they can be embedded in an XML document
static const QString textToXML(const QString& string)
{
  QString result = string;
  result.replace("&", "&amp;");  // keep first
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

pqXMLEventObserver::~pqXMLEventObserver()
{
}

void pqXMLEventObserver::setStream(QTextStream* stream)
{
  if(this->Stream)
    {
    *this->Stream << "</pqevents>\n";
    }
  pqEventObserver::setStream(stream);
  if(this->Stream)
    {
    *this->Stream << "<?xml version=\"1.0\" ?>\n";
    *this->Stream << "<pqevents>\n";
    }
}


void pqXMLEventObserver::onRecordEvent(
  const QString& Widget,
  const QString& Command,
  const QString& Arguments)
{
  if(this->Stream)
    {
    *this->Stream
      << "  <pqevent "
      << "object=\"" << textToXML(Widget).toAscii().data() << "\" "
      << "command=\"" << textToXML(Command).toAscii().data() << "\" "
      << "arguments=\"" << textToXML(Arguments).toAscii().data() << "\" "
      << "/>\n";
    }
}

