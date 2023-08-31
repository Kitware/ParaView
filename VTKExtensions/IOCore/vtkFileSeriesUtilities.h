// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkFileSeriesUtilities
 * @brief   A namespace providing tools for file series
 *
 * A namespace providing tools for file series, it provide
 * constexpr related to file series version as well as utility method
 * to check the file series version.
 *
 * The current file series version is 1.0.
 */

#ifndef vtkFileSeriesUtilities_h
#define vtkFileSeriesUtilities_h

#include <string>

namespace vtkFileSeriesUtilities
{
///@{
/**
 * Const variable describing the file series version currently in use
 */
const std::string FILE_SERIES_VERSION = "1.0";
constexpr int FILE_SERIES_VERSION_MAJ = 1;
constexpr int FILE_SERIES_VERSION_MIN = 0;
///@}

/**
 * Inline method to check a provided version against current version in use
 * Return true if current version is equal or higher, false otherwise.
 */
inline bool CheckVersion(const std::string& version)
{
  size_t pos = version.find('.');
  if (pos == std::string::npos)
  {
    return false;
  }
  if (std::atoi(version.substr(0, pos).c_str()) > FILE_SERIES_VERSION_MAJ)
  {
    return false;
  }
  if (version.size() < pos)
  {
    return false;
  }
  if (std::atoi(version.substr(pos + 1).c_str()) > FILE_SERIES_VERSION_MIN)
  {
    return false;
  }
  return true;
}
}

#endif
