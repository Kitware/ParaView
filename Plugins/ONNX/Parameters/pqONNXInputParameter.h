// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqONNXInputParameter_h
#define pqONNXInputParameter_h

#include "pqNumericParameter.h"
#include "pqONNXPluginParametersModule.h"

class QJsonObject;

/**
 * pqONNXInputParameter is a structure that extracts information
 * from the json object describing an ONNX input parameter.
 *
 * @see pqONNXJsonVerify for the expected json structure.
 */
class PQONNXPLUGINPARAMETERS_EXPORT pqONNXInputParameter : public pqNumericParameter
{
public:
  /**
   * Construct internal structure from the json object
   * representing a single ONNX parameter.
   * @param obj should be valid as defined by pqONNXJsonVerify.
   */
  pqONNXInputParameter(const QJsonObject& obj);
};
#endif
