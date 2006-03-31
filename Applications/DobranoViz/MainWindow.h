/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _MainWindow_h
#define _MainWindow_h

#include <pqMainWindow.h>

class pqLineChartWidget;
class LineChartAdapter;
class QComboBox;
class QPoint;

/// Provides the main window for the Dobran-O-Viz application
class MainWindow :
  public pqMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  ~MainWindow();

private slots:
  void onHelpAbout();
  
  void onLoadSetup();
  void onSavePDF();
  void onSavePNG();

  void onLoadExperimentalData();
  void onLoadExperimentalUncertainty();
  void onLoadSimulationUncertainty();
  void onLoadExperimentSimulationMap();

  void onExperimentalDataChanged(const QStringList&);
  void onVisibleDataChanged(const QString&);
  void onLineChartContextMenu(const QPoint&);

private:
  pqLineChartWidget* LineChartWidget;
  LineChartAdapter* LineChart;
  QComboBox* ChooseDataCombo;
};

#endif // !_MainWindow_h

