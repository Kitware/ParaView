/*=========================================================================

   Program: ParaView
   Module:    pqXMLEventSource.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqXMLEventSource.h"

#include <QDomDocument>
#include <QFile>
#include <QtDebug>

///////////////////////////////////////////////////////////////////////////
// pqXMLEventSource::pqImplementation

class pqXMLEventSource::pqImplementation
{
public:
  QDomDocument Document;
  QDomNode CurrentEvent;
};

///////////////////////////////////////////////////////////////////////////////////////////
// pqXMLEventSource

pqXMLEventSource::pqXMLEventSource(QObject* p) :
  pqEventSource(p),
  Implementation(new pqImplementation())
{
}

pqXMLEventSource::~pqXMLEventSource()
{
  delete this->Implementation;
}

void pqXMLEventSource::setContent(const QString& path)
{
  QFile file(path);
  if(!this->Implementation->Document.setContent(&file, false))
    {
    qCritical() << "Error parsing " << path << ": not a valid XML document";
    return;
    }
  
  QDomElement xml_events = this->Implementation->Document.documentElement();
  if(xml_events.nodeName() != "pqevents")
    {
    qCritical() << path<< " is not an XML test case document";
    return;
    }

  this->Implementation->CurrentEvent = xml_events.firstChild();
}

int pqXMLEventSource::getNextEvent(
  QString& object,
  QString& command,
  QString& arguments)
{
  if(this->Implementation->CurrentEvent.isNull())
    return DONE;
    
  if(!this->Implementation->CurrentEvent.isElement())
    return FAILURE;
    
  if(this->Implementation->CurrentEvent.nodeName() != "pqevent")
    return FAILURE;
    
  object = this->Implementation->CurrentEvent.toElement().attribute("object");
  command = this->Implementation->CurrentEvent.toElement().attribute("command");
  arguments = this->Implementation->CurrentEvent.toElement().attribute("arguments");

  this->Implementation->CurrentEvent =
    this->Implementation->CurrentEvent.nextSibling();

  return SUCCESS;
}

