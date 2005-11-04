/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqEventPlayer_h
#define _pqEventPlayer_h

#include <QString>
#include <QVector>

class QObject;
class pqWidgetEventPlayer;

/// Manages translation of high-level ParaQ events to low-level Qt events, for playback of test-cases, demos, tutorials, etc.
class pqEventPlayer
{
public:
  pqEventPlayer(QObject& RootObject);
  ~pqEventPlayer();

  /// Adds the default set of widget players to the current working set.  Players are executed in-order, so you can call addWidgetEventPlayer() before this function to override default players.
  void addDefaultWidgetEventPlayers();
  /// Adds a new player to the current working set of widget players.  pqEventPlayer assumes control of the lifetime of the supplied object.
  void addWidgetEventPlayer(pqWidgetEventPlayer*);

  /// This method is called with each high-level ParaQ event, which will invoke the corresponding low-level Qt functionality in-turn.  Returns true on success, false on failure.
  bool playEvent(const QString& Object, const QString& Command, const QString& Arguments = QString());

private:
  pqEventPlayer(const pqEventPlayer&);
  pqEventPlayer& operator=(const pqEventPlayer&);

  /// Stores the working set of widget players  
  QVector<pqWidgetEventPlayer*> Players;
  /// Stores the root of the Qt object hierarchy
  QObject& RootObject;
};

#endif // !_pqEventPlayer_h

