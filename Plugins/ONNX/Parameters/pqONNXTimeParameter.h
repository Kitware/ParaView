// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqONNXTimeParameter_h
#define pqONNXTimeParameter_h

#include "pqNumericParameter.h"

#include <QVector> // for QVector

#include "pqONNXPluginParametersModule.h"

class QJsonObject;

/**
 * pqONNXTimeParameter is a structure that extracts information
 * from the json object describing an ONNX time parameter.
 *
 * @see pqONNXJsonVerify for the expected json structure.
 */
class PQONNXPLUGINPARAMETERS_EXPORT pqONNXTimeParameter : public pqNumericParameter
{
public:
  /**
   * Construct internal structure from the json object
   * representing a single ONNX parameter.
   * @param obj should be valid as defined by pqONNXJsonVerify.
   */
  pqONNXTimeParameter(const QJsonObject& obj);

  /**
   * Return the list of times if any.
   */
  const QVector<double>& getTimes() const { return this->Times; }

private:
  QVector<double> Times;
};

#endif
