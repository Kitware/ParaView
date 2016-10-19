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

#ifndef pqPlotVariablesDialog_h
#define pqPlotVariablesDialog_h

#include <QDialog>
#include <QItemSelection>
#include <QLabel>
#include <QStringList>

class QListWidgetItem;
class QListWidget;

class pqPlotter;
class pqServer;
class vtkSMStringVectorProperty;

/// This dialog box provides an easy way to set up the readers in the pipeline
/// and to ready them for the rest of the tools.
class pqPlotVariablesDialog : public QDialog
{
  Q_OBJECT;

public:
  class pqUI;

  pqPlotVariablesDialog(QWidget* p, Qt::WindowFlags f = 0);
  ~pqPlotVariablesDialog();

  pqPlotVariablesDialog::pqUI* getUI() { return ui; }

  virtual QSize sizeHint() const;

  static void setFloatingPointPrecision(int precision);
  static int getFloatingPointPrecision();

  virtual void setupVariablesList(QStringList varStrings);
  virtual void setHeading(QString heading);
  virtual void setTimeRange(double min, double max);
  virtual void addVariable(QString varName);
  virtual void allocSetRange(QString varName, int numComp, int numElems, double** ranges);
  virtual bool addRangeToUI(QString itemText);
  virtual bool removeRangeFromUI(QString itemText);
  virtual bool areVariablesSelected();
  virtual QList<QListWidgetItem*> getSelectedItems();
  virtual QStringList getSelectedItemsStringList();
  virtual QListWidget* getVariableList();
  virtual void setPlotType(int);
  virtual int getPlotType();
  virtual void activateSelectionByNumberFrame();
  virtual void setNumberItemsLabel(QString value);
  virtual bool getUseParaViewGUIToSelectNodesCheckBoxState();
  virtual void setEnableNumberItems(bool flag);
  virtual void setupActivationForOKButton(bool flag);
  virtual QList<int> determineSelectedItemsList(bool& errFlag);
  virtual QString getNumberItemsLineEdit();
  virtual QStringList getVarsWithComponentSuffixes(vtkSMStringVectorProperty*);
  virtual QString stripComponentSuffix(QString variableAsString);

  virtual void setPlotter(pqPlotter* thePlotter);
  virtual pqPlotter* getPlotter();

public slots:
  void slotItemSelectionChanged();

  void slotOk(void);
  void slotCancel(void);

  void slotUseParaViewGUIToSelectNodesCheckBox(bool checked);
  void slotTextChanged(const QString&);

signals:
  void variableSelected(QListWidgetItem* item);
  void variableDeselectionByName(QString varName);
  void variableSelectionByName(QString varName);
  void okDismissed();
  void cancelDismissed();
  void useParaViewGUIToSelectNodesCheck();

protected:
  pqServer* Server;

private:
  Q_DISABLE_COPY(pqPlotVariablesDialog)

  pqUI* ui;

  class pqInternal;
  pqInternal* Internal;
};

#endif // pqPlotVariablesDialog_h
