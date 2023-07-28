// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSciVizStatisticsPrivate
 * @brief   Private class for scientific viz statistics classes.
 *
 * This class handles array selection in a way that makes ParaView happy.
 */

#ifndef vtkSciVizStatisticsPrivate_h
#define vtkSciVizStatisticsPrivate_h

#include "vtkStatisticsAlgorithmPrivate.h"

class vtkSciVizStatisticsP : public vtkStatisticsAlgorithmPrivate
{
public:
  bool Has(std::string arrName) { return this->Buffer.find(arrName) != this->Buffer.end(); }
};

#endif // vtkSciVizStatisticsPrivate_h

// VTK-HeaderTest-Exclude: vtkSciVizStatisticsPrivate.h
