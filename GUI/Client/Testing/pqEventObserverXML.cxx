/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqEventObserverXML.h"

pqEventObserverXML::pqEventObserverXML(ostream& stream) :
  Stream(stream)
{
  this->Stream << "<?xml version=\"1.0\" ?>\n";
  this->Stream << "<pqevents>\n";
}

pqEventObserverXML::~pqEventObserverXML()
{
  this->Stream << "</pqevents>\n";
}

void pqEventObserverXML::onRecordEvent(const QString& Widget, const QString& Command, const QString& Arguments)
{
  this->Stream
    << "  <pqevent "
    << "object=\"" << Widget.toAscii().data() << "\" "
    << "command=\"" << Command.toAscii().data() << "\" "
    << "arguments=\"" << Arguments.toAscii().data() << "\" "
    << "/>\n";
}

