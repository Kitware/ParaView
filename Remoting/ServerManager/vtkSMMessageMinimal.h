// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Include this header in your header files where you are referring to
// vtkSMMessage or vtkSMMessageCollection. It simply forward declares these
// without including any protobuf headers. In your cxx files, you need to
// include vtkSMMessage.h to see the full definition of the protobuf message
// classes as well as other operators.

#ifndef vtkSMMessageMinimal_h
#define vtkSMMessageMinimal_h

#include "vtkSystemIncludes.h"

namespace paraview_protobuf
{
class Message;
class MessageCollection;
}

typedef paraview_protobuf::Message vtkSMMessage;
typedef paraview_protobuf::MessageCollection vtkSMMessageCollection;

#endif

// VTK-HeaderTest-Exclude: vtkSMMessageMinimal.h
