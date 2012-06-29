/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Include this header in your header files where you are referring to
// vtkSMMessage or vtkSMMessageCollection. It simply forward declares these
// without including any protobuf headers. In your cxx files, you need to
// include vtkSMMessage.h to see the full definition of the protobuf message
// classes as well as other operators.

#ifndef __vtkSMMessageMinimal_h
#define __vtkSMMessageMinimal_h

#include "vtkSystemIncludes.h"

// Define the export Macro
#if defined(VTK_BUILD_SHARED_LIBS)
  #if defined(vtkPVServerImplementation_EXPORTS)
    #define VTK_PROTOBUF_EXPORT VTK_ABI_EXPORT
  #else
    #define VTK_PROTOBUF_EXPORT VTK_ABI_IMPORT
  #endif
#else
  #define VTK_PROTOBUF_EXPORT VTK_EXPORT
#endif
namespace paraview_protobuf
{
  class Message;
  class MessageCollection;
}

typedef paraview_protobuf::Message vtkSMMessage;
typedef paraview_protobuf::MessageCollection vtkSMMessageCollection;

#endif
