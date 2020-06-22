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
class QStandardItem;

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
  ~pqYoungsMaterialPropertyWidget() override;

  //@{
  /**
   * Get/Set the ordering arrays, linked to the SMProperty
   */
  void setOrderingArrays(const QList<QVariant>&);
  QList<QVariant> orderingArrays() const;
  //@}

  //@{
  /**
   * Get/Set the normal arrays, linked to the SMProperty
   */
  void setNormalArrays(const QList<QVariant>&);
  QList<QVariant> normalArrays() const;
  //@}

Q_SIGNALS:

  /**
   * Emitted when the ordering arrays change, linked ot the SMProperty
   */
  void orderingArraysChanged();

  /**
   * Emitted when the normal arrays change, linked ot the SMProperty
   */
  void normalArraysChanged();

protected:
  /**
   * Recover the currently selecticted item in the VolumeFractionArrays widget
   */
  QStandardItem* currentItem();

protected slots:
  /**
   * Called when the ordering array is changed
   */
  void onOrderingArraysChanged();

  /**
   * Called when the normal array is changed
   */
  void onNormalArraysChanged();

  /**
   * Called to initialize/update the combobox according to the current item
   */
  void updateComboBoxes();

private:
  Q_DISABLE_COPY(pqYoungsMaterialPropertyWidget)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
