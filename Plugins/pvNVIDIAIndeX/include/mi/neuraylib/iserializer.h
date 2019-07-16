/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief      Serialization of objects to a byte stream.

#ifndef MI_NEURAYLIB_ISERIALIZER_H
#define MI_NEURAYLIB_ISERIALIZER_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {

class ISerializer;
class IDeserializer;

/** 
    \defgroup mi_neuray_plugins Extensions and Plugins
    \ingroup mi_neuray

    Various ways to extend the \NeurayApiName, for example, \if IRAY_API image and \endif video
    plugins, or user-defined DB elements.
*/
/** 
\if IRAY_API \addtogroup mi_neuray_plugins
\elseif MDL_SOURCE_RELEASE \addtogroup mi_neuray_plugins
\else \addtogroup mi_neuray_dice
\endif
@{
*/

/// A tag represents a unique identifier for database elements in the database.
///
/// This POD type is the underlying storage class for #mi::neuraylib::Tag. The POD type is used to
/// pass arguments and return values safely across the API boundary. User code should use
/// #mi::neuraylib::Tag instead.
///
/// Due to class inheritance you can pass instances of #mi::neuraylib::Tag for parameters of the POD
/// type. Similarly, #mi::neuraylib::Tag has a constructor that accepts the POD type as argument.
/// Hence, you can assign the return value of methods returning an instance of the POD type directly
/// to an instance of #mi::neuraylib::Tag.
struct Tag_struct
{
    /// The actual ID of the tag.
    Uint32 id;
};

/// A tag represents a unique identifier for database elements in the database.
///
/// \see the underlying POD type #mi::neuraylib::Tag_struct.
class Tag : public Tag_struct
{
public:
    /// Constructor.
    ///
    /// Sets the ID initially to 0, i.e., creates an invalid tag.
    Tag() { id = 0; }

    /// Constructor from the underlying storage type.
    Tag( Tag_struct tag_struct) { id = tag_struct.id; }

    /// Returns \c true if the tag is valid, i.e., is not equal to #NULL_TAG.
    bool is_valid() const { return id != 0; }

    /// Helper typedef.
    ///
    /// This typedef represents the type of #is_valid() used by the #bool_conversion_support()
    /// operator.
    typedef bool (Tag::*bool_conversion_support)() const;

    /// Helper function for the conversion of a Tag to a bool.
    ///
    /// This helper function allows to write
    ///   \code
    ///     Tag t (...);
    ///     if (t) ...
    ///   \endcode
    /// instead of
    ///   \code
    ///     Tag t (...);
    ///     if (t.is_valid()) ...
    ///   \endcode
    operator bool_conversion_support() const
    {
        return is_valid() ? &Tag::is_valid : 0;
    }
};

/// Returns \c true if \p lhs is equal to \p rhs.
inline bool operator==( const Tag& lhs, const Tag& rhs)
{
    return lhs.id == rhs.id;
}

/// Returns \c true if \p lhs is not equal to \p rhs.
inline bool operator!=( const Tag& lhs, const Tag& rhs)
{
    return lhs.id != rhs.id;
}

/// Returns \c true if \p lhs is less than \p rhs.
inline bool operator<( const Tag& lhs, const Tag& rhs)
{
    return lhs.id < rhs.id;
}

/// Returns \c true if \p lhs is greater than \p rhs.
inline bool operator>( const Tag& lhs, const Tag& rhs)
{
    return lhs.id > rhs.id;
}

/// Returns \c true if \p lhs is less than or equal to \p rhs.
inline bool operator<=( const Tag& lhs, const Tag& rhs)
{
    return lhs.id <= rhs.id;
}

/// Returns \c true if \p lhs is greater than or equal to \p rhs.
inline bool operator>=( const Tag& lhs, const Tag& rhs)
{
    return lhs.id >= rhs.id;
}

/// This value of the tag represents an invalid tag which can not be accessed.
const Tag NULL_TAG;

