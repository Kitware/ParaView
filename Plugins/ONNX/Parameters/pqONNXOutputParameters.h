// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqONNXOutputParameters_h
#define pqONNXOutputParameters_h

class QJsonObject;

#include "pqONNXPluginParametersModule.h"

/**
 * pqONNXOutputParameters is a structure that extracts information
 * from the json object describing an ONNX output parameters.
 *
 * @see pqONNXJsonVerify for the expected json structure.
 */
class PQONNXPLUGINPARAMETERS_EXPORT pqONNXOutputParameters
{
public:
  /**
   * Construct internal structure from the json object
   * representing the ONNX output.
   * @param obj should be valid as defined by pqONNXJsonVerify.
   */
  pqONNXOutputParameters(const QJsonObject& obj);

  /**
   * Return true if output array should be associated to the cells.
   */
  bool getOnCell() const { return this->OnCell; }

  /**
   * Return the dimension of the output array.
   */
  int getDimension() const { return this->Dimension; }

private:
  bool OnCell = true;
  int Dimension = 1;
};

#endif
