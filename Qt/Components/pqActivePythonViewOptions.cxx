/*=========================================================================

  Program:   ParaView
  Module:    pqActivePythonViewOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqActivePythonViewOptions.h"

#include <QPointer>

#include "pqOptionsDialog.h"
#include "pqPythonViewOptions.h"

class pqActivePythonViewOptions::pqInternal
{
public:
  QPointer<pqOptionsDialog> Dialog;
  pqPythonViewOptions* Options;
};

pqActivePythonViewOptions::pqActivePythonViewOptions(QObject *parentObject)
  : pqActiveViewOptions(parentObject)
{
  this->Internal = new pqInternal;
}

pqActivePythonViewOptions::~pqActivePythonViewOptions()
{
  delete this->Internal;
}

void pqActivePythonViewOptions::showOptions(
    pqView *view, const QString &page, QWidget *widgetParent)
{
  if(!this->Internal->Dialog)
    {
    this->Internal->Dialog = new pqOptionsDialog(widgetParent);
    this->Internal->Dialog->setApplyNeeded(true);
    this->Internal->Dialog->setObjectName("ActiveRenderViewOptions");
    this->Internal->Dialog->setWindowTitle("View Settings (Render View)");
    this->Internal->Options = new pqPythonViewOptions;
    this->Internal->Dialog->addOptions(this->Internal->Options);
    if(page.isEmpty())
      {
      QStringList pages = this->Internal->Options->getPageList();
      if(pages.size())
        {
        this->Internal->Dialog->setCurrentPage(pages[0]);
        }
      }
    else
      {
      this->Internal->Dialog->setCurrentPage(page);
      }

    this->connect(this->Internal->Dialog, SIGNAL(finished(int)),
        this, SLOT(finishDialog()));
    }

  this->changeView(view);
  this->Internal->Dialog->show();
}

void pqActivePythonViewOptions::changeView(pqView *view)
{
  this->Internal->Options->setView(view);
}

void pqActivePythonViewOptions::closeOptions()
{
  if(this->Internal->Dialog)
    {
    this->Internal->Dialog->accept();
    }
}

void pqActivePythonViewOptions::finishDialog()
{
  emit this->optionsClosed(this);
}
