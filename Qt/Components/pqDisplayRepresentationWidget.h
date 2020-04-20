/*=========================================================================

   Program: ParaView
   Module:    pqDisplayRepresentationWidget.h

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
#ifndef pqDisplayRepresentationWidget_h
#define pqDisplayRepresentationWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"
#include <QWidget>

class pqDataRepresentation;
class vtkSMProxy;

/**
* A widget for representation type for a vtkSMRepresentationProxy. It works
* with a vtkSMRepresentationProxy, calling
* vtkSMRepresentationProxy::SetRepresentationType() to change the
* representation type to the one chosen by the user.
*/
class PQCOMPONENTS_EXPORT pqDisplayRepresentationWidget : public QWidget
{
  Q_OBJECT;
  Q_PROPERTY(QString representationText READ representationText WRITE setRepresentationText NOTIFY
      representationTextChanged);
  typedef QWidget Superclass;

public:
  pqDisplayRepresentationWidget(QWidget* parent = 0);
  ~pqDisplayRepresentationWidget() override;

  /**
  * Returns the selected representation as a string.
  */
  QString representationText() const;

public Q_SLOTS:
  /**
  * set the representation proxy or pqDataRepresentation instance.
  */
  void setRepresentation(pqDataRepresentation* display);
  void setRepresentation(vtkSMProxy* proxy);

  /**
  * set representation type.
  */
  void setRepresentationText(const QString&);

private Q_SLOTS:
  /**
  * Slot called when the combo-box is changed. If this change was due to
  * a UI interaction, we need to prompt the user if he really intended to make
  * that change (BUG #0015117).
  */
  void comboBoxChanged(const QString&);

Q_SIGNALS:
  void representationTextChanged(const QString&);

private:
  Q_DISABLE_COPY(pqDisplayRepresentationWidget)

  class pqInternals;
  pqInternals* Internal;

  class PropertyLinksConnection;
};

/**
* A property widget for selecting the display representation.
*/
class PQCOMPONENTS_EXPORT pqDisplayRepresentationPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  pqDisplayRepresentationPropertyWidget(vtkSMProxy* proxy, QWidget* parent = 0);
  ~pqDisplayRepresentationPropertyWidget() override;

private:
  pqDisplayRepresentationWidget* Widget;
};

#endif
