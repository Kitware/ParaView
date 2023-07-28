// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqIntRangeWidget_h
#define pqIntRangeWidget_h

#include "pqComponentsModule.h"
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

public:
  /**
   * constructor requires the proxy, property
   */
  pqIntRangeWidget(QWidget* parent = nullptr);
  ~pqIntRangeWidget() override;

  /**
   * get the value
   */
  int value() const;

  // get the min range value
  int minimum() const;
  // get the max range value
  int maximum() const;

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
   * this means the user is done changing text or the user is done moving the
   * slider. It implies value was changed and editing has finished.
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

private: // NOLINT(readability-redundant-access-specifiers)
  int Value;
  int Minimum;
  int Maximum;
  QSlider* Slider;
  pqLineEdit* LineEdit;
  bool BlockUpdate;
  vtkSmartPointer<vtkSMIntRangeDomain> Domain;
  vtkEventQtSlotConnect* DomainConnection;
  bool InteractingWithSlider;
  bool DeferredValueEdited;
};

#endif
