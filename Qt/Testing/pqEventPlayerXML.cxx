/*=========================================================================

   Program: ParaView
   Module:    pqEventPlayerXML.cxx

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

#include "pqEventPlayer.h"
#include "pqEventPlayerXML.h"

#include <QFile>
#include <QtXml/QDomDocument>
#include <QtDebug>

/// Removes XML entities from a string
static const QString xmlToText(const QString& string)
{
  QString result = string;
  
  result.replace("&lt;", "<");
  result.replace("&gt;", ">");
  result.replace("&apos;", "'");
  result.replace("&quot;", "\"");
  result.replace("&amp;", "&");
  
  return result;
}

///////////////////////////////////////////////////////////////////////////////////////////
// pqEventPlayerXML

bool pqEventPlayerXML::playXML(pqEventPlayer& player, const QString& Path)
{

  QFile file(Path);
  QDomDocument xml_document;
  if(!xml_document.setContent(&file, false))
    {
    qCritical() << "Error parsing " << Path << ": not a valid XML document";
    return false;
    }
  
  QDomElement xml_events = xml_document.documentElement();
  if(xml_events.nodeName() != "pqevents")
    {
    qCritical() << Path << " is not an XML test case document";
    return false;
    }


  this->Player = &player;
  QObject::connect(this->Player, SIGNAL(readyPlayEvent()),
                   this, SLOT(playNextEvent()));

  // get ready for the first one
  mDomNode = xml_events.firstChild();

  bool status = this->Player->exec();
  
  QObject::disconnect(this->Player, SIGNAL(readyPlayEvent()),
                   this, SLOT(playNextEvent()));
  this->Player = NULL;

  return status;
}
  
void pqEventPlayerXML::playNextEvent()
{
  while(!this->mDomNode.isNull() &&
      !(this->mDomNode.isElement() && this->mDomNode.nodeName() == "pqevent"))
    {
    this->mDomNode = this->mDomNode.nextSibling();
    }

  if(this->mDomNode.isNull())
    {
    this->Player->exit(true);
    }
  else
    {
    QString object, command, arguments;
    object = xmlToText(this->mDomNode.toElement().attribute("object"));
    command = xmlToText(this->mDomNode.toElement().attribute("command"));
    arguments = xmlToText(this->mDomNode.toElement().attribute("arguments"));
    this->Player->playEvent(object, command, arguments);
    this->mDomNode = this->mDomNode.nextSibling();
    }
}


