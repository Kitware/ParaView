/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqWidgetEventPlayer_h
#define _pqWidgetEventPlayer_h

class QObject;
class QString;

/// Abstract interface for an object that can playback high-level ParaQ events by translating them into low-level Qt events, for test-cases, demos, tutorials, etc.
class pqWidgetEventPlayer
{
public:
  virtual ~pqWidgetEventPlayer() {}

  virtual bool playEvent(QObject* Object, const QString& Command, const QString& Arguments) = 0;

protected:
  pqWidgetEventPlayer() {}
  pqWidgetEventPlayer(const pqWidgetEventPlayer&) {}
  pqWidgetEventPlayer& operator=(const pqWidgetEventPlayer&) { return *this; }
};

#endif // !_pqWidgetEventPlayer_h

