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
  struct InvokeRequestNoWarning { };
  struct _InvokeRequest { vtkSMMessage* _message; };
  struct _InvokeRequestArgs { paraview_protobuf::VariantList* _arguments; };
  inline _InvokeRequest
    operator << (vtkSMMessage& message, const InvokeRequest &)
    {
    _InvokeRequest r;
    r._message = &message;
    return r;
    };

  inline _InvokeRequest
    operator << (vtkSMMessage& message, const InvokeRequestNoWarning &)
    {
    _InvokeRequest r;
    r._message = &message;
    message.SetExtension(paraview_protobuf::InvokeRequest::no_error, true);
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
  // **** Operators to simplify construction of a InvokeResponse message ****/
  struct InvokeResponse { };
  struct _InvokeResponse { vtkSMMessage* _message;};
  struct _InvokeResponseEnd { };
  inline _InvokeResponse
    operator << (vtkSMMessage& message, const InvokeResponse &)
    {
    _InvokeResponse r;
    r._message = &message;

    ::google::protobuf::uint32 location = r._message->location();
    ::google::protobuf::uint64 id = r._message->global_id();
    r._message->Clear();
    r._message->set_global_id(id);
    r._message->set_location(location);

    return r;
    };
  inline _InvokeResponseEnd operator << (const _InvokeResponse& resp,
    const vtkClientServerStream& stream)
  {
    vtkTypeUInt32 arrayLength;
    paraview_protobuf::Variant* variant;
    paraview_protobuf::VariantList* args;
    double* dArray;
    double dValue;
    int iValue;
    int* iArray;
    char* txt;

    switch(stream.GetCommand(0))
      {
      case vtkClientServerStream::Reply:
        args =
            resp._message->MutableExtension(
                paraview_protobuf::InvokeResponse::arguments);
        for(int i=0; i < stream.GetNumberOfArguments(0); i++)
          {
          variant = args->add_variant();
          switch(stream.GetArgumentType(0, i))
            {
            case vtkClientServerStream::float32_value:
            case vtkClientServerStream::float64_value:
              variant->set_type(paraview_protobuf::Variant::FLOAT64);
              dValue = 0;
              stream.GetArgument(0, i, &dValue);
              variant->add_float64(dValue);
              break;

            case vtkClientServerStream::float64_array:
            case vtkClientServerStream::float32_array:
              variant->set_type(paraview_protobuf::Variant::FLOAT64);
              stream.GetArgumentLength(0, i, &arrayLength);
              dArray = new double[arrayLength];
              stream.GetArgument(0, i, dArray, arrayLength);
              for(vtkTypeUInt32 j=0; j < arrayLength; j++)
                {
                variant->add_float64(static_cast<double>(dArray[j]));
                }
              delete[] dArray;
              break;

            case vtkClientServerStream::int16_value:
            case vtkClientServerStream::int8_value:
            case vtkClientServerStream::uint8_value:
            case vtkClientServerStream::uint16_value:
            case vtkClientServerStream::int32_value:
              variant->set_type(paraview_protobuf::Variant::INT);
              iValue = 0;
              stream.GetArgument(0, i, &iValue);
              variant->add_integer(iValue);
              break;

            case vtkClientServerStream::int8_array:
            case vtkClientServerStream::uint8_array:
            case vtkClientServerStream::uint16_array:
            case vtkClientServerStream::int16_array:
            case vtkClientServerStream::stream_value:
            case vtkClientServerStream::int32_array:
              variant->set_type(paraview_protobuf::Variant::INT);
              stream.GetArgumentLength(0, i, &arrayLength);
              iArray = new int[arrayLength];
              stream.GetArgument(0, i, iArray, arrayLength);
              for(vtkTypeUInt32 j=0; j < arrayLength; j++)
                {
                variant->add_integer(static_cast<int>(iArray[j]));
                }
              delete[] iArray;
              break;

            case vtkClientServerStream::string_value:
               variant->set_type(paraview_protobuf::Variant::STRING);
               stream.GetArgumentLength(0, i, &arrayLength);
               txt = new char[arrayLength];
               stream.GetArgument(0, i, txt, arrayLength);
               variant->add_txt(txt, arrayLength);
               delete[] txt;
               break;

            case vtkClientServerStream::id_value:
            case vtkClientServerStream::uint32_value:
            case vtkClientServerStream::uint64_value:
            case vtkClientServerStream::int64_value:
            case vtkClientServerStream::uint32_array:
            case vtkClientServerStream::uint64_array:
            case vtkClientServerStream::int64_array:
               cout << "Type not managed yet !!!" << endl;
               break;
            }
          }
        break;
      case vtkClientServerStream::Error:
        resp._message->SetExtension(paraview_protobuf::InvokeResponse::error, true);
        break;
      }
    _InvokeResponseEnd r;
    return r;
  }
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
