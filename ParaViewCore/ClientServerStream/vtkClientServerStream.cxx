/*=========================================================================

  Program:   ParaView
  Module:    vtkClientServerStream.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientServerStream.h"

#include "vtkAbstractArray.h"
#include "vtkArrayIterator.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkByteSwap.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"
#include "vtkTypeTraits.h"
#include "vtkVariantExtract.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

//----------------------------------------------------------------------------
// Portability of typename keyword.
#if defined(__sgi) && !defined(_COMPILER_VERSION) && !defined(__GNUC__)
#define VTK_CSS_TYPENAME
#elif defined(_MSC_VER) && (_MSC_VER < 1310)
#define VTK_CSS_TYPENAME
#else
#define VTK_CSS_TYPENAME typename
#endif

//----------------------------------------------------------------------------
// Macro to dispatch templated functions based on type in a switch.
#define VTK_CSS_TEMPLATE_MACRO(kind, call)                                                         \
  case int8_##kind:                                                                                \
  {                                                                                                \
    vtkTypeInt8* T = 0;                                                                            \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case int16_##kind:                                                                               \
  {                                                                                                \
    vtkTypeInt16* T = 0;                                                                           \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case int32_##kind:                                                                               \
  {                                                                                                \
    vtkTypeInt32* T = 0;                                                                           \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case int64_##kind:                                                                               \
  {                                                                                                \
    vtkTypeInt64* T = 0;                                                                           \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case uint8_##kind:                                                                               \
  {                                                                                                \
    vtkTypeUInt8* T = 0;                                                                           \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case uint16_##kind:                                                                              \
  {                                                                                                \
    vtkTypeUInt16* T = 0;                                                                          \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case uint32_##kind:                                                                              \
  {                                                                                                \
    vtkTypeUInt32* T = 0;                                                                          \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case uint64_##kind:                                                                              \
  {                                                                                                \
    vtkTypeUInt64* T = 0;                                                                          \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case float32_##kind:                                                                             \
  {                                                                                                \
    vtkTypeFloat32* T = 0;                                                                         \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case float64_##kind:                                                                             \
  {                                                                                                \
    vtkTypeFloat64* T = 0;                                                                         \
    call;                                                                                          \
  }                                                                                                \
  break

//----------------------------------------------------------------------------
// Define some traits for vtkType types.
template <class T>
struct vtkClientServerTypeTraits;
#define VTK_CLIENT_SERVER_TYPE_TRAIT(in, out)                                                      \
  template <>                                                                                      \
  struct vtkClientServerTypeTraits<in>                                                             \
  {                                                                                                \
    /* Type identifier for value of this type. */                                                  \
    static vtkClientServerStream::Types Value() { return vtkClientServerStream::out##_value; }     \
    /* Type identifier for array of this type. */                                                  \
    static vtkClientServerStream::Types Array() { return vtkClientServerStream::out##_array; }     \
  }
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeInt8, int8);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeInt16, int16);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeInt32, int32);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeInt64, int64);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeUInt8, uint8);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeUInt16, uint16);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeUInt32, uint32);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeUInt64, uint64);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeFloat32, float32);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeFloat64, float64);
#undef VTK_CLIENT_SERVER_TYPE_TRAIT

//----------------------------------------------------------------------------
// Internal implementation data.
class vtkClientServerStreamInternals
{
public:
  vtkClientServerStreamInternals(vtkObjectBase* owner)
    : Objects(owner)
  {
  }
  vtkClientServerStreamInternals(const vtkClientServerStreamInternals& r, vtkObjectBase* owner)
    : Data(r.Data)
    , ValueOffsets(r.ValueOffsets)
    , MessageIndexes(r.MessageIndexes)
    , Objects(r.Objects, owner)
    , StartIndex(r.StartIndex)
    , Invalid(r.Invalid)
    , String(r.String)
  {
  }

  // Actual binary data in the stream.
  typedef std::vector<unsigned char> DataType;
  DataType Data;

  // Offset to each value stored in the stream.
  typedef std::vector<DataType::difference_type> ValueOffsetsType;
  ValueOffsetsType ValueOffsets;

  // Index into ValueOffsets of the first value corresponding to each
  // message.
  typedef std::vector<ValueOffsetsType::size_type> MessageIndexesType;
  MessageIndexesType MessageIndexes;

  // Hold references to vtkObjectBase instances stored in the stream.
  // The object that owns this stream is passed as the argument to
  // Register/UnRegister for objects stored in the stream because the
  // owner of this stream effectively owns those objects.  If no owner
  // is given, then the objects are not referenced.
  struct ObjectsType : std::vector<vtkObjectBase*>
  {
    typedef std::vector<vtkObjectBase*> Superclass;
    ObjectsType(vtkObjectBase* owner)
      : Owner(owner)
    {
    }
    ObjectsType(const ObjectsType& r, vtkObjectBase* owner)
      : Superclass(r)
      , Owner(owner)
    {
      if (this->Owner)
      {
        for (Superclass::iterator i = this->begin(); i != this->end(); ++i)
        {
          (*i)->Register(this->Owner);
        }
      }
    }
    ~ObjectsType() { this->Clear(); }
    ObjectsType& operator=(const ObjectsType& that)
    {
      Superclass::operator=(that);
      if (this->Owner)
      {
        for (Superclass::iterator i = this->begin(); i != this->end(); ++i)
        {
          (*i)->Register(this->Owner);
        }
      }
      return *this;
    }
    vtkObjectBase* Owner;

    void Insert(vtkObjectBase* obj)
    {
      if (obj)
      {
        if (this->Owner)
        {
          obj->Register(this->Owner);
        }
        this->push_back(obj);
      }
    }

    void Clear()
    {
      for (Superclass::iterator i = this->begin(); i != this->end(); ++i)
      {
        if (this->Owner)
        {
          (*i)->UnRegister(this->Owner);
        }
      }
      this->erase(this->begin(), this->end());
    }
  };
  ObjectsType Objects;

  // Index into ValueOffsets where the last Command started.  Used to
  // detect valid message completion.
  static const ValueOffsetsType::size_type InvalidStartIndex;
  ValueOffsetsType::size_type StartIndex;

  // Whether the stream has been constructed improperly.
  int Invalid;

  // Buffer for return value from StreamToString.
  std::string String;

  // Access to protected members of vtkClientServerStream.
  static vtkClientServerStream& Write(vtkClientServerStream& css, const void* data, size_t length)
  {
    return css.Write(data, length);
  }
  static const unsigned char* GetValue(const vtkClientServerStream& css, int message, int value)
  {
    return css.GetValue(message, value);
  }
};

const vtkClientServerStreamInternals::ValueOffsetsType::size_type
  vtkClientServerStreamInternals::InvalidStartIndex =
    static_cast<vtkClientServerStreamInternals::ValueOffsetsType::size_type>(-1);

//----------------------------------------------------------------------------
vtkClientServerStream::vtkClientServerStream(vtkObjectBase* owner)
{
  // Initialize the internal representation of the stream.
  this->Internal = new vtkClientServerStreamInternals(owner);
  this->Reserve(1024);
  this->Reset();
}

