/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqEventPlayer.h"
#include "pqEventPlayerXML.h"

#include <QFile>
#include <QtXml/QDomDocument>
#include <QtDebug>

bool pqEventPlayerXML::playXML(pqEventPlayer& Player, const QString& Path)
{
  QFile file(Path);
  QDomDocument xml_document;
  xml_document.setContent(&file, false);

  QDomElement xml_events = xml_document.documentElement();
  if(xml_events.nodeName() != "pqevents")
    {
    qCritical() << Path << " is not an XML test case document";
    return false;
    }

  for(QDomNode xml_event = xml_events.firstChild(); !xml_event.isNull(); xml_event = xml_event.nextSibling())
    {
    if(!(xml_event.isElement() && xml_event.nodeName() == "pqevent"))
      continue;
      
    const QString object = xml_event.toElement().attribute("object");
    const QString command = xml_event.toElement().attribute("command");
    const QString arguments = xml_event.toElement().attribute("arguments");
      
    if(!Player.playEvent(object, command, arguments))
      return false;
    }
    
  return true;
}

