/*=========================================================================

   Program: ParaView
   Module:  pqCompositeTreePropertyWidget.h

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
#ifndef pqCompositeTreePropertyWidget_h
#define pqCompositeTreePropertyWidget_h

#include "pqPropertyWidget.h"
#include "pqTimer.h"        // needed for pqTimer.
#include "vtkNew.h"         // needed for vtkNew.
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkSMIntVectorProperty;
class vtkEventQtSlotConnect;
class vtkSMCompositeTreeDomain;
class pqCompositeDataInformationTreeModel;
class pqTreeView;

/**
 * @class pqCompositeDataInformationTreeModel
 * @brief property widget for a vtkSMIntVectorProperty with vtkSMCompositeTreeDomain.
 *
 * pqCompositeDataInformationTreeModel is a subclass of pqPropertyWidget that is
 * created by default for any vtkSMIntVectorProperty having a
 * vtkSMCompositeTreeDomain. This widgets creates a tree view which shows the
 * composite-dataset hierarchy. It supports both multiblock and AMR datasets
 * with ability to select blocks either using flat or composite index or
 * AMR-specific, level number or level number and block number.
 *
 * Internally, it uses pqCompositeDataInformationTreeModel and keeps it updated
 * as the vtkSMCompositeTreeDomain changes.
 */
class PQCOMPONENTS_EXPORT pqCompositeTreePropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QList<QVariant> values READ values WRITE setValues NOTIFY valuesChanged);

public:
  pqCompositeTreePropertyWidget(
    vtkSMIntVectorProperty* smproperty, vtkSMProxy* smproxy, QWidget* parent = nullptr);
  ~pqCompositeTreePropertyWidget() override;

  /**
   * API to get/set the selected values. What there values represents depends on
   * the domain and the property e.g. it can represent flat indices or amr-level
   * number or (amr-level, amr-block) pairs.
   */
  QList<QVariant> values() const;
  void setValues(const QList<QVariant>& values);

Q_SIGNALS:
  void valuesChanged();

private Q_SLOTS:
  void domainModified();

private:
  Q_DISABLE_COPY(pqCompositeTreePropertyWidget);

  pqTimer Timer;
  vtkNew<vtkEventQtSlotConnect> VTKConnect;
  vtkWeakPointer<vtkSMCompositeTreeDomain> Domain;
  vtkWeakPointer<vtkSMIntVectorProperty> Property;
  QPointer<pqCompositeDataInformationTreeModel> Model;
  QPointer<pqTreeView> TreeView;
  int DepthExpansion = 2;
};

#endif
