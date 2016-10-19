/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTConstants.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   vtkIceTConstants
 * @brief   Keeper of constants for IceT classes.
 *
 *
 *
 * vtkIceTConstants is not really meant to be used as a class.  Rather, it
 * simply holds constants that can be used with the IceT classes.  These
 * constants are defined outside of the vtkIceTRenderManager and
 * vtkOpenGLIceTRenderer definitions so that they can be accessed when the IceT
 * implementation classes are not compiled.  Because ParaView is designed to run
 * client/server and because the server should be able to use its IceT
 * implementation even when the client is not compiled with IceT.  In this case,
 * the client needs to send flags to the server.
 *
 * Those flags are defined here.  This class does not have any dependence on any
 * MPI or IceT libraries, so can be used on either client or server.
 *
 * @sa
 * vtkIceTRenderManager, vtkOpenGLIceTRenderer
 *
*/

#ifndef vtkIceTConstants_h
#define vtkIceTConstants_h
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkIceTConstants
{
public:
  enum StrategyType
  {
    DEFAULT = 0,
    REDUCE = 1,
    VTREE = 2,
    SPLIT = 3,
    SEQUENTIAL = 4,
    DIRECT = 5
  };

  enum ComposeOperationType
  {
    ComposeOperationClosest = 0,
    ComposeOperationOver = 1
  };
};

#endif // vtkIceTConstants_h

// VTK-HeaderTest-Exclude: vtkIceTConstants.h
