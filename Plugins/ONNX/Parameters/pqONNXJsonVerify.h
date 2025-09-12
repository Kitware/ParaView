// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqONNXJsonVerify_h
#define pqONNXJsonVerify_h

#include "pqONNXPluginParametersModule.h"

class QJsonObject;

/**
 * Helper methods to check that the input json matches the expected structure.
 *
 * Here is a sample of the Json format:
 * @code{.json}
 * {
 * "Version": {
 *   "Major": int,
 *   "Minor": int,
 *   "Patch": int
 * },
 * "Input": [
 *   {
 *     "Name": str,
 *     "Min": double,
 *     "Max": double,
 *     "Default": double
 *   },
 *   ...
 *   ,
 *   { "Name": str
 *     "IsTime": bool,
 *     <opt>"NumSteps": int
 *     <opt>"Min": double,
 *     <opt>"Max": double,
 *     <opt>"TimeValues": [int]
 *   }
 * ],
 * "Output": {
 *   "OnCellData": bool,
 *   "Dimension": int >= 1,
 * }
 *
 * @endcode
 *
 * The objects under the "Input" are parameters for ONNX. One of them
 * may be a Time parameter: it has no "Default" but a "IsTime" set to "true".
 * The Time parameter should have either an explicit "TimeValues" array or
 * define a range with NumSteps, Min and Max values.
 *
 * Any extra fields are silently ignored.
 */
namespace pqONNXJsonVerify
{
constexpr int Major = 1;
constexpr int Minor = 0;
constexpr int Patch = 0;

/**
 * Return true if the json matches the current specification.
 *
 * Return false if any error is encountered.
 * Extra fields are silently ignored.
 */
bool PQONNXPLUGINPARAMETERS_EXPORT check(const QJsonObject& root);

/**
 * Return true if the parameter object describes a list of Times.
 *
 * @param parameter is an object form the "Input" array.
 */
bool PQONNXPLUGINPARAMETERS_EXPORT isTime(const QJsonObject& parameter);

/**
 * Return true if the current implementation supports the given version.
 * @param version is the "Version" object.
 */
bool PQONNXPLUGINPARAMETERS_EXPORT isSupportedVersion(const QJsonObject& version);
};

#endif
// VTK-HeaderTest-Exclude: pqONNXJsonVerify.h
