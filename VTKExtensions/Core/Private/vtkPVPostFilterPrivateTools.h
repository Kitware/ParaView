// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVPostFilterPrivateTools
 * @brief   Post Filter private tools
 *
 * vtkPVPostFilterPrivateTools is a private namespace used by
 * vtkPVPostFilter and vtkPVPostFilterExecutive
 */

#ifndef vtkPVPostFilterPrivateTools_h
#define vtkPVPostFilterPrivateTools_h

#include <string>

class vtkDataSetAttributes;
class vtkDataSet;
class vtkInformation;
class vtkInformationInformationVectorKey;
class vtkDataObject;

namespace vtkPVPostFilterPrivateTools
{
/**
 * Return default component name for provided number and count
 */
std::string DefaultComponentName(int componentNumber, int componentCount);

/**
 * Demangles a mangled string containing an array name and a component name.
 */
void DeMangleArrayName(const std::string& mangledName, vtkDataSet* dataSet,
  std::string& demangledName, std::string& demangledComponentName);

/**
 * Extract a specific component for demangled name
 */
bool ExtractComponent(vtkDataSetAttributes* dsa, const char* requested_name,
  const char* demangled_name, const char* demangled_component_name);

/**
 * Convert a name cell data to point data and add to output
 */
void CellDataToPointData(vtkDataSet* output, const char* name);

/**
 * Convert a name point data to cell data and add to output
 */
void PointDataToCellData(vtkDataSet* output, const char* name);

/**
 * Do the conversion for the requested name and field association, on the provided output.
 * If checkOnly is true, only check if conversions would occur.
 * Return true if conversion occurs, false otherwise.
 */
bool DoAnyNeededConversions(vtkDataSet* output, const char* requested_name, int fieldAssociation,
  const char* demangled_name, const char* demangled_component_name, bool checkOnly);

/**
 * Do the conversion for the requested info, on the provided output.
 * If checkOnly is true, only check if conversions would occur.
 * Return true if conversion occurs, false otherwise.
 */
bool DoAnyNeededConversions(vtkInformation* info, vtkInformationInformationVectorKey* key,
  vtkDataSet* dataset, bool checkOnly);

/**
 * Do the conversions for the requested info, on the provided output.
 * If output is composite, process all, if not, process as a dataset.
 * If checkOnly is true, only check if conversions would occur.
 * Return true if conversion occurs, false otherwise.
 */
bool DoAnyNeededConversions(vtkInformation* info, vtkInformationInformationVectorKey* key,
  vtkDataObject* output, bool checkOnly = false);
};

#endif