/// All serializable objects have to be derived from this interface.
///
/// This allows to serialize them to a network stream, to disk etc. Some objects derived from this
/// implement default functions for some of the member functions. This is the case if the class
/// might be used locally only and never be stored to disk.
class ISerializable :
    public base::Interface_declare<0x7a70f2fb,0x1b27,0x416f,0xaa,0x21,0x16,0xc7,0xb4,0xd4,0x1f,0xfc>
{
public:
    /// Returns the class ID of the object.
    ///
    /// The class ID must be unique per registered class. It is used by the serializer to identify
    /// the class during serialization. The deserializer uses the class ID reconstruct a
    /// corresponding class instance during deserialization.
    ///
    /// \return The class ID.
    virtual base::Uuid get_class_id() const = 0;

    /// Serializes the object to the given \p serializer.
    ///
    /// The serialization has to include all sub elements pointed to but serialized together with
    /// this object.
    virtual void serialize( ISerializer* serializer) const = 0;

    /// Deserializes the object from the given \p deserializer.
    ///
    /// The deserialization has to include all sub elements pointed to but serialized together with
    /// this object.
    virtual void deserialize( IDeserializer* deserializer) = 0;
};

/// Target for serializing objects to byte streams.
///
/// The serializer can be used to serialize objects to a byte stream. It is used when serializing
/// objects to disk, to a network connection, etc.
///
/// Arrays of values of a particular type can be serialized with a single call by passing the array
/// size as the \c count parameter. The address of subsequent array elements is obtained by pointer
/// arithmetic.
class ISerializer :
    public base::Interface_declare<0xdcf5a659,0x2b06,0x436b,0x82,0x55,0x36,0x9d,0xbd,0xe7,0x42,0xb1>
{
public:
    /// Writes a serializable object to the serializer.
    ///
    /// \param serializable   The object to serialize.
    /// \return               \c true, unless the class is not registered.
    virtual bool serialize( const ISerializable* serializable) = 0;

    /// Writes values of type bool to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const bool* value, Size count = 1) = 0;

    /// Writes values of type #mi::Uint8 to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Uint8* value, Size count = 1) = 0;

    /// Writes values of type #mi::Uint16 to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Uint16* value, Size count = 1) = 0;

    /// Writes values of type #mi::Size to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Uint32* value, Size count = 1) = 0;

    /// Writes values of type #mi::Uint64 to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Uint64* value, Size count = 1) = 0;

    /// Writes values of type #mi::Sint8 to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Sint8* value, Size count = 1) = 0;

    /// Writes values of type #mi::Sint16 to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Sint16* value, Size count = 1) = 0;

    /// Writes values of type #mi::Sint32 to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Sint32* value, Size count = 1) = 0;

    /// Writes values of type #mi::Sint64 to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Sint64* value, Size count = 1) = 0;

    /// Writes values of type #mi::Float32 to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Float32* value, Size count = 1) = 0;

    /// Writes values of type #mi::Float64 to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Float64* value, Size count = 1) = 0;

    /// Gives a hint to the serializer about the required buffer size.
    ///
    /// Advises the serializer to resize its internal buffer such that it can hold at least
    /// \p capacity additional bytes. This information can be useful to avoid frequent reallocations
    /// that might be necessary if large amounts of data are written.
    virtual void reserve( Size capacity) = 0;

    /// Flushes the so-far serialized data.
    ///
    /// The meaning of \em flushing depends on the context in which the serializer is used. If
    /// flushing is not supported in some context, nothing happens. If flushing is supported in some
    /// context, it typically means to process the already serialized data in the same way as the
    /// entire data at the end of the serialization would have been processed. For example, large
    /// buffers whose content is produced slowly over time can be subdivided into smaller chunks
    /// which can then be processed earlier than the entire buffer.
    virtual void flush() = 0;

    /// Writes values of type #mi::neuraylib::Tag_struct to the serializer.
    ///
    /// \param value   The address of the values to be written.
    /// \param count   The number of values to be written.
    /// \return        \c true (The method does not fail.)
    virtual bool write( const Tag_struct* value, Size count = 1) = 0;
};

/*@}*/ // end group mi_neuray_plugins / mi_neuray_dice

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ISERIALIZER_H
