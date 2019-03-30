/*=========================================================================

  Program:   ParaView
  Module:    pqPythonView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef pqPythonView_h
#define pqPythonView_h

#include "pqSMProxy.h"
#include "pqView.h"

#include <cassert>

class vtkSMPythonViewProxy;

class PQCORE_EXPORT pqPythonView : public pqView
{
  Q_OBJECT
  typedef pqView Superclass;

public:
  static QString pythonViewType() { return "PythonView"; }

  // Constructor:
  // \c type  :- view type.
  // \c group :- SManager registration group name.
  // \c name  :- SManager registration name.
  // \c view  :- RenderView proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqPythonView(const QString& type, const QString& group, const QString& name,
    vtkSMViewProxy* renModule, pqServer* server, QObject* parent = NULL);

  // Destructor.
  ~pqPythonView() override;

  /**
  * Set/get the Python script
  */
  void setPythonScript(QString& script);
  QString getPythonScript();

  /**
  * Get the view proxy as a vtkSMPythonViewProxy
  */
  vtkSMPythonViewProxy* getPythonViewProxy();

protected:
  /**
  * Overridden to popup the context menu, if some actions have been added
  * using addMenuAction.
  */
  bool eventFilter(QObject* caller, QEvent* e) override;

  /**
  * Creates a new instance of the QWidget subclass to be used to show this
  * view.
  */
  QWidget* createWidget() override;

private:
  Q_DISABLE_COPY(pqPythonView)

  class pqInternal;
  pqInternal* Internal;
};

#endif
