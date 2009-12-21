// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqPlotVariablesDialog.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef __pqPlotVariablesDialog_h
#define __pqPlotVariablesDialog_h

#include <QDialog>
#include <QLabel>
#include <QStringList>

class QListWidgetItem;
class QListWidget;

class pqPlotter;
class pqServer;
class vtkSMStringVectorProperty;

class HoverLabel : public QLabel
{
  Q_OBJECT;

public:
  HoverLabel(QWidget *parent = 0);

  virtual void mouseMoveEvent ( QMouseEvent * theEvent );

  void setPlotter(pqPlotter * thePlotter);

  pqPlotter * plotter;
};

/// This dialog box provides an easy way to set up the readers in the pipeline
/// and to ready them for the rest of the tools.
class pqPlotVariablesDialog : public QDialog
{
  Q_OBJECT;
public:
  class pqUI;

  pqPlotVariablesDialog(QWidget *p, Qt::WindowFlags f = 0);
  ~pqPlotVariablesDialog();

  pqPlotVariablesDialog::pqUI * getUI() { return ui; }

  static void setFloatingPointPrecision(int precision);
  static int getFloatingPointPrecision();

  virtual void setupVariablesList(QStringList varStrings);
  virtual void refreshVariablesList(QStringList varStrings);
  virtual void setHeading(QString heading);
  virtual void setTimeRange(double min, double max);
  virtual void addVariable(QString varName);
  virtual void allocSetRange(QString varName, int numComp, int numElems, double ** ranges);
  virtual void addVariableRange(QString varName, int compBegin, int compEnd);
  virtual void initRangeUI();
  virtual bool addRangesToUI(const QList<QListWidgetItem *> & selecteditems);
  virtual bool areVariablesSelected();
  virtual QList<QListWidgetItem *> getSelectedItems();
  virtual QStringList getSelectedItemsStringList();
  virtual QListWidget * getVariableList();
  virtual void setPlotType(int);
  virtual int getPlotType();
  virtual void activateSelectionByNumberFrame();
  virtual void setNumberItemsLabel(QString value);
  virtual bool getUseParaViewGUIToSelectNodesCheckBoxState();
  virtual void setEnableNumberItems(bool flag);
  virtual void setupActivationForOKButton(bool flag);
  virtual QList<int> determineSelectedItemsList(bool & errFlag);
  virtual QString getNumberItemsLineEdit();
  virtual QStringList getVarsWithComponentSuffixes(vtkSMStringVectorProperty *);
  virtual QString stripComponentSuffix(QString variableAsString);

  virtual void setPlotter(pqPlotter * thePlotter);
  virtual pqPlotter * getPlotter();

public slots:
  void listItemClicked(QListWidgetItem *);

  void slotOk(void);
  void slotCancel(void);

  void slotUseParaViewGUIToSelectNodesCheckBox(bool checked);
  void slotTextChanged(const QString &);

signals:
  void variableSelected();
  void okDismissed();
  void cancelDismissed();
  void useParaViewGUIToSelectNodesCheck();

protected:
  pqServer *Server;

private:
  pqPlotVariablesDialog(const pqPlotVariablesDialog &); // Not implemented
  void operator=(const pqPlotVariablesDialog &);        // Not implemented

  pqUI *ui;

  class pqInternal;
  pqInternal * Internal;
};

#endif //__pqPlotVariablesDialog_h
