/*=========================================================================

  Program:   ParaView
  Module:    pqPythonViewOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqPythonViewOptions.h"
#include "ui_pqPythonViewOptions.h"

#include "vtkSMPropertyHelper.h"

#include <QHBoxLayout>

#include "pqPythonView.h"

#include "pqActiveView.h"

class pqPythonViewOptions::pqInternal
{
public:
  Ui::pqPythonViewOptions ui;
};

//----------------------------------------------------------------------------
pqPythonViewOptions::pqPythonViewOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal();
  this->Internal->ui.setupUi(this);

  QObject::connect(this->Internal->ui.PythonScriptTextEdit,
                   SIGNAL(textChanged()),
                   this, SIGNAL(changesAvailable()));

}

//----------------------------------------------------------------------------
pqPythonViewOptions::~pqPythonViewOptions()
{
}

//----------------------------------------------------------------------------
void pqPythonViewOptions::setPage(const QString&)
{
}

//----------------------------------------------------------------------------
QStringList pqPythonViewOptions::getPageList()
{
  QStringList ret;
  ret << "Python View";
  return ret;
}

//----------------------------------------------------------------------------
void pqPythonViewOptions::applyChanges()
{
  if(!this->View)
    {
    return;
    }

  // Update View options...
  QString pythonScript(this->Internal->ui.PythonScriptTextEdit->toPlainText());
  this->View->setPythonScript(pythonScript);

  this->View->render();
}

//----------------------------------------------------------------------------
void pqPythonViewOptions::resetChanges()
{
  if(!this->View)
    {
    return;
    }

  this->setView(this->View);
}

//----------------------------------------------------------------------------
void pqPythonViewOptions::setView(pqView* view)
{
  this->View = qobject_cast<pqPythonView*>(view);

  if(!this->View)
    {
    return;
    }

  // Update UI with the current values
  this->Internal->ui.PythonScriptTextEdit->setText(this->View->getPythonScript());
}
