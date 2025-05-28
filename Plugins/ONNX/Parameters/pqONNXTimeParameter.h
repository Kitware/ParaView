// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqONNXTimeParameter_h
#define pqONNXTimeParameter_h

class QJsonObject;

#include <QString> // for QString
#include <QVector> // for QVector

#include "pqONNXPluginParametersModule.h"

/**
 * pqONNXTimeParameter is a structure that extracts information
 * from the json object describing an ONNX time parameter.
 *
 * @see pqONNXJsonVerify for the expected json structure.
 */
class PQONNXPLUGINPARAMETERS_EXPORT pqONNXTimeParameter
{
public:
  /**
   * Construct internal structure from the json object
   * representing a single ONNX parameter.
   * @param obj should be valid as defined by pqONNXJsonVerify.
   */
  pqONNXTimeParameter(const QJsonObject& obj);

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
   * Return the list of times if any.
   */
  const QVector<double>& getTimes() const { return this->Times; }

private:
  QString Name = "";
  double Min = 0;
  double Max = 1;
  QVector<double> Times;
};

#endif
