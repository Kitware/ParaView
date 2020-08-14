/*=========================================================================

   Program:   ParaView
   Module:    pqIntRangeWidget.h

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
#ifndef pqIntRangeWidget_h
#define pqIntRangeWidget_h

#include "pqComponentsModule.h"
#include "vtkSetGet.h" // for VTK_LEGACY
#include "vtkSmartPointer.h"
#include <QWidget>

class QSlider;
class pqLineEdit;
class vtkSMIntRangeDomain;
class vtkEventQtSlotConnect;

/**
* a widget with a tied slider and line edit for editing a int property
*/
class PQCOMPONENTS_EXPORT pqIntRangeWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(int value READ value WRITE setValue USER true)
  Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
  Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
#if !defined(VTK_LEGACY_REMOVE)
  Q_PROPERTY(bool strictRange READ strictRange WRITE setStrictRange);
#endif

public:
  /**
  * constructor requires the proxy, property
  */
  pqIntRangeWidget(QWidget* parent = NULL);
  ~pqIntRangeWidget() override;

  /**
  * get the value
  */
  int value() const;

  // get the min range value
  int minimum() const;
  // get the max range value
  int maximum() const;

  /**
   * @deprecated strictRange is deprecated and is not working as intended
   */
  VTK_LEGACY(bool strictRange() const);

  // Sets the range domain to monitor. This will automatically update
  // the widgets range when the domain changes.
  void setDomain(vtkSMIntRangeDomain* domain);

Q_SIGNALS:
  /**
  * signal the value changed
  */
  void valueChanged(int);

  /**
  * signal the value was edited
  * this means the user is done changing text
  * or the user is done moving the slider. It implies
  * value was changed and editing has finished.
  */
  void valueEdited(int);

public Q_SLOTS:
  /**
  * set the value
  */
  void setValue(int);

  // set the min range value
  void setMinimum(int);
  // set the max range value
  void setMaximum(int);

#if !defined(VTK_LEGACY_REMOVE)
  /**
   * @deprecated strictRange is deprecated and is not working as intended
   */
  void setStrictRange(bool);
#endif

private Q_SLOTS:
  void sliderChanged(int);
  void textChanged(const QString&);
  void editingFinished();
  void updateValidator();
  void domainChanged();
  void emitValueEdited();
  void emitIfDeferredValueEdited();
  void sliderPressed();
  void sliderReleased();

private:
  int Value;
  int Minimum;
  int Maximum;
  QSlider* Slider;
  pqLineEdit* LineEdit;
  bool BlockUpdate;
#if !defined(VTK_LEGACY_REMOVE)
  bool StrictRange = false;
#endif
  vtkSmartPointer<vtkSMIntRangeDomain> Domain;
  vtkEventQtSlotConnect* DomainConnection;
  bool InteractingWithSlider;
  bool DeferredValueEdited;
};

#endif
