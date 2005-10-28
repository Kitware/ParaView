/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqMenuEventTranslatorAdaptor_h
#define _pqMenuEventTranslatorAdaptor_h

#include <QObject>

class QAction;

class pqMenuEventTranslatorAdaptor :
  public QObject
{
  Q_OBJECT
  
public:
  pqMenuEventTranslatorAdaptor(QAction*);

signals:
  void recordEvent(QObject*, const QString&, const QString&);

private:
  pqMenuEventTranslatorAdaptor(const pqMenuEventTranslatorAdaptor&);
  pqMenuEventTranslatorAdaptor& operator=(const pqMenuEventTranslatorAdaptor&);
  
  QAction* const action;
  
private slots:
  void onTriggered(bool);
};

#endif // !_pqMenuEventTranslatorAdaptor_h

