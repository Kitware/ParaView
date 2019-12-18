/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMessage.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// bring in the definition for VTK_PROTOBUF_EXPORT.
#include "vtkSMMessage.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports

// The actual implementation is the file generated from "protoc".
#include "vtkPVMessage.pb.cc"
