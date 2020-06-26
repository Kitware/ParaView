// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSLACDataLoadManager.h

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

#ifndef pqSLACDataLoadManager_h
#define pqSLACDataLoadManager_h

#include <QDialog>

class pqServer;

/// This dialog box provides an easy way to set up the readers in the pipeline
/// and to ready them for the rest of the SLAC tools.
class pqSLACDataLoadManager : public QDialog
{
  Q_OBJECT;

public:
  pqSLACDataLoadManager(QWidget* p, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqSLACDataLoadManager();

public Q_SLOTS:
  virtual void checkInputValid();
  virtual void setupPipeline();

Q_SIGNALS:
  void createdPipeline();

protected:
  pqServer* Server;

private:
  Q_DISABLE_COPY(pqSLACDataLoadManager)

  class pqUI;
  pqUI* ui;
};

#endif // pqSLACDataLoadManager_h
