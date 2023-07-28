// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonDebugLeaksView_h
#define pqPythonDebugLeaksView_h

#include "vtkQtDebugLeaksView.h"

#include "pqPythonModule.h" // for exports
#include <QPointer>         // for QPointer

/**
 * @class pqPythonDebugLeaksView
 * @brief Widget to track VTK object references.
 *
 * pqPythonDebugLeaksView extends `vtkQtDebugLeaksView` to add support for
 * double clicking on a row and adding corresponding objects to the
 * interpreter in a Python shell. The Python shell must be provided using
 * `pqPythonDebugLeaksView::setShell`.
 */
class pqPythonShell;
class PQPYTHON_EXPORT pqPythonDebugLeaksView : public vtkQtDebugLeaksView
{
  Q_OBJECT

public:
  pqPythonDebugLeaksView(QWidget* p = nullptr);
  ~pqPythonDebugLeaksView() override;

  void setShell(pqPythonShell*);
  pqPythonShell* shell() const;

protected:
  void onObjectDoubleClicked(vtkObjectBase* object) override;
  void onClassNameDoubleClicked(const QString& className) override;

  virtual void addObjectToPython(vtkObjectBase* object);
  virtual void addObjectsToPython(const QList<vtkObjectBase*>& objects);

  QPointer<pqPythonShell> Shell;
};

#endif // !pqPythonDebugLeaksView_h
