// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqNumericParameter_h
#define pqNumericParameter_h

#include "pqONNXPluginParametersModule.h"

#include <QString> // for QString

/**
 * pqNumericParameter is a structure that represents a numerical parameter
 * as a named value inside a range.
 * It is intended to be inherited to parse some external structure.
 *
 * @see pqDoublePropertyMultiWidget for usage.
 */
class PQONNXPLUGINPARAMETERS_EXPORT pqNumericParameter
{
public:
  pqNumericParameter() = default;

  pqNumericParameter(const QString& name, double min, double max, double defaultValue)
    : Name(name)
    , Min(min)
    , Max(max)
    , Default(defaultValue)
  {
  }

  /**
   * Get the parameter name.
   */
  QString getName() const { return this->Name; }

  /**
   * Get the parameter minimum value.
   */
  double getMin() const { return this->Min; }

  /**
   * Get the parameter maximum value.
   */
  double getMax() const { return this->Max; }

  /**
   * Get the parameter default value.
   */
  double getDefault() const { return this->Default; }

protected:
  void setName(const QString& name) { this->Name = name; }

  void setMin(double min) { this->Min = min; }

  void setMax(double max) { this->Max = max; }

  void setDefault(double value) { this->Default = value; }

private:
  QString Name = "";
  double Min = 0;
  double Max = 1;
  double Default = 0.5;
};

#endif
