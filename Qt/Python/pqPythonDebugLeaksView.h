/*=========================================================================

   Program: ParaView
   Module:    pqPythonDebugLeaksView.h

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
