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
// .NAME vtkSciVizStatisticsPrivate - Private class for scientific viz statistics classes.
// .SECTION Description
// This class handles array selection in a way that makes ParaView happy.

#ifndef __vtkSciVizStatisticsPrivate_h
#define __vtkSciVizStatisticsPrivate_h

#include "vtkStatisticsAlgorithmPrivate.h"

class vtkSciVizStatisticsP : public vtkStatisticsAlgorithmPrivate
{
public:
  bool Has( vtkStdString arrName )
    {
    return this->Buffer.find( arrName ) != this->Buffer.end();
    }
};


#endif // __vtkSciVizStatisticsPrivate_h
