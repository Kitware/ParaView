/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqVariableSelectorWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#ifndef QVARIABLESELECTORWIDGET_H
#define QVARIABLESELECTORWIDGET_H

#include "QtWidgetsExport.h"

#include "qwidget.h"
class QComboBox;
class QHBoxLayout;
#include <QString>

class QTWIDGETS_EXPORT pqVariableSelectorWidget : public QWidget
{

  Q_OBJECT

public:
  pqVariableSelectorWidget( QWidget *parent=0 );
  virtual ~pqVariableSelectorWidget();
  
  enum VariableType {
    TYPE_CELL=0,
    TYPE_NODE
  };

  VariableType getCurVarTypeID() const;
  QString getCurVarType() const;
  QString getCurVarName() const;
  QString getCurVarName( VariableType type ) const;
  void getVarTypes( QStringList &names );
  void getVarNames( VariableType type, QStringList &names );

  void clearVarNames( VariableType type );
  void setVarNames( VariableType type, const QStringList &names );
  void addVarNames( VariableType type, const QStringList &names );
  void removeVarNames( VariableType type, const QStringList &names );
  bool addVarName( VariableType type, const QString &name );
  bool removeVarName( VariableType type, const QString &name );

  QComboBox *getVarTypeCombo();
  QComboBox *getVarNameCombo( VariableType type );

  void clearVarTypes();
  void clearVarNames();
 
  /* 
  void printVarTypes();
  void printVarNames();
  void printVarNames( VariableType type );

  static void TestPlan();
  */


public slots:
  virtual void setCurVarType( VariableType type );
  virtual void setCurVarName( const QString &name );
  virtual void update();
  virtual void typeActivated( const QString &type );
  virtual void nameActivated( const QString &name ); 

signals:
  void varTypeChanged( const QString & );
  void varNameChanged( const QString & );

protected: 
  int findVarType( const QString &type );
  int findVarName( VariableType type, const QString &name );

  QHBoxLayout   *Layout;
  QComboBox     *Type;
  QComboBox     *Name[2];
  QString       PrevName[2];
  QString       PrevType;
};

#endif


