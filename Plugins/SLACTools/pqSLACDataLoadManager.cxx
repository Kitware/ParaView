// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSLACDataLoadManager.cxx

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

#include "pqSLACDataLoadManager.h"

#include <QPushButton>
#include <QtDebug>

#include "ui_pqSLACDataLoadManager.h"
class pqSLACDataLoadManager::pqUI : public Ui::pqSLACDataLoadManager {};

//=============================================================================
pqSLACDataLoadManager::pqSLACDataLoadManager(QWidget *p,
                                             Qt::WindowFlags f/*=0*/)
  : QDialog(p, f)
{
  this->ui = new pqSLACDataLoadManager::pqUI;
  this->ui->setupUi(this);

  this->ui->meshFile->setForceSingleFile(true);
  this->ui->modeFile->setForceSingleFile(false);
  this->ui->particlesFile->setForceSingleFile(false);

  this->ui->meshFile->setExtension("SLAC Mesh Files (*.ncdf *.nc)");
  this->ui->modeFile->setExtension("SLAC Mode Files (*.mod *.m?)");
  this->ui->particlesFile->setExtension("SLAC Particle Files (*.ncdf *.netcdf)");

  QObject::connect(
              this->ui->meshFile, SIGNAL(filenamesChanged(const QStringList &)),
              this, SLOT(checkInputValid()));

  this->checkInputValid();
}

pqSLACDataLoadManager::~pqSLACDataLoadManager()
{
  delete this->ui;
}

//-----------------------------------------------------------------------------
void pqSLACDataLoadManager::checkInputValid()
{
  bool valid = true;

  qWarning() << this->ui->meshFile->filenames().size() << endl;
  if (this->ui->meshFile->filenames().isEmpty()) valid = false;

  this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}
