/*=========================================================================

   Program:   ParaQ
   Module:    pqEventPlayerXML.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/// Disables Qt "effects" that might-or-might-not interfere with testing
#include <QApplication>
class pqDisableEffects
{
public:
  pqDisableEffects() :
    General(QApplication::isEffectEnabled(Qt::UI_General)),
    AnimateMenu(QApplication::isEffectEnabled(Qt::UI_AnimateMenu)),
    FadeMenu(QApplication::isEffectEnabled(Qt::UI_FadeMenu)),
    AnimateCombo(QApplication::isEffectEnabled(Qt::UI_AnimateCombo)),
    AnimateTooltip(QApplication::isEffectEnabled(Qt::UI_AnimateTooltip)),
    FadeTooltip(QApplication::isEffectEnabled(Qt::UI_FadeTooltip)),
    AnimateToolBox(QApplication::isEffectEnabled(Qt::UI_AnimateToolBox))
  {
    QApplication::setEffectEnabled(Qt::UI_General, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
    QApplication::setEffectEnabled(Qt::UI_FadeMenu, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);
    QApplication::setEffectEnabled(Qt::UI_FadeTooltip, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateToolBox, false);
  }
  
  ~pqDisableEffects()
  {
    QApplication::setEffectEnabled(Qt::UI_General, this->General);
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, this->AnimateMenu);
    QApplication::setEffectEnabled(Qt::UI_FadeMenu, this->FadeMenu);
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, this->AnimateCombo);
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, this->AnimateTooltip);
    QApplication::setEffectEnabled(Qt::UI_FadeTooltip, this->FadeTooltip);
    QApplication::setEffectEnabled(Qt::UI_AnimateToolBox, this->AnimateToolBox);
  }
  
private:
  const bool General;
  const bool AnimateMenu;
  const bool FadeMenu;
  const bool AnimateCombo;
  const bool AnimateTooltip;
  const bool FadeTooltip;
  const bool AnimateToolBox;
};

///////////////////////////////////////////////////////////////////////////////////////////
// pqEventPlayerXML

bool pqEventPlayerXML::playXML(pqEventPlayer& Player, const QString& Path)
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

  pqDisableEffects disable_effects;

  for(QDomNode xml_event = xml_events.firstChild(); !xml_event.isNull(); xml_event = xml_event.nextSibling())
    {
    if(!(xml_event.isElement() && xml_event.nodeName() == "pqevent"))
      continue;
      
    const QString object = xmlToText(xml_event.toElement().attribute("object"));
    const QString command = xmlToText(xml_event.toElement().attribute("command"));
    const QString arguments = xmlToText(xml_event.toElement().attribute("arguments"));
      
    if(!Player.playEvent(object, command, arguments))
      return false;
    }
    
  return true;
}
