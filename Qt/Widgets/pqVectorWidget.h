// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVectorWidget_h
#define pqVectorWidget_h

#include "pqWidgetsModule.h"

#include <QVariant>
#include <QVector2D>
#include <QVector3D>
#include <QWidget>

/**
 * pqVectorWidget is an abstract widget that can be used to edit a vector.
 */
class PQWIDGETS_EXPORT pqVectorWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QVariant value READ value USER true);

public:
  pqVectorWidget(const QVariant& value, QWidget* parent = nullptr);
  ~pqVectorWidget() override = default;

  /**
   * Get the value directly in vector form.
   */
  const QVariant& value() const { return this->Vector; }

Q_SIGNALS:
  /**
   * Signal emitted when any component value changed.
   */
  void valueChanged(const QVariant&);

public Q_SLOTS:

  /**
   * Set the value at index \p index of the QVariant with \p value.
   * Must be overidden to cast to the specialized QVariant.
   */
  virtual void setValue(int index, float value) = 0;

protected:
  QVariant Vector;

  /**
   * This function creates an horizontal layout with \p nbElem
   * spinboxes
   */
  void CreateUI(unsigned int nbElem);
  virtual float getValue(int index) = 0;

private:
  Q_DISABLE_COPY(pqVectorWidget);
};

/**
 * pqVectorWidgetImpl is a templated class inherited from pqVectorWidget
 * to be used with QVector classes or QQuaternion.
 */
template <class T, unsigned int S>
class pqVectorWidgetImpl : public pqVectorWidget
{
public:
  pqVectorWidgetImpl(const T& value, QWidget* parent = nullptr)
    : pqVectorWidget(value, parent)
  {
    // Create the UI with the specified templated number of elements
    this->CreateUI(S);
  }

  T value() const { return this->Vector.template value<T>(); }

  void setValue(int index, float value) override
  {
    T vec = this->Vector.template value<T>();
    vec[index] = value;
    this->Vector = QVariant::fromValue(vec);
    Q_EMIT this->valueChanged(this->Vector);
  }

protected:
  float getValue(int index) override { return this->Vector.template value<T>()[index]; }
};

#endif
