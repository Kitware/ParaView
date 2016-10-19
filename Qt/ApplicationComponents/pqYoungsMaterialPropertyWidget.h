/*=========================================================================

   Program: ParaView
   Module:  pqYoungsMaterialPropertyWidget.h

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
#ifndef pqYoungsMaterialPropertyWidget_h
#define pqYoungsMaterialPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqStringVectorPropertyWidget.h"

#include <QScopedPointer>

class vtkSMPropertyGroup;

/**
* This is a custom widget for YoungsMaterialInterface filter. We use a custom widget
* since this filter has unusual requirements when it comes to setting
* OrderingArrays and NormalArrays properties.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqYoungsMaterialPropertyWidget
  : public pqStringVectorPropertyWidget
{
  Q_OBJECT
  typedef pqStringVectorPropertyWidget Superclass;
  Q_PROPERTY(QList<QVariant> orderingArrays READ orderingArrays WRITE setOrderingArrays NOTIFY
      orderingArraysChanged);
  Q_PROPERTY(QList<QVariant> normalArrays READ normalArrays WRITE setNormalArrays NOTIFY
      normalArraysChanged);

public:
  pqYoungsMaterialPropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* group, QWidget* parent = 0);
  virtual ~pqYoungsMaterialPropertyWidget();

  void setOrderingArrays(const QList<QVariant>&);
  QList<QVariant> orderingArrays() const;

  void setNormalArrays(const QList<QVariant>&);
  QList<QVariant> normalArrays() const;

signals:
  void normalArraysChanged();
  void orderingArraysChanged();

private slots:
  void normalArraysChanged(const QString& value);
  void orderingArraysChanged(const QString& value);
  void updateComboBoxes();

private:
  Q_DISABLE_COPY(pqYoungsMaterialPropertyWidget)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
