/*=========================================================================

   Program: ParaView
   Module:    pqActivePlotMatrixViewOptions.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pqActivePlotMatrixViewOptions.h"

#include "pqApplicationCore.h"
#include "pqPlotMatrixOptionsEditor.h"
#include "pqOptionsDialog.h"
#include "pqView.h"

#include <QString>
#include <QVariant>
#include <QWidget>

#include "vtkSMProxy.h"

//----------------------------------------------------------------------------
pqActivePlotMatrixViewOptions::pqActivePlotMatrixViewOptions(QObject *parentObject)
  : pqActiveViewOptions(parentObject)
{
  this->Dialog = 0;
}

pqActivePlotMatrixViewOptions::~pqActivePlotMatrixViewOptions()
{
}

void pqActivePlotMatrixViewOptions::showOptions(pqView *view, const QString &page,
    QWidget *widgetParent)
{
  // Create the chart options dialog if necessary.
  if(!this->Dialog)
    {
    this->Dialog = new pqOptionsDialog(widgetParent);
    this->Dialog->setObjectName("ActivePlotMatrixOptions");
    this->Editor = new pqPlotMatrixOptionsEditor();
    this->changeView(view);
    this->Dialog->addOptions(this->Editor);

    this->connect(this->Dialog, SIGNAL(finished(int)),
        this, SLOT(finishDialog(int)));
    this->connect(this->Dialog, SIGNAL(destroyed()),
        this, SLOT(cleanupDialog()));
    this->connect(this->Dialog, SIGNAL(aboutToApplyChanges()),
        this, SLOT(openUndoSet()));
    this->connect(this->Dialog, SIGNAL(appliedChanges()),
        this, SLOT(closeUndoSet()));
    }

  this->changeView(view);
  if(page.isEmpty())
    {
    this->Dialog->setCurrentPage("General");
    }
  else
    {
    this->Dialog->setCurrentPage(page);
    }

  this->Dialog->setResult(0);
  this->Dialog->show();
}

void pqActivePlotMatrixViewOptions::changeView(pqView *view)
{
  if(this->Dialog)
    {
    this->Editor->setView(view);
    this->Dialog->setWindowTitle("Plot Matrix View Settings");
    }
}

void pqActivePlotMatrixViewOptions::closeOptions()
{
  this->Dialog->accept();
}

void pqActivePlotMatrixViewOptions::finishDialog(int result)
{
  if(result != QDialog::Accepted)
    {
    this->Dialog->setApplyNeeded(false);
    }

  emit this->optionsClosed(this);
}

void pqActivePlotMatrixViewOptions::cleanupDialog()
{
  // If the dialog was deleted, the chart options will be deleted as
  // well, which will clean up the chart connections.
  this->Dialog = 0;
}

void pqActivePlotMatrixViewOptions::openUndoSet()
{
}

void pqActivePlotMatrixViewOptions::closeUndoSet()
{
  pqView *view = this->Editor->getView();
  if(view)
    {
    view->getProxy()->UpdateVTKObjects();
    view->render();
    }
}
