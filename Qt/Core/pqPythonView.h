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
#ifndef __pqPythonView_h
#define __pqPythonView_h

#include "pqView.h"
#include "pqSMProxy.h"

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
  pqPythonView(const QString& type,
               const QString& group,
               const QString& name, 
               vtkSMViewProxy* renModule, 
               pqServer* server, 
               QObject* parent=NULL);

  // Destructor.
  virtual ~pqPythonView();

  /// Set/get the Python script
  void setPythonScript(QString & script);
  QString getPythonScript();

  /// Returns the QVTKWidget for this render Window.
  virtual QWidget* getWidget();

  /// Get the view proxy as a vtkSMPythonViewProxy
  vtkSMPythonViewProxy* getPythonViewProxy();

protected slots:
  virtual void initializeAfterObjectsCreated();

  /// Setups up RenderModule and QVTKWidget binding.
  /// This method is called for all pqRenderView objects irrespective
  /// of whether it is created from state/undo-redo/python or by the GUI. Hence
  /// don't change any render module properties here.
  virtual void initializeWidgets();

protected:
  /// Overridden to popup the context menu, if some actions have been added
  /// using addMenuAction.
  virtual bool eventFilter(QObject* caller, QEvent* e);

  /// Creates a new instance of the QWidget subclass to be used to show this
  /// view. Default implementation creates a QVTKWidget.
  virtual QWidget* createWidget();

  /// Use this method to initialize the pqObject state using the
  /// underlying vtkSMProxy. This needs to be done only once,
  /// after the object has been created. 
  virtual void initialize();

  /// On Mac, we usually try to cache the front buffer to avoid unecessary
  //  updates.
  bool AllowCaching;

private: 
  pqPythonView(const pqPythonView&); // Not implemented.
  void operator=(const pqPythonView&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif
