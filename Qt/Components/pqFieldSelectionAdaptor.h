/*=========================================================================

   Program:   ParaView
   Module:    pqFieldSelectionAdaptor.h

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

#ifndef pq_FieldSelectionAdaptor_h
#define pq_FieldSelectionAdaptor_h

#include <QObject>
#include <QString>
#include <QStringList>
class QComboBox;
#include "vtkSmartPointer.h"
class vtkSMProperty;
class vtkSMDomain;
class vtkObject;
class vtkCommand;
class vtkEventQtSlotConnect;
#include "pqComponentsModule.h"

/**
* adaptor to which combines cell & point arrays into one selection
* this adaptor also takes care of the domain, so there's no need to
* use the pqComboBoxDomain
*/
class PQCOMPONENTS_EXPORT pqFieldSelectionAdaptor : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString attributeMode READ attributeMode WRITE setAttributeMode)
  Q_PROPERTY(QString scalar READ scalar WRITE setScalar)
  Q_PROPERTY(QStringList selection READ selection WRITE setSelection)

public:
  /**
  * constructor requires a QComboBox,
  */
  pqFieldSelectionAdaptor(QComboBox* p, vtkSMProperty* prop);
  ~pqFieldSelectionAdaptor();

  /**
  * get the attribute mode
  */
  QString attributeMode() const;
  /**
  * get the scalar
  */
  QString scalar() const;

  void setSelection(const QStringList& selection);
  QStringList selection() const;

signals:
  /**
  * signal the attributeMode and/or scalar changed
  */
  void selectionChanged();

public slots:
  /**
  * set the attribute mode
  */
  void setAttributeMode(const QString&);
  /**
  * set the scalar mode
  */
  void setScalar(const QString&);

  // set the attribute mode and scalar
  void setAttributeModeAndScalar(const QString& mode, const QString& scalar);

protected slots:
  void updateGUI();
  void indexChanged(int index);

  void domainChanged();
private slots:
  void internalDomainChanged();
  void blockDomainModified(vtkObject* caller, unsigned long, void*, void*, vtkCommand*);

protected:
  QStringList Selection;
  vtkSmartPointer<vtkSMProperty> Property;
  vtkSmartPointer<vtkSMDomain> AttributeModeDomain;
  vtkSmartPointer<vtkSMDomain> ScalarDomain;
  vtkEventQtSlotConnect* Connection;
  bool MarkedForUpdate;
  bool IsGettingAllDomains;
};

#endif // pq_FieldSelectionAdaptor_h
