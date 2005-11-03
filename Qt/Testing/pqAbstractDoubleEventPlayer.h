/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqAbstractDoubleEventPlayer_h
#define _pqAbstractDoubleEventPlayer_h

#include "pqWidgetEventPlayer.h"

/// Translates high-level ParaQ events into low-level Qt slider events
class pqAbstractDoubleEventPlayer :
  public pqWidgetEventPlayer
{
public:
  pqAbstractDoubleEventPlayer();

  bool playEvent(QObject* Object, const QString& Command, const QString& Arguments);

private:
  pqAbstractDoubleEventPlayer(const pqAbstractDoubleEventPlayer&);
  pqAbstractDoubleEventPlayer& operator=(const pqAbstractDoubleEventPlayer&);
};

#endif // !_pqAbstractDoubleEventPlayer_h

