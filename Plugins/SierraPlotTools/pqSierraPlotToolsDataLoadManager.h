// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSierraPlotToolsDataLoadManager.h

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

#ifndef pqSierraPlotToolsDataLoadManager_h
#define pqSierraPlotToolsDataLoadManager_h

#include <QDialog>

class pqServer;

/// This dialog box provides an easy way to set up the readers in the pipeline
/// and to ready them for the rest of the tools.
class pqSierraPlotToolsDataLoadManager : public QDialog
{
  Q_OBJECT;

public:
  pqSierraPlotToolsDataLoadManager(QWidget* p, Qt::WindowFlags f = 0);
  ~pqSierraPlotToolsDataLoadManager();

public slots:
  virtual void checkInputValid();
  virtual void setupPipeline();

signals:
  void createdPipeline();

protected:
  pqServer* Server;

private:
  Q_DISABLE_COPY(pqSierraPlotToolsDataLoadManager)

  class pqUI;
  pqUI* ui;
};

#endif // pqSierraPlotToolsDataLoadManager_h
