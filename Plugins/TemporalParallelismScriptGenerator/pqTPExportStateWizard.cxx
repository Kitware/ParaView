/*=========================================================================

   Program: ParaView
   Module:    pqTPExportStateWizard.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqTPExportStateWizard.h"

#include <QLabel>
#include <QList>
#include <pqImageOutputInfo.h>

//-----------------------------------------------------------------------------
pqTPExportStateWizard::pqTPExportStateWizard(
  QWidget *parentObject, Qt::WindowFlags parentFlags)
  : Superclass(parentObject, parentFlags)
{
}

//-----------------------------------------------------------------------------
pqTPExportStateWizard::~pqTPExportStateWizard()
{
}

//-----------------------------------------------------------------------------
void pqTPExportStateWizard::customize()
{
  // for spatio-temporal scripts we don't care about frequency or fitting
  // the image to screen
  QList<pqImageOutputInfo*> infos = this->getImageOutputInfos();
  for(QList<pqImageOutputInfo*>::iterator it=infos.begin();
      it!=infos.end();it++)
    {
    (*it)->hideFrequencyInput();
    (*it)->hideFitToScreen();
    }

  this->Internals->wizardPage1->setTitle("Export Spatio-Temporal Parallel Script");
  this->Internals->label->setText("This wizard will guide you through the steps required to export the current visualization state as a Python script that can be run with spatio-temporal parallelism with ParaView.  Make sure to add appropriate writers for the desired pipelines to be used in the Writers menu.");
  QStringList labels;
  labels << "Pipeline Name" << "File Location";
  this->Internals->nameWidget->setHorizontalHeaderLabels(labels);
  this->Internals->liveViz->hide();
  this->Internals->rescaleDataRange->hide();
}

//-----------------------------------------------------------------------------
bool pqTPExportStateWizard::getCommandString(QString& command)
{
  command.clear();
  return true;
}