//----------------------------------------------------------------------------
vtkClientServerStream::~vtkClientServerStream()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
vtkClientServerStream::vtkClientServerStream(const vtkClientServerStream& r, vtkObjectBase* owner)
{
  // Allocate and copy the internal representation of the stream.
  this->Internal = new vtkClientServerStreamInternals(*r.Internal, owner);
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator=(const vtkClientServerStream& that)
{
  *this->Internal = *that.Internal;
  return *this;
}

//----------------------------------------------------------------------------
void vtkClientServerStream::Copy(const vtkClientServerStream* source)
{
  *this = *source;
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::Write(const void* data, size_t length)
{
  // Make sure we have data.
  if (length == 0)
  {
    return *this;
  }
  else if (!data)
  {
    vtkGenericWarningMacro("vtkClientServerStream::Write given NULL pointer and non-zero length.");
    return *this;
  }

  // Copy the value into the data.
  this->Internal->Data.resize(this->Internal->Data.size() + length);
  memcpy(&*(this->Internal->Data.end() - length), data, length);
  return *this;
}

//----------------------------------------------------------------------------
void vtkClientServerStream::Reserve(size_t size)
{
  this->Internal->Data.reserve(size);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::Reset()
{
  // Empty the entire stream.
  vtkClientServerStreamInternals::DataType().swap(this->Internal->Data);

  this->Internal->ValueOffsets.erase(
    this->Internal->ValueOffsets.begin(), this->Internal->ValueOffsets.end());
  this->Internal->MessageIndexes.erase(
    this->Internal->MessageIndexes.begin(), this->Internal->MessageIndexes.end());
  this->Internal->Objects.Clear();

  // No message has yet been started.
  this->Internal->Invalid = 0;
  this->Internal->StartIndex = vtkClientServerStreamInternals::InvalidStartIndex;

// Store the byte order of data to come.
#ifdef VTK_WORDS_BIGENDIAN
  this->Internal->Data.push_back(vtkClientServerStream::BigEndian);
#else
  this->Internal->Data.push_back(vtkClientServerStream::LittleEndian);
#endif
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(vtkClientServerStream::Commands t)
{
  // Make sure we do not start another command without ending the
  // previous one.
  if (this->Internal->StartIndex != vtkClientServerStreamInternals::InvalidStartIndex)
  {
    // Got two Commands without an End between them.  This is an
    // invalid stream.
    this->Internal->Invalid = 1;
    return *this;
  }

  // Save where this message starts.
  this->Internal->StartIndex = this->Internal->ValueOffsets.size();

  // The command counts as the first value in the message.
  this->Internal->ValueOffsets.push_back(this->Internal->Data.end() - this->Internal->Data.begin());

  // Store the command in the stream.
  vtkTypeUInt32 data = static_cast<vtkTypeUInt32>(t);
  return this->Write(&data, sizeof(data));
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(vtkClientServerStream::Types t)
{
  // If this is the end of a message, record the message start
  // position.
  if (t == vtkClientServerStream::End)
  {
    // Make sure the command we are ending actually started.
    if (this->Internal->StartIndex == vtkClientServerStreamInternals::InvalidStartIndex)
    {
      // This is an invalid stream.
      this->Internal->Invalid = 1;
      return *this;
    }

    // Store the value index where this command started.
    this->Internal->MessageIndexes.push_back(this->Internal->StartIndex);

    // No current Command is being constructed.
    this->Internal->StartIndex = vtkClientServerStreamInternals::InvalidStartIndex;
  }

  // All values write their type first.  Mark the start of this type
  // and optional value.
  this->Internal->ValueOffsets.push_back(this->Internal->Data.end() - this->Internal->Data.begin());

  // Store the type in the stream.
  vtkTypeUInt32 data = static_cast<vtkTypeUInt32>(t);
  return this->Write(&data, sizeof(data));
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(vtkClientServerStream::Argument a)
{
  if (a.Data && a.Size)
  {
    // Mark the start of this type and optional value.
    this->Internal->ValueOffsets.push_back(
      this->Internal->Data.end() - this->Internal->Data.begin());

    // If the argument is a vtk_object_pointer, we need to store a
    // reference to the object.
    vtkTypeUInt32 tp;
    memcpy(&tp, a.Data, sizeof(tp));
    if (tp == vtkClientServerStream::vtk_object_pointer)
    {
      vtkObjectBase* obj;
      memcpy(&obj, a.Data + sizeof(tp), sizeof(obj));
      this->Internal->Objects.Insert(obj);
    }

    // Write the data to the stream.
    return this->Write(a.Data, a.Size);
  }
  return *this;
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(vtkClientServerStream::Array a)
{
  // Store the array type, then length, then data.
  *this << a.Type;
  this->Write(&a.Length, sizeof(a.Length));
  this->Write(a.Data, a.Size);

  // Special case for InsertString.  We need to add the null terminator.
  if (a.Type == vtkClientServerStream::string_value)
  {
    char n = 0;
    this->Write(&n, 1);
  }
  return *this;
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(const vtkClientServerStream& css)
{
  const unsigned char* data;
  size_t length;
  // Do not allow object pointers to be passed in binary form.
  if (this != &css && css.Internal->Objects.empty() && css.GetData(&data, &length))
  {
    // Store the stream_value type, then length, then data.
    *this << vtkClientServerStream::stream_value;
    vtkTypeUInt32 size = static_cast<vtkTypeUInt32>(length);
    this->Write(&size, sizeof(size));
    return this->Write(data, size);
  }
  else
  {
    // This is an invalid stream.
    this->Internal->Invalid = 1;
    return *this;
  }
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(vtkClientServerID id)
{
  // Store the id_value type and then the id value itself.
  *this << vtkClientServerStream::id_value;
  return this->Write(&id.ID, sizeof(id.ID));
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(vtkObjectBase* obj)
{
  // The stream will now hold a reference to the object.
  this->Internal->Objects.Insert(obj);

  // Store the vtk_object_pointer type and then the pointer value itself.
  *this << vtkClientServerStream::vtk_object_pointer;
  return this->Write(&obj, sizeof(obj));
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(const vtkStdString& val)
{
  (*this) << val.c_str();
  return *this;
}

//----------------------------------------------------------------------------
template <typename iterT>
void vtkClientServerPutArrayVariant(vtkClientServerStream& css, iterT* it)
{
  vtkIdType numVals = it->GetNumberOfValues();
  for (vtkIdType i = 0; i < numVals; ++i)
  {
    css << it->GetValue(i);
  }
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(const vtkVariant& val)
{
  vtkTypeUInt8 variantValid = val.IsValid();
  if (variantValid && val.IsVTKObject() && !val.IsArray())
  { // If val is a VTK object that is not an array, fail by encoding an invalid vtkVariant value.
    variantValid = 0;
  }
  (*this) << variantValid;

  if (variantValid)
  {
    vtkTypeUInt8 variantType = static_cast<vtkTypeUInt8>(val.GetType());
    (*this) << variantType;

    bool validDummy;
    switch (variantType)
    {
      vtkExtendedTemplateMacro((*this) << vtkVariantExtract<VTK_TT>(val, validDummy));
      case VTK_OBJECT:
      { // The object must be an array; encode it.
        vtkAbstractArray* array = val.ToArray();
        vtkTypeInt32 arrayType = array->GetDataType();
        vtkTypeInt32 numComponents = array->GetNumberOfComponents();
        vtkTypeInt64 numTuples = array->GetNumberOfTuples();
        (*this) << arrayType << numComponents << numTuples;
        vtkArrayIterator* iter = array->NewIterator();
        switch (array->GetDataType())
        {
          vtkExtendedArrayIteratorTemplateMacro(
            vtkClientServerPutArrayVariant<VTK_TT>(*this, static_cast<VTK_TT*>(iter)));
        }
        iter->Delete();
      }
      break;
    }
  }
  return *this;
}

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(bool x)
{
  // Store the type first, then the value.
  // Boolean values in the stream are represented by vtkTypeUInt8.
  *this << vtkClientServerStream::bool_value;
  vtkTypeUInt8 b = x ? 1 : 0;
  return this->Write(&b, sizeof(b));
}

//----------------------------------------------------------------------------
// Template and macro to implement all insertion operators in the same way.
template <class T>
vtkClientServerStream& vtkClientServerStreamOperatorSL(vtkClientServerStream* self, T x)
{
  // Store the type first, then the value.
  typedef VTK_CSS_TYPENAME vtkTypeTraits<T>::SizedType Type;
  *self << vtkClientServerTypeTraits<Type>::Value();
  return vtkClientServerStreamInternals::Write(*self, &x, sizeof(x));
}

#define VTK_CLIENT_SERVER_OPERATOR(type)                                                           \
  vtkClientServerStream& vtkClientServerStream::operator<<(type x)                                 \
  {                                                                                                \
    return vtkClientServerStreamOperatorSL(this, x);                                               \
  }
VTK_CLIENT_SERVER_OPERATOR(char)
VTK_CLIENT_SERVER_OPERATOR(int)
VTK_CLIENT_SERVER_OPERATOR(short)
VTK_CLIENT_SERVER_OPERATOR(long)
VTK_CLIENT_SERVER_OPERATOR(signed char)
VTK_CLIENT_SERVER_OPERATOR(unsigned char)
VTK_CLIENT_SERVER_OPERATOR(unsigned int)
VTK_CLIENT_SERVER_OPERATOR(unsigned short)
VTK_CLIENT_SERVER_OPERATOR(unsigned long)
#if defined(VTK_TYPE_USE_LONG_LONG)
VTK_CLIENT_SERVER_OPERATOR(long long)
VTK_CLIENT_SERVER_OPERATOR(unsigned long long)
#endif
#if defined(VTK_TYPE_USE___INT64)
VTK_CLIENT_SERVER_OPERATOR(__int64)
VTK_CLIENT_SERVER_OPERATOR(unsigned __int64)
#endif
VTK_CLIENT_SERVER_OPERATOR(float)
VTK_CLIENT_SERVER_OPERATOR(double)
#undef VTK_CLIENT_SERVER_OPERATOR

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator<<(const char* x)
{
  // String length will include null terminator.  NULL string will be
  // length 0.
  vtkTypeUInt32 length = static_cast<vtkTypeUInt32>(x ? (strlen(x) + 1) : 0);
  *this << vtkClientServerStream::string_value;
  this->Write(&length, sizeof(length));
  return this->Write(x, static_cast<size_t>(length));
}

//----------------------------------------------------------------------------
vtkClientServerStream::Array vtkClientServerStream::InsertString(const char* begin, const char* end)
{
  // Make sure the string does not end early.
  const char* c = begin;
  while (c < end && *c)
  {
    ++c;
  }
  end = c;

  // Construct a fake array with a string type.  This will be handled
  // by the insertion operator with a special case for InsertString to
  // add the null terminator.
  vtkClientServerStream::Array a = { vtkClientServerStream::string_value,
    static_cast<vtkTypeUInt32>(end - begin + 1), static_cast<vtkTypeUInt32>(end - begin), begin };
  return a;
}

//----------------------------------------------------------------------------
// Template and macro to implement all InsertArray methods in the same way.
template <class T>
vtkClientServerStream::Array vtkClientServerStreamInsertArray(const T* data, int length)
{
  // Construct and return the array information structure.
  typedef VTK_CSS_TYPENAME vtkTypeTraits<T>::SizedType Type;
  vtkClientServerStream::Array a = { vtkClientServerTypeTraits<Type>::Array(),
    static_cast<vtkTypeUInt32>(length), static_cast<vtkTypeUInt32>(sizeof(Type) * length), data };
  return a;
}

#define VTK_CLIENT_SERVER_INSERT_ARRAY(type)                                                       \
  vtkClientServerStream::Array vtkClientServerStream::InsertArray(const type* data, int length)    \
  {                                                                                                \
    return vtkClientServerStreamInsertArray(data, length);                                         \
  }
VTK_CLIENT_SERVER_INSERT_ARRAY(char)
VTK_CLIENT_SERVER_INSERT_ARRAY(short)
VTK_CLIENT_SERVER_INSERT_ARRAY(int)
VTK_CLIENT_SERVER_INSERT_ARRAY(long)
VTK_CLIENT_SERVER_INSERT_ARRAY(signed char)
VTK_CLIENT_SERVER_INSERT_ARRAY(unsigned char)
VTK_CLIENT_SERVER_INSERT_ARRAY(unsigned short)
VTK_CLIENT_SERVER_INSERT_ARRAY(unsigned int)
VTK_CLIENT_SERVER_INSERT_ARRAY(unsigned long)
#if defined(VTK_TYPE_USE_LONG_LONG)
VTK_CLIENT_SERVER_INSERT_ARRAY(long long)
VTK_CLIENT_SERVER_INSERT_ARRAY(unsigned long long)
#endif
#if defined(VTK_TYPE_USE___INT64)
VTK_CLIENT_SERVER_INSERT_ARRAY(__int64)
VTK_CLIENT_SERVER_INSERT_ARRAY(unsigned __int64)
#endif
VTK_CLIENT_SERVER_INSERT_ARRAY(float)
VTK_CLIENT_SERVER_INSERT_ARRAY(double)
#undef VTK_CLIENT_SERVER_INSERT_ARRAY

//----------------------------------------------------------------------------
// Template to implement each type conversion in the lookup tables below.
// The "long, long, long" arguments are used to convince VS6 to select
// the proper overload when more than one of these functions otherwise
// looks the same after template instantiation.  This works around the
// lack of partial ordering of templates in VS6.
template <class SourceType, class T>
void vtkClientServerStreamGetArgumentCase(
  SourceType*, const unsigned char* src, T* dest, long, long, long)
{
  // Copy the value out of the stream and convert it.
  SourceType value;
  memcpy(&value, src, sizeof(value));
  *dest = static_cast<T>(value);
}
template <class SourceType>
void vtkClientServerStreamGetArgumentCase(
  SourceType*, const unsigned char* src, bool* dest, long, long, int)
{
  // Copy the value out of the stream and convert it.
  SourceType value;
  memcpy(&value, src, sizeof(value));
  *dest = value ? true : false;
}
template <class T>
void vtkClientServerStreamGetArgumentCase(bool*, const unsigned char* src, T* dest, long, int, int)
{
  // Boolean values in the stream are represented by vtkTypeUInt8.
  vtkTypeUInt8 value;
  memcpy(&value, src, sizeof(value));
  *dest = static_cast<T>(value);
}
void vtkClientServerStreamGetArgumentCase(
  bool*, const unsigned char* src, bool* dest, int, int, int)
{
  // Boolean values in the stream are represented by vtkTypeUInt8.
  vtkTypeUInt8 value;
  memcpy(&value, src, sizeof(value));
  *dest = value ? true : false;
}

#define VTK_CSS_GET_ARGUMENT_CASE(TypeId, SourceType)                                              \
  case vtkClientServerStream::TypeId:                                                              \
  {                                                                                                \
    SourceType* T = 0;                                                                             \
    vtkClientServerStreamGetArgumentCase(T, src, dest, 1, 1, 1);                                   \
  }                                                                                                \
  break

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, vtkTypeInt8* dest)
{
  // Lookup what types can be converted safely to a vtkTypeInt8.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(bool_value, bool);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    default:
      return 0;
  };
  return 1;
}

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, vtkTypeInt16* dest)
{
  // Lookup what types can be converted safely to a vtkTypeInt16.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(bool_value, bool);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    default:
      return 0;
  };
  return 1;
}

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, vtkTypeInt32* dest)
{
  // Lookup what types can be converted safely to a vtkTypeInt32.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(bool_value, bool);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(int64_value, vtkTypeInt64);
    VTK_CSS_GET_ARGUMENT_CASE(uint64_value, vtkTypeUInt64);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    default:
      return 0;
  };
  return 1;
}

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, vtkTypeInt64* dest)
{
  // Lookup what types can be converted safely to a vtkTypeInt64.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(int64_value, vtkTypeInt64);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(bool_value, bool);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint64_value, vtkTypeUInt64);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    VTK_CSS_GET_ARGUMENT_CASE(float64_value, vtkTypeFloat64);
    default:
      return 0;
  };
  return 1;
}

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, vtkTypeUInt8* dest)
{
  // Lookup what types can be converted safely to a vtkTypeUInt8.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(bool_value, bool);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    default:
      return 0;
  };
  return 1;
}

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, vtkTypeUInt16* dest)
{
  // Lookup what types can be converted safely to a vtkTypeUInt16.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(bool_value, bool);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    default:
      return 0;
  };
  return 1;
}

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, vtkTypeUInt32* dest)
{
  // Lookup what types can be converted safely to a vtkTypeUInt32.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(bool_value, bool);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(int64_value, vtkTypeInt64);
    VTK_CSS_GET_ARGUMENT_CASE(uint64_value, vtkTypeUInt64);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    default:
      return 0;
  };
  return 1;
}

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, vtkTypeUInt64* dest)
{
  // Lookup what types can be converted safely to a vtkTypeUInt64.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint64_value, vtkTypeUInt64);
    VTK_CSS_GET_ARGUMENT_CASE(bool_value, bool);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(int64_value, vtkTypeInt64);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    VTK_CSS_GET_ARGUMENT_CASE(float64_value, vtkTypeFloat64);
    default:
      return 0;
  };
  return 1;
}

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, vtkTypeFloat32* dest)
{
  // Lookup what types can be converted safely to a vtkTypeFloat32.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(float64_value, vtkTypeFloat64);
    VTK_CSS_GET_ARGUMENT_CASE(int64_value, vtkTypeInt64);
    VTK_CSS_GET_ARGUMENT_CASE(uint64_value, vtkTypeUInt64);
    default:
      return 0;
  };
  return 1;
}

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, vtkTypeFloat64* dest)
{
  // Lookup what types can be converted safely to a vtkTypeFloat64.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(int64_value, vtkTypeInt64);
    VTK_CSS_GET_ARGUMENT_CASE(uint64_value, vtkTypeUInt64);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    VTK_CSS_GET_ARGUMENT_CASE(float64_value, vtkTypeFloat64);

    default:
      return 0;
  };
  return 1;
}

