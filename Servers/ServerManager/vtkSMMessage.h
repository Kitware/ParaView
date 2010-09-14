/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMessage.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMessage
// .SECTION Description
// Header class that setup every thing in order to use Protobuf messages in
// a transparent manner

#ifndef __vtkSMMessage_h
#define __vtkSMMessage_h

#include <string>
#if __GNUC__
# pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#include "vtkPVMessage.pb.h"
#if __GNUC__
# pragma GCC diagnostic warning "-Wsign-compare"
#endif

#include "vtkClientServerStream.h"

typedef paraview_protobuf::Message           vtkSMMessage;
typedef paraview_protobuf::MessageCollection vtkSMMessageCollection;

namespace pvstream
{
  // **** Operators to simplify construction of a InvokeRequest message ****/
  struct InvokeRequest { };
  struct _InvokeRequest { vtkSMMessage* _message; };
  struct _InvokeRequestArgs { paraview_protobuf::VariantList* _arguments; };
  inline _InvokeRequest
    operator << (vtkSMMessage& message, const InvokeRequest &)
    {
    _InvokeRequest r;
    r._message = &message;
    return r;
    };

  inline _InvokeRequestArgs
    operator << (const _InvokeRequest& req, const char* method)
      {
      vtkSMMessage* message = req._message;
      message->SetExtension(paraview_protobuf::InvokeRequest::method, method);
      _InvokeRequestArgs args;
      args._arguments =
        message->MutableExtension(paraview_protobuf::InvokeRequest::arguments);
      return args;
      }

  inline _InvokeRequestArgs
    operator << (const _InvokeRequestArgs& req, const int& value)
      {
      paraview_protobuf::Variant * variant = req._arguments->add_variant();
      variant->set_type(paraview_protobuf::Variant::INT);
      variant->add_integer(value);
      return req;
      }

  inline _InvokeRequestArgs
    operator << (const _InvokeRequestArgs& req, const double& value)
      {
      paraview_protobuf::Variant * variant = req._arguments->add_variant();
      variant->set_type(paraview_protobuf::Variant::FLOAT64);
      variant->add_float64(value);
      return req;
      }

  inline _InvokeRequestArgs
    operator << (const _InvokeRequestArgs& req, const char* value)
      {
      paraview_protobuf::Variant * variant = req._arguments->add_variant();
      variant->set_type(paraview_protobuf::Variant::STRING);
      variant->add_txt(value);
      return req;
      }

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
  inline _InvokeRequestArgs
    operator << (const _InvokeRequestArgs& req, const vtkIdType& value)
      {
      paraview_protobuf::Variant * variant = req._arguments->add_variant();
      variant->set_type(paraview_protobuf::Variant::IDTYPE);
      variant->add_idtype(value);
      return req;
      }
#endif

};

inline vtkClientServerStream& operator << (vtkClientServerStream& stream,
  const paraview_protobuf::Variant& variant)
{
  switch (variant.type())
    {
  case paraview_protobuf::Variant::INT:
    for (int cc=0; cc < variant.integer_size(); cc++)
      {
      stream << variant.integer(cc);
      }
    break;

  case paraview_protobuf::Variant::FLOAT64:
    for (int cc=0; cc < variant.float64_size(); cc++)
      {
      stream << variant.float64(cc);
      }
    break;

  case paraview_protobuf::Variant::IDTYPE:
    for (int cc=0; cc < variant.idtype_size(); cc++)
      {
      stream << variant.idtype(cc);
      }
    break;

  case paraview_protobuf::Variant::STRING:
    for (int cc=0; cc < variant.txt_size(); cc++)
      {
      stream << variant.txt(cc).c_str();
      }
    break;

  default:
    break;
    }
  return stream;
}

inline void Test()
{
  vtkSMMessage message;
  message << pvstream::InvokeRequest() << "UpdatePipeline" << 1 << 12.0 <<
    "hello";
}

using namespace paraview_protobuf;

//// Merge other into the current object
//void MergeMessage(Message& out, Message& other)
//{

//}


/*
ProxyState::Property& operator<<(ProxyState::Property& out, double value)
{
  Variant *var = out.mutable_value();
  var->set_type(Variant::FLOAT64);
  var->add_float64(value);
  return out;
}

ProxyState::Property& operator<<(ProxyState::Property& out, int value)
{
  Variant *var = out.mutable_value();
  var->set_type(Variant::INT);
  var->add_integer(value);
  return out;
}
ProxyState::Property& operator<<(ProxyState::Property& out, vtkIdType value)
{
  Variant *var = out.mutable_value();
  var->set_type(Variant::IDTYPE);
  var->add_integer(value);
  return out;
}

ProxyState::Property& operator<<(ProxyState::Property& out, int value)
{
  Variant *var = out.mutable_value();
  var->set_type(Variant::INPUT);
  var->add_integer(value);
  return out;
}

ProxyState::Property& operator<<(ProxyState::Property& out, int value)
{
  Variant *var = out.mutable_value();
  var->set_type(Variant::PROXY);
  var->add_integer(value);
  return out;
}

ProxyState::Property& operator<<(ProxyState::Property& out, int value)
{
  Variant *var = out.mutable_value();
  var->set_type(Variant::STRING);
  var->add_txt(value);
  return out;
}


*/
#endif
