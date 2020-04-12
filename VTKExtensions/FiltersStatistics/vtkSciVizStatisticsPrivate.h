/*=========================================================================

  Program:   ParaView
  Module:    vtkSciVizStatisticsPrivate.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
