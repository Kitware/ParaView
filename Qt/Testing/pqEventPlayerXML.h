/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqEventPlayerXML_h
#define _pqEventPlayerXML_h

#include <QString>

class pqEventPlayer;

class pqEventPlayerXML
{
public:
  /// Loads an XML test case from a file, and plays it with the given player, returning true iff every command was successfully executed
  bool playXML(pqEventPlayer& Player, const QString& Path);
};

#endif // !_pqEventPlayerXML_h

