// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// bring in the definition for VTK_PROTOBUF_EXPORT.
#include "vtkSMMessage.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports

// The actual implementation is the file generated from "protoc".
#include "vtkPVMessage.pb.cc"
