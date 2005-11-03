/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqMenuEventTranslatorAdaptor.h"

#include <QAction>

pqMenuEventTranslatorAdaptor::pqMenuEventTranslatorAdaptor(QAction* Action) :
  action(Action)
{
  QObject::connect(action, SIGNAL(triggered(bool)), this, SLOT(onTriggered(bool)));
}

void pqMenuEventTranslatorAdaptor::onTriggered(bool)
{
  recordEvent(action, "activate", "");
}