int vtkClientServerStreamGetArgument(
  vtkClientServerStream::Types type, const unsigned char* src, bool* dest)
{
  // Lookup what types can be converted safely to a bool.
  switch (type)
  {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(bool_value, bool);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint64_value, vtkTypeUInt64);
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(int64_value, vtkTypeInt64);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    VTK_CSS_GET_ARGUMENT_CASE(float64_value, vtkTypeFloat64);
    default:
      return 0;
  };
  return 1;
}

#undef VTK_CSS_GET_ARGUMENT_CASE

//----------------------------------------------------------------------------
// Template and macro to implement value GetArgument methods in the same way.
template <class T>
int vtkClientServerStreamGetArgumentValue(
  const vtkClientServerStream* self, int midx, int argument, T* value, long)
{
  typedef VTK_CSS_TYPENAME vtkTypeTraits<T>::SizedType Type;
  if (const unsigned char* data =
        vtkClientServerStreamInternals::GetValue(*self, midx, 1 + argument))
  {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Call the type conversion function for this type.
    return vtkClientServerStreamGetArgument(
      static_cast<vtkClientServerStream::Types>(tp), data, reinterpret_cast<Type*>(value));
  }
  return 0;
}
int vtkClientServerStreamGetArgumentValue(
  const vtkClientServerStream* self, int midx, int argument, bool* value, int)
{
  if (const unsigned char* data =
        vtkClientServerStreamInternals::GetValue(*self, midx, 1 + argument))
  {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Call the type conversion function for this type.
    return vtkClientServerStreamGetArgument(
      static_cast<vtkClientServerStream::Types>(tp), data, value);
  }
  return 0;
}

#define VTK_CSS_GET_ARGUMENT(type)                                                                 \
  int vtkClientServerStream::GetArgument(int message, int argument, type* value) const             \
  {                                                                                                \
    return vtkClientServerStreamGetArgumentValue(this, message, argument, value, 1);               \
  }
VTK_CSS_GET_ARGUMENT(bool)
VTK_CSS_GET_ARGUMENT(signed char)
VTK_CSS_GET_ARGUMENT(char)
VTK_CSS_GET_ARGUMENT(int)
VTK_CSS_GET_ARGUMENT(short)
VTK_CSS_GET_ARGUMENT(long)
VTK_CSS_GET_ARGUMENT(unsigned char)
VTK_CSS_GET_ARGUMENT(unsigned int)
VTK_CSS_GET_ARGUMENT(unsigned short)
VTK_CSS_GET_ARGUMENT(unsigned long)
VTK_CSS_GET_ARGUMENT(float)
VTK_CSS_GET_ARGUMENT(double)
#if defined(VTK_TYPE_USE_LONG_LONG)
VTK_CSS_GET_ARGUMENT(long long)
VTK_CSS_GET_ARGUMENT(unsigned long long)
#endif
#if defined(VTK_TYPE_USE___INT64)
VTK_CSS_GET_ARGUMENT(__int64)
VTK_CSS_GET_ARGUMENT(unsigned __int64)
#endif
#undef VTK_CSS_GET_ARGUMENT

template <typename SourceType, typename DestType>
int vtkClientServerStreamGetArgumentArrayCase(
  const unsigned char* src, DestType* dest, vtkTypeUInt32 length)
{
  // Get the length of the value in the stream.
  vtkTypeUInt32 len;
  memcpy(&len, src, sizeof(len));
  src += sizeof(len);
  if (len == length)
  {
    // Copy the value out of the stream.
    std::transform(reinterpret_cast<const SourceType*>(src),
      reinterpret_cast<const SourceType*>(src) + len, dest,
      [](const SourceType& val) { return static_cast<DestType>(val); });
    return 1;
  }

  return 0;
}

#define VTK_CSS_GET_ARGUMENT_ARRAY_CASE(TypeId, SourceType)                                        \
  case vtkClientServerStream::TypeId:                                                              \
  {                                                                                                \
    return vtkClientServerStreamGetArgumentArrayCase<SourceType, T>(data, value, length);          \
  }

//----------------------------------------------------------------------------
// Template and macro to implement array GetArgument methods in the same way.
template <class T>
int vtkClientServerStreamGetArgumentArray(
  const vtkClientServerStream* self, int midx, int argument, T* value, vtkTypeUInt32 length)
{
  typedef VTK_CSS_TYPENAME vtkTypeTraits<T>::SizedType Type;
  if (const unsigned char* data =
        vtkClientServerStreamInternals::GetValue(*self, midx, 1 + argument))
  {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // If the type and length of the array match, use it.
    const auto array_type = vtkClientServerTypeTraits<Type>::Array();
    if (static_cast<vtkClientServerStream::Types>(tp) == array_type)
    {
      // Get the length of the value in the stream.
      vtkTypeUInt32 len;
      memcpy(&len, data, sizeof(len));
      data += sizeof(len);

      if (len == length)
      {
        // Copy the value out of the stream.
        memcpy(value, data, len * sizeof(Type));
        return 1;
      }
    }
    else if (array_type == vtkClientServerStream::int8_array ||
      array_type == vtkClientServerStream::int16_array ||
      array_type == vtkClientServerStream::int32_array ||
      array_type == vtkClientServerStream::int64_array ||
      array_type == vtkClientServerStream::uint8_array ||
      array_type == vtkClientServerStream::uint16_array ||
      array_type == vtkClientServerStream::uint32_array ||
      array_type == vtkClientServerStream::uint64_array ||
      array_type == vtkClientServerStream::float32_array ||
      array_type == vtkClientServerStream::float64_array)
    {
      // if the dest array is a numeric type, we will transform the array values
      // in the stream that are of supported numeric types to the dest. note, it
      // simply uses static_cast and hence may cause precision loss, overflow
      // etc.
      switch (static_cast<vtkClientServerStream::Types>(tp))
      {
        VTK_CSS_GET_ARGUMENT_ARRAY_CASE(int8_array, vtkTypeInt8);
        VTK_CSS_GET_ARGUMENT_ARRAY_CASE(int16_array, vtkTypeInt16);
        VTK_CSS_GET_ARGUMENT_ARRAY_CASE(int32_array, vtkTypeInt32);
        VTK_CSS_GET_ARGUMENT_ARRAY_CASE(int64_array, vtkTypeInt64);
        VTK_CSS_GET_ARGUMENT_ARRAY_CASE(uint8_array, vtkTypeUInt8);
        VTK_CSS_GET_ARGUMENT_ARRAY_CASE(uint16_array, vtkTypeUInt16);
        VTK_CSS_GET_ARGUMENT_ARRAY_CASE(uint32_array, vtkTypeUInt32);
        VTK_CSS_GET_ARGUMENT_ARRAY_CASE(uint64_array, vtkTypeUInt64);
        VTK_CSS_GET_ARGUMENT_ARRAY_CASE(float32_array, vtkTypeFloat32);
        VTK_CSS_GET_ARGUMENT_ARRAY_CASE(float64_array, vtkTypeFloat64);
        default:
          break;
      }
    }
  }
  return 0;
}

