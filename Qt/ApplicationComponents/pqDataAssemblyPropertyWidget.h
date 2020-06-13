/*=========================================================================

   Program: ParaView
   Module:  pqDataAssemblyPropertyWidget.h

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
#ifndef pqDataAssemblyPropertyWidget_h
#define pqDataAssemblyPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

#include <QScopedPointer> // for QScopedPointer

/**
 * @class pqDataAssemblyPropertyWidget
 * @brief pqPropertyWidget for properties with vtkDataAssembly
 *
 * pqDataAssemblyPropertyWidget is intended for properties that rely on a
 * vtkDataAssembly i.e. use a vtkSMDataAssemblyDomain. Currently, this only
 * support getting/setting the list of chosen paths based on the
 * vtkDataAssembly. In the future, we will add support for additional properties
 * such as color, opacity etc.
 */
class vtkObject;
class PQAPPLICATIONCOMPONENTS_EXPORT pqDataAssemblyPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QList<QVariant> chosenPaths READ chosenPathsAsVariantList WRITE setChosenPaths NOTIFY
      chosenPathsChanged);

public:
  pqDataAssemblyPropertyWidget(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  pqDataAssemblyPropertyWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqDataAssemblyPropertyWidget() override;

  //@{
  /**
   * API for getting/setting selected/chosen path strings.
   */
  void setChosenPaths(const QStringList& paths);
  const QStringList& chosenPaths() const;
  void setChosenPaths(const QList<QVariant>& paths);
  QList<QVariant> chosenPathsAsVariantList() const;
  //@}

Q_SIGNALS:
  void chosenPathsChanged();

private Q_SLOTS:
  void updateDataAssembly(vtkObject* sender);
  void assemblyTreeModified(int role);
  void stringListModified();

private:
  Q_DISABLE_COPY(pqDataAssemblyPropertyWidget);
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