#define VTK_CSS_GET_ARGUMENT_ARRAY(type)                                                           \
  int vtkClientServerStream::GetArgument(                                                          \
    int message, int argument, type* value, vtkTypeUInt32 length) const                            \
  {                                                                                                \
    return vtkClientServerStreamGetArgumentArray(this, message, argument, value, length);          \
  }
VTK_CSS_GET_ARGUMENT_ARRAY(signed char)
VTK_CSS_GET_ARGUMENT_ARRAY(char)
VTK_CSS_GET_ARGUMENT_ARRAY(int)
VTK_CSS_GET_ARGUMENT_ARRAY(short)
VTK_CSS_GET_ARGUMENT_ARRAY(long)
VTK_CSS_GET_ARGUMENT_ARRAY(unsigned char)
VTK_CSS_GET_ARGUMENT_ARRAY(unsigned int)
VTK_CSS_GET_ARGUMENT_ARRAY(unsigned short)
VTK_CSS_GET_ARGUMENT_ARRAY(unsigned long)
VTK_CSS_GET_ARGUMENT_ARRAY(float)
VTK_CSS_GET_ARGUMENT_ARRAY(double)
#if defined(VTK_TYPE_USE_LONG_LONG)
VTK_CSS_GET_ARGUMENT_ARRAY(long long)
VTK_CSS_GET_ARGUMENT_ARRAY(unsigned long long)
#endif
#if defined(VTK_TYPE_USE___INT64)
VTK_CSS_GET_ARGUMENT_ARRAY(__int64)
VTK_CSS_GET_ARGUMENT_ARRAY(unsigned __int64)
#endif
#undef VTK_CSS_GET_ARGUMENT_ARRAY

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgument(int message, int argument, const char** value) const
{
  // Get a pointer to the type/value pair in the stream.
  if (const unsigned char* data = this->GetValue(message, 1 + argument))
  {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Make sure the type is a string.
    if (static_cast<vtkClientServerStream::Types>(tp) == vtkClientServerStream::string_value)
    {
      // Get the length of the string.
      vtkTypeUInt32 len;
      memcpy(&len, data, sizeof(len));
      data += sizeof(len);

      if (len > 0)
      {
        // Give back a pointer directly to the string in the stream.
        *value = reinterpret_cast<const char*>(data);
      }
      else
      {
        // String pointer value was NULL.
        *value = 0;
      }
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgument(int message, int argument, char** value) const
{
  const char* tmp;
  if (this->GetArgument(message, argument, &tmp))
  {
    *value = const_cast<char*>(tmp);
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgument(int message, int argument, vtkStdString* value) const
{
  return this->GetArgument(message, argument, static_cast<std::string*>(value));
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgument(int message, int argument, std::string* value) const
{
  char* tmp = NULL;
  if (this->GetArgument(message, argument, &tmp) && tmp)
  {
    *value = tmp;
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgument(
  int message, int argument, vtkClientServerStream* value) const
{
  // Get a pointer to the type/value pair in the stream.
  if (const unsigned char* data = this->GetValue(message, 1 + argument))
  {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Make sure the type is a stream_value.
    if (static_cast<vtkClientServerStream::Types>(tp) == vtkClientServerStream::stream_value)
    {
      // Get the length of the stream data.
      vtkTypeUInt32 len;
      memcpy(&len, data, sizeof(len));
      data += sizeof(len);

      // Set the data in the given stream.
      size_t length = static_cast<size_t>(len);
      return value->SetData(data, length);
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgument(int message, int argument, vtkClientServerID* value) const
{
  // Get a pointer to the type/value pair in the stream.
  if (const unsigned char* data = this->GetValue(message, 1 + argument))
  {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Make sure the type is an id_value.
    if (static_cast<vtkClientServerStream::Types>(tp) == vtkClientServerStream::id_value)
    {
      // Copy the value out of the stream.
      memcpy(&value->ID, data, sizeof(value->ID));
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
// Template to implement GetArgument zero-to-null-pointer conversions.
template <class SourceType>
int vtkClientServerStreamGetArgumentPointer(
  SourceType*, const unsigned char* src, vtkObjectBase** dest)
{
  // Copy the value out of the stream.
  SourceType value;
  memcpy(&value, src, sizeof(value));

  // Values of 0 can be converted to a NULL pointer.
  if (value == static_cast<SourceType>(0))
  {
    *dest = 0;
    return 1;
  }
  return 0;
}

int vtkClientServerStream::GetArgument(int message, int argument, vtkObjectBase** value) const
{
  // Get a pointer to the type/value pair in the stream.
  int result = 0;
  if (const unsigned char* data = this->GetValue(message, 1 + argument))
  {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Make sure the type is a vtk_object_pointer or is 0.
    switch (static_cast<vtkClientServerStream::Types>(tp))
    {
      VTK_CSS_TEMPLATE_MACRO(
        value, result = vtkClientServerStreamGetArgumentPointer(T, data, value));
      case vtkClientServerStream::vtk_object_pointer:
      {
        // Copy the value out of the stream.
        memcpy(value, data, sizeof(*value));
        result = 1;
      }
      break;
      case vtkClientServerStream::id_value:
      {
        // Copy the value out of the stream.
        vtkClientServerID id;
        memcpy(&id.ID, data, sizeof(id.ID));
        // ID value 0 can be converted to a NULL pointer.
        if (id.ID == 0)
        {
          *value = 0;
          return 1;
        }
        return 0;
      }
      break;
      default:
        break;
    }
  }
  return result;
}

template <typename T>
int vtkClientServerGetVariant(
  const vtkClientServerStream* self, int message, int& argument, vtkVariant& out)
{
  T raw;
  int result = self->GetArgument(message, argument++, &raw);
  if (!result)
    return result;
  out = raw;
  return result; // result != 0 by definition
}

// Specialize for vtkVariant as \a argument is handled differently.
template <>
int vtkClientServerGetVariant<vtkVariant>(
  const vtkClientServerStream* self, int message, int& argument, vtkVariant& out)
{
  int result = self->GetArgument(message, argument, &out);
  if (!result)
    return result;
  return result; // result != 0 by definition
}

template <typename T>
int vtkClientServerGetArrayVariant(
  const vtkClientServerStream* self, int message, int& argument, vtkAbstractArray* array)
{
  vtkVariant value;
  vtkIdType maxId = array->GetMaxId();
  int result = 1;
  for (vtkIdType i = 0; i <= maxId; ++i)
  {
    result = vtkClientServerGetVariant<T>(self, message, argument, value);
    if (!result)
      return result;
    array->SetVariantValue(i, value);
  }
  return result;
}

int vtkClientServerStream::GetArgument(int message, int& argument, vtkVariant* value) const
{
  int result;

  vtkTypeUInt8 variantValid;
  result = this->GetArgument(message, argument++, &variantValid);
  if (!result)
    return result;

  if (variantValid)
  {
    vtkTypeUInt8 variantType;
    result = this->GetArgument(message, argument++, &variantType);
    if (!result)
      return result;

    switch (variantType)
    {
      vtkExtendedTemplateMacro(vtkClientServerGetVariant<VTK_TT>(this, message, argument, *value));
      case VTK_OBJECT:
      {
        // Only vtkAbstractArray subclasses are supported, and only their raw values are encoded (no
        // vtkInformation keys)
        // Packed values include: the array type, the # components(nc), # tuples(nt), nc*nt raw
        // values.
        vtkTypeInt32 arrayType;
        vtkTypeInt32 numComponents;
        vtkTypeInt64 numTuples;
        result = this->GetArgument(message, argument++, &arrayType);
        if (!result)
          return result;
        result = this->GetArgument(message, argument++, &numComponents);
        if (!result)
          return result;
        result = this->GetArgument(message, argument++, &numTuples);
        if (!result)
          return result;
        vtkSmartPointer<vtkAbstractArray> array;
        array.TakeReference(vtkAbstractArray::CreateArray(arrayType));
        if (!array)
          return 1;
        array->SetNumberOfComponents(numComponents);
        array->SetNumberOfTuples(numTuples);
        switch (arrayType)
        {
          vtkExtraExtendedTemplateMacro(
            vtkClientServerGetArrayVariant<VTK_TT>(this, message, argument, array));
        }
        *value = array.GetPointer();
      }
      break;
    }
  }
  else
  {
    *value = vtkVariant();
  }
  return result;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgumentLength(int message, int argument, vtkTypeUInt32* length) const
{
  // Get a pointer to the type/value pair in the stream.
  if (const unsigned char* data = this->GetValue(message, 1 + argument))
  {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Make sure the type is an array type.
    switch (static_cast<vtkClientServerStream::Types>(tp))
    {
      case vtkClientServerStream::int8_array:
      case vtkClientServerStream::uint8_array:
      case vtkClientServerStream::int16_array:
      case vtkClientServerStream::uint16_array:
      case vtkClientServerStream::int32_array:
      case vtkClientServerStream::uint32_array:
      case vtkClientServerStream::float32_array:
      case vtkClientServerStream::int64_array:
      case vtkClientServerStream::uint64_array:
      case vtkClientServerStream::float64_array:
      {
        // Get the length of the array.
        memcpy(length, data, sizeof(*length));
      }
        return 1;
      default:
        break;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgumentObject(
  int message, int argument, vtkObjectBase** value, const char* type) const
{
  // Get the argument object and check its type.
  vtkObjectBase* obj;
  if (this->GetArgument(message, argument, &obj))
  {
    // if the pointer is valid then check the type
    if (obj && !obj->IsA(type))
    {
      return 0;
    }
    *value = obj;
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetData(const unsigned char** data, size_t* length) const
{
  // Do not return data unless stream is valid.
  if (!this->Internal->Invalid)
  {
    if (data)
    {
      *data = &*this->Internal->Data.begin();
    }

    if (length)
    {
      *length = this->Internal->Data.size();
    }
    return 1;
  }
  else
  {
    if (data)
    {
      *data = 0;
    }

    if (length)
    {
      *length = 0;
    }
    return 0;
  }
}

//----------------------------------------------------------------------------
int vtkClientServerStream::SetData(const unsigned char* data, size_t length)
{
  // Reset and remove the byte order entry from the stream.
  this->Reset();
  this->Internal->Data.erase(this->Internal->Data.begin(), this->Internal->Data.end());

  // Store the given data in the stream.
  if (data)
  {
    this->Internal->Data.insert(this->Internal->Data.begin(), data, data + length);
  }

  // Parse the stream to fill in ValueOffsets and MessageIndexes and
  // to perform byte-swapping if necessary.
  if (this->ParseData())
  {
// Data have been byte-swapped to the native representation.
#ifdef VTK_WORDS_BIGENDIAN
    this->Internal->Data[0] = vtkClientServerStream::BigEndian;
#else
    this->Internal->Data[0] = vtkClientServerStream::LittleEndian;
#endif
    return 1;
  }
  else
  {
    // Data are invalid.  Reset the stream and report failure.
    this->Reset();
    return 0;
  }
}

//----------------------------------------------------------------------------
int vtkClientServerStream::ParseData()
{
  // Make sure we have at least one byte.
  if (this->Internal->Data.empty())
  {
    return 0;
  }

  // We are not modifying the vector size.  It is safe to use pointers
  // into it.
  unsigned char* begin = &*this->Internal->Data.begin();
  unsigned char* end = begin + this->Internal->Data.size();

  // Save the byte order.
  int order = *begin;

  // Traverse the data until no more commands are found.
  unsigned char* data = begin + 1;
  while (data && data < end)
  {
    // Parse the command.
    data = this->ParseCommand(order, data, begin, end);

    // Parse the arguments until End is reached.
    int foundEnd = 0;
    while (!foundEnd && data && data < end)
    {
      // Parse the type identifier.
      vtkClientServerStream::Types type;
      data = this->ParseType(order, data, begin, end, &type);
      if (!data)
      {
        break;
      }

      // Process the rest of the value based on its type.
      switch (type)
      {
        case vtkClientServerStream::int8_value:
        case vtkClientServerStream::uint8_value:
        case vtkClientServerStream::bool_value:
          data = this->ParseValue(order, data, end, 1);
          break;
        case vtkClientServerStream::int8_array:
        case vtkClientServerStream::uint8_array:
          data = this->ParseArray(order, data, end, 1);
          break;
        case vtkClientServerStream::int16_value:
        case vtkClientServerStream::uint16_value:
          data = this->ParseValue(order, data, end, 2);
          break;
        case vtkClientServerStream::int16_array:
        case vtkClientServerStream::uint16_array:
          data = this->ParseArray(order, data, end, 2);
          break;
        case vtkClientServerStream::id_value:
        case vtkClientServerStream::int32_value:
        case vtkClientServerStream::uint32_value:
        case vtkClientServerStream::float32_value:
          data = this->ParseValue(order, data, end, 4);
          break;
        case vtkClientServerStream::int32_array:
        case vtkClientServerStream::uint32_array:
        case vtkClientServerStream::float32_array:
          data = this->ParseArray(order, data, end, 4);
          break;
        case vtkClientServerStream::int64_value:
        case vtkClientServerStream::uint64_value:
        case vtkClientServerStream::float64_value:
          data = this->ParseValue(order, data, end, 8);
          break;
        case vtkClientServerStream::int64_array:
        case vtkClientServerStream::uint64_array:
        case vtkClientServerStream::float64_array:
          data = this->ParseArray(order, data, end, 8);
          break;
        case vtkClientServerStream::string_value:
          data = this->ParseString(order, data, end);
          break;
        case vtkClientServerStream::stream_value:
          data = this->ParseStream(order, data, end);
          break;
        case vtkClientServerStream::LastResult:
          // There are no data for this type.  Do nothing.
          break;
        case vtkClientServerStream::End:
          this->ParseEnd();
          foundEnd = 1;
          break;

        // An object pointer cannot be safely transferred in a buffer.
        case vtkClientServerStream::vtk_object_pointer:
        default: /* ERROR */
          data = 0;
          break;
      }
    }
  }

  // Return whether parsing finished.
  return (data == end) ? 1 : 0;
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseCommand(
  int order, unsigned char* data, unsigned char* begin, unsigned char* end)
{
  // Byte-swap this command's identifier.
  if (data > end - sizeof(vtkTypeUInt32))
  {
    /* ERROR */
    return 0;
  }
  this->PerformByteSwap(order, data, 1, sizeof(vtkTypeUInt32));

  // Mark the start of the command.
  this->Internal->StartIndex =
    this->Internal->ValueOffsets.end() - this->Internal->ValueOffsets.begin();
  this->Internal->ValueOffsets.push_back(data - begin);

  // Return the position after the command identifier.
  return data + sizeof(vtkTypeUInt32);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::ParseEnd()
{
  // Record completed message.
  this->Internal->MessageIndexes.push_back(this->Internal->StartIndex);
  this->Internal->StartIndex = vtkClientServerStreamInternals::InvalidStartIndex;
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseType(int order, unsigned char* data,
  unsigned char* begin, unsigned char* end, vtkClientServerStream::Types* type)
{
  // Read this type identifier.
  vtkTypeUInt32 tp;
  if (data > end - sizeof(tp))
  {
    /* ERROR */
    return 0;
  }
  this->PerformByteSwap(order, data, 1, sizeof(tp));
  memcpy(&tp, data, sizeof(tp));
  *type = static_cast<vtkClientServerStream::Types>(tp);

  // Record the start of this type and optional value.
  this->Internal->ValueOffsets.push_back(data - begin);

  // Return the position after the type identifier.
  return data + sizeof(vtkTypeUInt32);
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseValue(
  int order, unsigned char* data, unsigned char* end, unsigned int wordSize)
{
  // Byte-swap the value.
  if (data > end - wordSize)
  {
    /* ERROR */
    return 0;
  }
  this->PerformByteSwap(order, data, 1, wordSize);

  // Return the position after the value.
  return data + wordSize;
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseArray(
  int order, unsigned char* data, unsigned char* end, unsigned int wordSize)
{
  // Read the array length.
  vtkTypeUInt32 length;
  if (data > end - sizeof(length))
  {
    /* ERROR */
    return 0;
  }
  this->PerformByteSwap(order, data, 1, sizeof(length));
  memcpy(&length, data, sizeof(length));
  data += sizeof(length);

  // Calculate the size of the array data.
  vtkTypeUInt32 size = length * wordSize;

  // Byte-swap the array data.
  if (data > end - size)
  {
    /* ERROR */
    return 0;
  }
  this->PerformByteSwap(order, data, length, wordSize);

  // Return the position after the array.
  return data + size;
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseString(
  int order, unsigned char* data, unsigned char* end)
{
  // Read the string length.
  vtkTypeUInt32 length;
  if (data > end - sizeof(length))
  {
    /* ERROR */
    return 0;
  }
  this->PerformByteSwap(order, data, 1, sizeof(length));
  memcpy(&length, data, sizeof(length));
  data += sizeof(length);

  // Skip the string data.  It does not need swapping.
  if (data > end - length)
  {
    /* ERROR */
    return 0;
  }

  // Return the position after the string.
  return data + length;
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseStream(
  int order, unsigned char* data, unsigned char* end)
{
  // Stream data are represented as an array of bytes.
  return this->ParseArray(order, data, end, 1);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::PerformByteSwap(
  int dataByteOrder, unsigned char* data, unsigned int numWords, unsigned int wordSize)
{
  char* ptr = reinterpret_cast<char*>(data);
  if (dataByteOrder == vtkClientServerStream::BigEndian)
  {
    switch (wordSize)
    {
      case 1:
        break;
      case 2:
        vtkByteSwap::Swap2BERange(ptr, numWords);
        break;
      case 4:
        vtkByteSwap::Swap4BERange(ptr, numWords);
        break;
      case 8:
        vtkByteSwap::Swap8BERange(ptr, numWords);
        break;
      default:
        break;
    }
  }
  else
  {
    switch (wordSize)
    {
      case 1:
        break;
      case 2:
        vtkByteSwap::Swap2LERange(ptr, numWords);
        break;
      case 4:
        vtkByteSwap::Swap4LERange(ptr, numWords);
        break;
      case 8:
        vtkByteSwap::Swap8LERange(ptr, numWords);
        break;
      default:
        break;
    }
  }
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetNumberOfMessages() const
{
  return static_cast<int>(this->Internal->MessageIndexes.size());
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetNumberOfValues(int message) const
{
  if (!this->Internal->Invalid && message >= 0 && message < this->GetNumberOfMessages())
  {
    if (message + 1 < this->GetNumberOfMessages())
    {
      // Requested message is not the last message.  Use the beginning
      // of the next message to find its length.
      return static_cast<int>(
        this->Internal->MessageIndexes[message + 1] - this->Internal->MessageIndexes[message]);
    }
    else if (this->Internal->StartIndex != vtkClientServerStreamInternals::InvalidStartIndex)
    {
      // Requested message is the last completed message, but there is
      // a partial message in progress.  Use the beginning of the next
      // partial message to find this message's length.
      return static_cast<int>(this->Internal->StartIndex - this->Internal->MessageIndexes[message]);
    }
    else
    {
      // Requested message is the last message.  Use the length of the
      // value indices array to find the message's length.
      return static_cast<int>(
        this->Internal->ValueOffsets.size() - this->Internal->MessageIndexes[message]);
    }
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
const unsigned char* vtkClientServerStream::GetValue(int message, int value) const
{
  if (value >= 0 && value < this->GetNumberOfValues(message))
  {
    // Get the index to the beginning of the requested message.
    vtkClientServerStreamInternals::ValueOffsetsType::size_type index =
      this->Internal->MessageIndexes[message];

    // Return a pointer to the value-th value in the message.
    const unsigned char* data = &*this->Internal->Data.begin();
    return data + this->Internal->ValueOffsets[index + value];
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
// Templates to find argument size within a stream.
template <class T>
size_t vtkClientServerStreamValueSize(T*)
{
  return sizeof(T);
}
template <class T>
size_t vtkClientServerStreamArraySize(const unsigned char* data, T*)
{
  // Get the length of the value in the stream.
  vtkTypeUInt32 len;
  memcpy(&len, data, sizeof(len));
  return sizeof(len) + len * sizeof(T);
}

vtkClientServerStream::Argument vtkClientServerStream::GetArgument(int message, int argument) const
{
  // Prepare a return value.
  vtkClientServerStream::Argument result = { 0, 0 };

  // Get a pointer to the type/value pair in the stream.
  if (const unsigned char* data = this->GetValue(message, 1 + argument))
  {
    // Store the starting location of the value.
    result.Data = data;

    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Find the length of the argument's data based on its type.
    switch (tp)
    {
      VTK_CSS_TEMPLATE_MACRO(value, result.Size = sizeof(tp) + vtkClientServerStreamValueSize(T));
      VTK_CSS_TEMPLATE_MACRO(
        array, result.Size = sizeof(tp) + vtkClientServerStreamArraySize(data, T));
      case vtkClientServerStream::id_value:
      {
        result.Size = sizeof(tp) + sizeof(vtkClientServerID().ID);
      }
      break;
      case vtkClientServerStream::string_value:
      {
        // A string is represented as an array of 1 byte values.
        vtkTypeUInt8* T = 0;
        result.Size = sizeof(tp) + vtkClientServerStreamArraySize(data, T);
      }
      break;
      case vtkClientServerStream::vtk_object_pointer:
      {
        result.Size = sizeof(tp) + sizeof(vtkObjectBase*);
      }
      break;
      case vtkClientServerStream::stream_value:
      {
        // A stream is represented as an array of 1 byte values.
        vtkTypeUInt8* T = 0;
        result.Size = sizeof(tp) + vtkClientServerStreamArraySize(data, T);
      }
      break;
      case vtkClientServerStream::LastResult:
      {
        result.Size = sizeof(tp);
      }
      break;
      case vtkClientServerStream::End:
      default:
        result.Data = 0;
        break;
    }
  }
  return result;
}

//----------------------------------------------------------------------------
vtkClientServerStream::Commands vtkClientServerStream::GetCommand(int message) const
{
  // The first value in a message is always the command.
  if (const unsigned char* data = this->GetValue(message, 0))
  {
    // Retrieve the command value and convert it to the proper type.
    vtkTypeUInt32 cmd;
    memcpy(&cmd, data, sizeof(cmd));
    if (cmd < vtkClientServerStream::EndOfCommands)
    {
      return static_cast<vtkClientServerStream::Commands>(cmd);
    }
  }
  return vtkClientServerStream::EndOfCommands;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetNumberOfArguments(int message) const
{
  // Arguments do not include the Command or End markers.
  if (int numValues = this->GetNumberOfValues(message))
  {
    return numValues - 2;
  }
  else
  {
    return -1;
  }
}

//----------------------------------------------------------------------------
vtkClientServerStream::Types vtkClientServerStream::GetArgumentType(int message, int argument) const
{
  // Get a pointer to the type/value pair in the stream.
  if (const unsigned char* data = this->GetValue(message, 1 + argument))
  {
    // Get the type of the value in the stream and convert it.
    vtkTypeUInt32 type;
    memcpy(&type, data, sizeof(type));
    if (type < vtkClientServerStream::End)
    {
      return static_cast<vtkClientServerStream::Types>(type);
    }
  }
  return vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
// Map from the vtkClientServerStream::Types enumeration to strings.
// This must be kept in-sync with the enumeration.
static const char* const vtkClientServerStreamTypeNames[][4] = { { "int8_value", "int8", "i8", 0 },
  { "int8_array", "int8a", "i8a", 0 }, { "int16_value", "int16", "i16", 0 },
  { "int16_array", "int16a", "i16a", 0 }, { "int32_value", "int32", "i32", 0 },
  { "int32_array", "int32a", "i32a", 0 }, { "int64_value", "int64", "i64", 0 },
  { "int64_array", "int64a", "i64a", 0 }, { "uint8_value", "uint8", "u8", 0 },
  { "uint8_array", "uint8a", "u8a", 0 }, { "uint16_value", "uint16", "u16", 0 },
  { "uint16_array", "uint16a", "u16a", 0 }, { "uint32_value", "uint32", "u32", 0 },
  { "uint32_array", "uint32a", "u32a", 0 }, { "uint64_value", "uint64", "u64", 0 },
  { "uint64_array", "uint64a", "u64a", 0 }, { "float32_value", "float32", "f32", 0 },
  { "float32_array", "float32a", "f32a", 0 }, { "float64_value", "float64", "f64", 0 },
  { "float64_array", "float64a", "f64a", 0 }, { "bool_value", "bool", "bool", 0 },
  { "string_value", "string", "str", 0 }, { "id_value", "id", 0, 0 },
  { "vtk_object_pointer", "obj", 0, 0 }, { "stream_value", "stream", 0, 0 },
  { "LastResult", "result", 0, 0 }, { "End", 0, 0, 0 }, { 0, 0, 0, 0 } };

//----------------------------------------------------------------------------
const char* vtkClientServerStream::GetStringFromType(vtkClientServerStream::Types type)
{
  return vtkClientServerStream::GetStringFromType(type, 0);
}

//----------------------------------------------------------------------------
const char* vtkClientServerStream::GetStringFromType(vtkClientServerStream::Types type, int index)
{
  // Lookup the type if it is in range.
  if (type >= vtkClientServerStream::int8_value && type <= vtkClientServerStream::End)
  {
    if (index <= 0)
    {
      return vtkClientServerStreamTypeNames[type][0];
    }
    const char* const* names = vtkClientServerStreamTypeNames[type];
    int i = 1;
    while (i < index && names[i])
    {
      ++i;
    }
    if (names[i])
    {
      return names[i];
    }
    else
    {
      return names[i - 1];
    }
  }
  else
  {
    return "unknown";
  }
}

//----------------------------------------------------------------------------
vtkClientServerStream::Types vtkClientServerStream::GetTypeFromString(const char* name)
{
  return vtkClientServerStream::GetTypeFromString(name, 0);
}

//----------------------------------------------------------------------------
vtkClientServerStream::Types vtkClientServerStream::GetTypeFromString(
  const char* begin, const char* end)
{
  // Setup the ending position.
  if (begin && (!end || end < begin))
  {
    end = begin + strlen(begin);
  }

  // Find a string matching the given name.
  for (int t = vtkClientServerStream::int8_value; begin && t < vtkClientServerStream::End; ++t)
  {
    for (const char* const* n = vtkClientServerStreamTypeNames[t]; *n; ++n)
    {
      if (strncmp(*n, begin, end - begin) == 0)
      {
        return static_cast<vtkClientServerStream::Types>(t);
      }
    }
  }
  return vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
// Map from the vtkClientServerStream::Commands enumeration to strings.
// This must be kept in-sync with the enumeration.
static const char* const vtkClientServerStreamCommandNames[] = { "New", "Invoke", "Delete",
  "Assign", "Reply", "Error", "EndOfCommands", 0 };

//----------------------------------------------------------------------------
const char* vtkClientServerStream::GetStringFromCommand(vtkClientServerStream::Commands cmd)
{
  // Lookup the command if it is in range.
  if (cmd >= vtkClientServerStream::New && cmd <= vtkClientServerStream::EndOfCommands)
  {
    return vtkClientServerStreamCommandNames[cmd];
  }
  else
  {
    return "unknown";
  }
}

//----------------------------------------------------------------------------
vtkClientServerStream::Commands vtkClientServerStream::GetCommandFromString(const char* name)
{
  return vtkClientServerStream::GetCommandFromString(name, 0);
}

//----------------------------------------------------------------------------
vtkClientServerStream::Commands vtkClientServerStream::GetCommandFromString(
  const char* begin, const char* end)
{
  // Setup the ending position.
  if (begin && (!end || end < begin))
  {
    end = begin + strlen(begin);
  }

  // Find a string matching the given name.
  for (int c = vtkClientServerStream::New; begin && c < vtkClientServerStream::EndOfCommands; ++c)
  {
    if (strncmp(vtkClientServerStreamCommandNames[c], begin, end - begin) == 0)
    {
      return static_cast<vtkClientServerStream::Commands>(c);
    }
  }
  return vtkClientServerStream::EndOfCommands;
}

//----------------------------------------------------------------------------
void vtkClientServerStream::Print(ostream& os) const
{
  vtkIndent indent;
  this->Print(os, indent);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::Print(ostream& os, vtkIndent indent) const
{
  for (int m = 0; m < this->GetNumberOfMessages(); ++m)
  {
    this->PrintMessage(os, m, indent);
  }
}

//----------------------------------------------------------------------------
void vtkClientServerStream::PrintMessage(ostream& os, int message) const
{
  vtkIndent indent;
  this->PrintMessage(os, message, indent);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::PrintMessage(ostream& os, int message, vtkIndent indent) const
{
  os << indent << "Message " << message << " = ";
  os << this->GetStringFromCommand(this->GetCommand(message)) << "\n";
  for (int a = 0; a < this->GetNumberOfArguments(message); ++a)
  {
    this->PrintArgument(os, message, a, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
void vtkClientServerStream::PrintArgument(ostream& os, int message, int argument) const
{
  vtkIndent indent;
  this->PrintArgument(os, message, argument, indent);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::PrintArgument(
  ostream& os, int message, int argument, vtkIndent indent) const
{
  this->PrintArgumentInternal(os, message, argument, 1, indent);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::PrintArgumentValue(ostream& os, int message, int argument) const
{
  vtkIndent indent;
  this->PrintArgumentInternal(os, message, argument, 0, indent);
}

//----------------------------------------------------------------------------
// Function to convert a value to a string.
template <class T>
void vtkClientServerStreamValueToString(
  const vtkClientServerStream* self, ostream& os, int m, int a, T*)
{
  typedef VTK_CSS_TYPENAME vtkTypeTraits<T>::PrintType PrintType;
  T arg = T();
  self->GetArgument(m, a, &arg);
  os << static_cast<PrintType>(arg);
}

//----------------------------------------------------------------------------
// Function to convert an array to a string.
template <class T>
void vtkClientServerStreamArrayToString(
  const vtkClientServerStream* self, ostream& os, int m, int a, T*)
{
  typedef VTK_CSS_TYPENAME vtkTypeTraits<T>::PrintType PrintType;
  vtkTypeUInt32 length;
  T arglocal[6];
  T* arg = arglocal;
  self->GetArgumentLength(m, a, &length);
  if (length > 6)
  {
    arg = new T[length];
  }
  self->GetArgument(m, a, arg, length);
  const char* comma = "";
  for (vtkTypeUInt32 i = 0; i < length; ++i)
  {
    os << comma << static_cast<PrintType>(arg[i]);
    comma = ", ";
  }
  if (arg != arglocal)
  {
    delete[] arg;
  }
}

//----------------------------------------------------------------------------
// Function to print a value argument.
template <class T>
void vtkClientServerStreamPrintValue(
  const vtkClientServerStream* self, ostream& os, vtkIndent indent, int m, int a, int t, T* tt)
{
  if (t)
  {
    vtkClientServerStream::Types type = self->GetArgumentType(m, a);
    os << indent << "Argument " << a << " = " << self->GetStringFromType(type) << " {";
  }
  vtkClientServerStreamValueToString(self, os, m, a, tt);
  if (t)
  {
    os << "}\n";
  }
}

//----------------------------------------------------------------------------
// Function to print an array argument.
template <class T>
void vtkClientServerStreamPrintArray(
  const vtkClientServerStream* self, ostream& os, vtkIndent indent, int m, int a, int t, T* tt)
{
  if (t)
  {
    vtkClientServerStream::Types type = self->GetArgumentType(m, a);
    os << indent << "Argument " << a << " = " << self->GetStringFromType(type) << " {";
  }
  vtkClientServerStreamArrayToString(self, os, m, a, tt);
  if (t)
  {
    os << "}\n";
  }
}

//----------------------------------------------------------------------------
void vtkClientServerStream::PrintArgumentInternal(
  ostream& os, int message, int argument, int annotate, vtkIndent indent) const
{
  switch (this->GetArgumentType(message, argument))
  {
    VTK_CSS_TEMPLATE_MACRO(
      value, vtkClientServerStreamPrintValue(this, os, indent, message, argument, annotate, T));
    VTK_CSS_TEMPLATE_MACRO(
      array, vtkClientServerStreamPrintArray(this, os, indent, message, argument, annotate, T));
    case vtkClientServerStream::bool_value:
    {
      bool arg;
      int result = this->GetArgument(message, argument, &arg);
      if (annotate)
      {
        os << indent << "Argument " << argument << " = bool_value ";
        os << "{" << (arg ? "true" : "false") << "}\n";
      }
      else if (result)
      {
        os << (arg ? "true" : "false");
      }
    }
    break;
    case vtkClientServerStream::string_value:
    {
      const char* arg = NULL;
      this->GetArgument(message, argument, &arg);
      if (annotate)
      {
        os << indent << "Argument " << argument << " = string_value ";
        if (arg)
        {
          os << "{" << arg << "}\n";
        }
        else
        {
          os << "(null)\n";
        }
      }
      else if (arg)
      {
        os << arg;
      }
    }
    break;
    case vtkClientServerStream::stream_value:
    {
      vtkClientServerStream arg;
      int result = this->GetArgument(message, argument, &arg);
      if (annotate)
      {
        os << indent << "Argument " << argument << " = stream_value ";
        if (result)
        {
          vtkIndent nextIndent = indent.GetNextIndent();
          os << "{\n";
          arg.Print(os, nextIndent);
          os << nextIndent << "}\n";
        }
        else
        {
          os << "invalid\n";
        }
      }
      else if (result)
      {
        arg.Print(os);
      }
    }
    break;
    case vtkClientServerStream::id_value:
    {
      vtkClientServerID arg;
      this->GetArgument(message, argument, &arg);
      if (annotate)
      {
        os << indent << "Argument " << argument << " = id_value {" << arg.ID << "}\n";
      }
      else
      {
        os << arg.ID;
      }
    }
    break;
    case vtkClientServerStream::vtk_object_pointer:
    {
      vtkObjectBase* arg;
      this->GetArgument(message, argument, &arg);
      if (annotate)
      {
        os << indent << "Argument " << argument << " = vtk_object_pointer ";
        if (arg)
        {
          os << "{" << arg->GetClassName() << " (" << arg << ")}\n";
        }
        else
        {
          os << "(null)\n";
        }
      }
      else
      {
        os << arg;
      }
    }
    break;
    case vtkClientServerStream::LastResult:
    {
      if (annotate)
      {
        os << indent << "Argument " << argument << " = LastResult\n";
      }
    }
    break;
    default:
    {
      if (annotate)
      {
        os << indent << "Argument " << argument << " = invalid\n";
      }
    }
    break;
  }
}

//----------------------------------------------------------------------------
const char* vtkClientServerStream::StreamToString() const
{
  std::ostringstream ostr;
  this->StreamToString(ostr);
  this->Internal->String = ostr.str();
  return this->Internal->String.c_str();
}

//----------------------------------------------------------------------------
void vtkClientServerStream::StreamToString(ostream& os) const
{
  vtkIndent indent;
  this->StreamToString(os, indent);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::StreamToString(ostream& os, vtkIndent indent) const
{
  for (int m = 0; m < this->GetNumberOfMessages(); ++m)
  {
    os << indent;
    this->MessageToString(os, m, indent);
  }
}

//----------------------------------------------------------------------------
void vtkClientServerStream::MessageToString(ostream& os, int m) const
{
  vtkIndent indent;
  this->MessageToString(os, m, indent);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::MessageToString(ostream& os, int m, vtkIndent indent) const
{
  os << this->GetStringFromCommand(this->GetCommand(m));
  for (int a = 0; a < this->GetNumberOfArguments(m); ++a)
  {
    os << " ";
    this->ArgumentToString(os, m, a, indent);
  }
  os << "\n";
}

//----------------------------------------------------------------------------
void vtkClientServerStream::ArgumentToString(ostream& os, int m, int a) const
{
  vtkIndent indent;
  this->ArgumentToString(os, m, a, indent);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::ArgumentToString(ostream& os, int m, int a, vtkIndent indent) const
{
  vtkClientServerStream::Types type = this->GetArgumentType(m, a);

  // Special case for strings: string0 == null, string() == ""
  if (type == vtkClientServerStream::string_value)
  {
    const char* arg = NULL;
    this->GetArgument(m, a, &arg);
    if (!arg)
    {
      os << "string0";
      return;
    }
    int needType = (*arg == 0) ? 1 : 0;
    for (const char* c = arg; *c; ++c)
    {
      if (*c == '(' || *c == ')')
      {
        needType = 1;
        break;
      }
    }
    if (!needType)
    {
      this->ArgumentValueToString(os, m, a, indent);
      return;
    }
  }

  os << this->GetStringFromType(type, 1) << "(";
  this->ArgumentValueToString(os, m, a, indent);
  os << ")";
}

//----------------------------------------------------------------------------
void vtkClientServerStream::ArgumentValueToString(ostream& os, int m, int a, vtkIndent indent) const
{
  switch (this->GetArgumentType(m, a))
  {
    VTK_CSS_TEMPLATE_MACRO(value, vtkClientServerStreamValueToString(this, os, m, a, T));
    VTK_CSS_TEMPLATE_MACRO(array, vtkClientServerStreamArrayToString(this, os, m, a, T));
    case vtkClientServerStream::bool_value:
    {
      bool arg;
      this->GetArgument(m, a, &arg);
      os << (arg ? "true" : "false");
    }
    break;
    case vtkClientServerStream::string_value:
    {
      const char* arg = NULL;
      this->GetArgument(m, a, &arg);
      if (arg)
      {
        for (const char* c = arg; *c; ++c)
        {
          switch (*c)
          {
            case '\\':
              os << "\\\\";
              break;
            case '(':
              os << "\\(";
              break;
            case ')':
              os << "\\)";
              break;
            default:
              os << *c;
              break;
          }
        }
      }
    }
    break;
    case vtkClientServerStream::stream_value:
    {
      vtkClientServerStream arg;
      int result = this->GetArgument(m, a, &arg);
      if (result)
      {
        os << "\n";
        arg.StreamToString(os, indent.GetNextIndent());
        os << indent;
      }
    }
    break;
    case vtkClientServerStream::id_value:
    {
      vtkClientServerID arg;
      this->GetArgument(m, a, &arg);
      os << arg.ID;
    }
    break;
    case vtkClientServerStream::vtk_object_pointer:
    {
      vtkObjectBase* arg;
      this->GetArgument(m, a, &arg);
      if (arg)
      {
        os << arg;
      }
      else
      {
        os << "0";
      }
    }
    break;
    case vtkClientServerStream::LastResult:
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
int vtkClientServerStream::StreamFromString(const char* str)
{
  this->Reset();
  if (this->StreamFromStringInternal(str, str + strlen(str)))
  {
    return 1;
  }
  else
  {
    this->Reset();
    return 0;
  }
}

//----------------------------------------------------------------------------
int vtkClientServerStream::StreamFromStringInternal(const char* begin, const char* end)
{
  const char* position = begin;
  while (1)
  {
    // Skip whitespace and newlines.
    while (position < end &&
      (*position == ' ' || *position == '\t' || *position == '\r' || *position == '\n'))
    {
      ++position;
    }
    if (position == end)
    {
      break;
    }

    // Add this message.
    if (!this->AddMessageFromString(position, end, &position))
    {
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::AddMessageFromString(
  const char* begin, const char* end, const char** next)
{
  // Find the end of the command name.
  const char* commandBegin = begin;
  const char* commandEnd = commandBegin;
  while (commandEnd < end &&
    !(*commandEnd == ' ' || *commandEnd == '\t' || *commandEnd == '\r' || *commandEnd == '\n'))
  {
    ++commandEnd;
  }
  vtkClientServerStream::Commands cmd = this->GetCommandFromString(commandBegin, commandEnd);
  if (cmd == vtkClientServerStream::EndOfCommands)
  {
    // The command is missing, try to guess it based on the first
    // argument.
    if ((commandEnd - commandBegin > 3 && strncmp(commandBegin, "id(", 3) == 0) ||
      (commandEnd - commandBegin == 8 && strncmp(commandBegin, "result()", 8) == 0) ||
      (commandEnd - commandBegin == 12 && strncmp(commandBegin, "LastResult()", 12) == 0))
    {
      // First argument is an id.  Assume this is an Invoke command.
      cmd = vtkClientServerStream::Invoke;
    }
    else if (commandEnd - commandBegin > 3 && strncmp(commandBegin, "vtk", 3) == 0)
    {
      // First argument looks like the name of a VTK class.  Assume
      // this is a New command.
      cmd = vtkClientServerStream::New;
    }
    else
    {
      // We did not find a message.
      return 0;
    }

    // We guessed the command, so the first argument starts at
    // commandBegin.
    commandEnd = commandBegin;
  }

  // Insert the command into the stream.
  *this << cmd;

  // Scan for arguments until the end of a line occurs outside an
  // argument.
  const char* position = commandEnd;
  while (1)
  {
    // Skip whitespace.
    while (position < end && (*position == ' ' || *position == '\t'))
    {
      ++position;
    }

    // Check for end-of-line.
    if (position == end || *position == '\r' || *position == '\n')
    {
      break;
    }

    // Add the next argument.
    if (!this->AddArgumentFromString(position, end, &position))
    {
      return 0;
    }
  }

  // Insert the message ending token into the stream.
  *this << vtkClientServerStream::End;

  // Report where to continue scanning.
  *next = position;
  return 1;
}

//----------------------------------------------------------------------------
int vtkClientServerStreamPointerFromString(
  const char* begin, const char* end, vtkObjectBase** value)
{
  // Copy the value to a null-terminated buffer.
  char buffer[60];
  char* ptr = buffer;
  if (end - begin + 1 > 60)
  {
    ptr = new char[end - begin + 1];
  }
  strncpy(ptr, begin, end - begin);
  ptr[end - begin] = 0;

  // Try to convert the value.
  int result = sscanf(ptr, "%p", value) ? 1 : 0;

  // Free the buffer.
  if (ptr != buffer)
  {
    delete[] ptr;
  }
  return result;
}

//----------------------------------------------------------------------------
template <class T>
int vtkClientServerStreamValueFromString(const char* begin, const char* end, T* value)
{
  // Copy the value to a null-terminated buffer.
  char buffer[60];
  char* ptr = buffer;
  if (end - begin + 1 > 60)
  {
    ptr = new char[end - begin + 1];
  }
  strncpy(ptr, begin, end - begin);
  ptr[end - begin] = 0;

  // Try to convert the value.
  VTK_CSS_TYPENAME vtkTypeTraits<T>::PrintType pvalue;
  int result = sscanf(ptr, vtkTypeTraits<T>::ParseFormat(), &pvalue) ? 1 : 0;
  if (result)
  {
    *value = static_cast<T>(pvalue);
  }

  // Free the buffer.
  if (ptr != buffer)
  {
    delete[] ptr;
  }
  return result;
}

//----------------------------------------------------------------------------
int vtkClientServerStreamBoolFromString(const char* begin, const char* end, bool* value)
{
  // Scan for the beginning of the value.
  const char* valueBegin = begin;
  while (valueBegin < end &&
    (*valueBegin == ' ' || *valueBegin == '\t' || *valueBegin == '\r' || *valueBegin == '\n'))
  {
    ++valueBegin;
  }

  // Scan for the end of the value.
  const char* valueEnd = valueBegin;
  while (valueEnd < end &&
    (*valueEnd != ' ' && *valueEnd != '\t' && *valueEnd != '\r' && *valueEnd != '\n'))
  {
    ++valueEnd;
  }

  // Make sure this was the only value.
  while (valueEnd < end)
  {
    if (*valueEnd != ' ' && *valueEnd != '\t' && *valueEnd != '\r' && *valueEnd != '\n')
    {
      return 0;
    }
  }

  // Check the value.
  if (valueEnd - valueBegin == 4 &&
    (valueBegin[0] == 't' && valueBegin[1] == 'r' && valueBegin[2] == 'u' && valueBegin[3] == 'e'))
  {
    *value = true;
    return 1;
  }
  else if (valueEnd - valueBegin == 5 &&
    (valueBegin[0] == 'f' && valueBegin[1] == 'a' && valueBegin[2] == 'l' && valueBegin[3] == 's' &&
             valueBegin[4] == 'e'))
  {
    *value = false;
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
template <class T>
int vtkClientServerStreamValueFromString(
  vtkClientServerStream* self, const char* begin, const char* end, T*)
{
  T value;
  if (vtkClientServerStreamValueFromString(begin, end, &value))
  {
    *self << value;
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
template <class T>
int vtkClientServerStreamArrayFromString(
  vtkClientServerStream* self, const char* begin, const char* end, T*)
{
  // Count how many values will be read.
  int length = 0;
  int text = 0;
  int comma = 0;
  for (const char* c = begin; c < end; ++c)
  {
    switch (*c)
    {
      case ' ':
        break;
      case '\t':
        break;
      case '\r':
        break;
      case '\n':
        break;
      case ',':
        comma = 1;
        break;
      default:
        text = 1;
        break;
    }
    if (comma)
    {
      if (text)
      {
        ++length;
      }
      text = 0;
      comma = 0;
    }
  }
  if (text)
  {
    ++length;
  }

  // Allocate the array.
  T arraylocal[6];
  T* ptr = arraylocal;
  if (length > 6)
  {
    ptr = new T[length];
  }

  // Parse each value.
  int result = 1;
  int index = 0;
  const char* valueBegin = begin;
  while (result && valueBegin < end && index < length)
  {
    // Scan for the beginning of the value.
    while (valueBegin < end &&
      (*valueBegin == ' ' || *valueBegin == '\t' || *valueBegin == '\r' || *valueBegin == '\n'))
    {
      ++valueBegin;
    }

    // Scan for the end of the value.
    const char* valueEnd = valueBegin;
    while (valueEnd < end && *valueEnd != ',' && *valueEnd != ' ' && *valueEnd != '\t' &&
      *valueEnd != '\r' && *valueEnd != '\n')
    {
      ++valueEnd;
    }

    // Parse this value.
    if (!vtkClientServerStreamValueFromString(valueBegin, valueEnd, ptr + index))
    {
      result = 0;
    }
    ++index;

    // Scan past the comma.
    valueBegin = valueEnd;
    while (valueBegin < end && *valueBegin != ',')
    {
      ++valueBegin;
    }
    if (valueBegin < end && *valueBegin == ',')
    {
      ++valueBegin;
    }
  }

  // Make sure we got all the values.
  if (index < length)
  {
    result = 0;
  }

  // Insert the array.
  if (result)
  {
    *self << vtkClientServerStream::InsertArray(ptr, length);
  }

  // Free the array.
  if (ptr != arraylocal)
  {
    delete[] ptr;
  }

  return result;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::AddArgumentFromString(
  const char* begin, const char* end, const char** next)
{
  // Find the end of the argument type.
  const char* typeBegin = begin;
  const char* typeEnd = typeBegin;
  int done = 0;
  while (!done && typeEnd < end)
  {
    switch (*typeEnd)
    {
      case '(':
        done = 1;
        break;
      case ' ':
        done = 1;
        break;
      case '\t':
        done = 1;
        break;
      case '\r':
        done = 1;
        break;
      case '\n':
        done = 1;
        break;
      default:
        ++typeEnd;
        break;
    }
  }

  // If we did not find a paren, just assume it is a string argument.
  if (*typeEnd != '(')
  {
    // Report where to continue scanning.
    *next = typeEnd;

    // Special case for strings: string0 == null
    if ((strncmp(typeBegin, "string0", typeEnd - typeBegin) == 0) ||
      (strncmp(typeBegin, "str0", typeEnd - typeBegin) == 0))
    {
      // Insert the null string.
      *this << static_cast<char*>(0);
      return 1;
    }

    // Insert the string argument.
    *this << vtkClientServerStream::InsertString(typeBegin, typeEnd);
    return 1;
  }

  // Get the argument type.
  vtkClientServerStream::Types type = vtkClientServerStream::GetTypeFromString(typeBegin, typeEnd);
  if (type == vtkClientServerStream::End)
  {
    // Unknown type!
    return 0;
  }

  // Find the argument value boundaries.
  const char* valueBegin = typeEnd + 1;
  const char* valueEnd = valueBegin;
  int nesting = 1;
  int hasComma = 0;
  done = 0;
  while (!done && valueEnd < end)
  {
    switch (*valueEnd)
    {
      case '\\':
        if (++valueEnd < end)
        {
          ++valueEnd;
        }
        break;
      case '(':
        ++nesting;
        ++valueEnd;
        break;
      case ')':
        if (--nesting == 0)
        {
          done = 1;
        }
        else
        {
          ++valueEnd;
        }
        break;
      case ',':
        ++valueEnd;
        hasComma = 1;
        break;
      default:
        ++valueEnd;
        break;
    }
  }

  if (valueEnd == end)
  {
    // Unterminated argument value!
    return 0;
  }

  // If the type is a scalar type and the value has a comma, convert
  // the type to an array type.
  switch (type)
  {
    case int8_value:
      if (hasComma)
      {
        type = int8_array;
      }
      break;
    case int16_value:
      if (hasComma)
      {
        type = int16_array;
      }
      break;
    case int32_value:
      if (hasComma)
      {
        type = int32_array;
      }
      break;
    case int64_value:
      if (hasComma)
      {
        type = int64_array;
      }
      break;
    case uint8_value:
      if (hasComma)
      {
        type = uint8_array;
      }
      break;
    case uint16_value:
      if (hasComma)
      {
        type = uint16_array;
      }
      break;
    case uint32_value:
      if (hasComma)
      {
        type = uint32_array;
      }
      break;
    case uint64_value:
      if (hasComma)
      {
        type = uint64_array;
      }
      break;
    case float32_value:
      if (hasComma)
      {
        type = float32_array;
      }
      break;
    case float64_value:
      if (hasComma)
      {
        type = float64_array;
      }
      break;
    default:
      break;
  }

  // Convert the value from a string to the proper type.
  int result = 0;
  switch (type)
  {
    VTK_CSS_TEMPLATE_MACRO(
      value, result = vtkClientServerStreamValueFromString(this, valueBegin, valueEnd, T));
    VTK_CSS_TEMPLATE_MACRO(
      array, result = vtkClientServerStreamArrayFromString(this, valueBegin, valueEnd, T));
    case vtkClientServerStream::bool_value:
    {
      // Convert the bool value from a string.
      bool arg;
      result = vtkClientServerStreamBoolFromString(valueBegin, valueEnd, &arg);
      if (result)
      {
        *this << arg;
      }
    }
    break;
    case vtkClientServerStream::string_value:
    {
      // Allocate a buffer for the string.
      char buffer[128];
      char* ptr = buffer;
      if (valueEnd - valueBegin + 1 > 128)
      {
        ptr = new char[valueEnd - valueEnd + 1];
      }

      // Copy the string to the buffer removing escapes.
      char* out = ptr;
      for (const char* c = valueBegin; c < valueEnd; ++c)
      {
        switch (*c)
        {
          case '\\':
            if (++c < valueEnd)
            {
              *out++ = *c;
            }
            VTK_FALLTHROUGH;
          default:
            *out++ = *c;
            break;
        }
      }
      *out++ = 0;

      // Insert the string argument.
      *this << out;

      // Free the buffer.
      if (ptr != buffer)
      {
        delete[] ptr;
      }

      result = 1;
    }
    break;
    case vtkClientServerStream::stream_value:
    {
      vtkClientServerStream arg;
      result = arg.StreamFromStringInternal(valueBegin, valueEnd);
      if (result)
      {
        *this << arg;
      }
    }
    break;
    case vtkClientServerStream::id_value:
    {
      // Convert the ID value from a string.
      vtkClientServerID arg;
      result = vtkClientServerStreamValueFromString(valueBegin, valueEnd, &arg.ID);
      if (result)
      {
        *this << arg;
      }
    }
    break;
    case vtkClientServerStream::vtk_object_pointer:
    {
      vtkObjectBase* arg;
      result = vtkClientServerStreamPointerFromString(valueBegin, valueEnd, &arg);
      if (result)
      {
        *this << arg;
      }
    }
    break;
    case vtkClientServerStream::LastResult:
    {
      *this << vtkClientServerStream::LastResult;
      result = 1;
    }
    break;
    default:
      break;
  }

  // Report where to continue scanning.
  if (result)
  {
    *next = valueEnd + 1;
  }

  return result;
}
