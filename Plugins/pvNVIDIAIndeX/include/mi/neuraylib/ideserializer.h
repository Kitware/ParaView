/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief      Deserialization of objects from a byte stream.

#ifndef MI_NEURAYLIB_IDESERIALIZER_H
#define MI_NEURAYLIB_IDESERIALIZER_H

#include <mi/base/interface_declare.h>
#include <mi/neuraylib/iserializer.h>

namespace mi {

namespace neuraylib {

/** 
\if IRAY_API \addtogroup mi_neuray_plugins
\elseif MDL_SOURCE_RELEASE \addtogroup mi_neuray_plugins
\else \addtogroup mi_neuray_dice
\endif
@{
*/

/// Source for deserializing objects from byte streams.
///
/// The deserializer can be used to deserialize objects from a byte stream. It is used when
/// deserializing objects from disk, from a network connection, etc.
///
/// Arrays of values of a particular type can be deserialized with a single call by passing the
/// array size as the \c count parameter. The address of subsequent array elements is obtained by
/// pointer arithmetic.
class IDeserializer :
    public base::Interface_declare<0x7052258a,0x579b,0x4861,0x9c,0x12,0x5f,0x02,0x9c,0x86,0xfb,0xce>
{
public:
    /// Reads a serializable object from the deserializer.
    ///
    /// \return        The deserialized object.
    virtual ISerializable* deserialize() = 0;

    /// Reads a serializable object from the serializer.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \tparam T     The interface type of the element to return
    /// \return       The deserialized object, or \c NULL if the element is not of type \c T.
    template <class T>
    T* deserialize()
    {
        ISerializable* ptr_iserializable = deserialize();
        if ( !ptr_iserializable)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iserializable->get_interface( typename T::IID()));
        ptr_iserializable->release();
        return ptr_T;
    }

    /// Reads values of type bool from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( bool* value, Size count = 1) = 0;

    /// Reads values of type #mi::Uint8 from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Uint8* value, Size count = 1) = 0;

    /// Reads values of type #mi::Uint16 from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Uint16* value, Size count = 1) = 0;

    /// Reads values of type #mi::Size from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Uint32* value, Size count = 1) = 0;

    /// Reads values of type #mi::Uint64 from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Uint64* value, Size count = 1) = 0;

    /// Reads values of type #mi::Sint8 from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Sint8* value, Size count = 1) = 0;

    /// Reads values of type #mi::Sint16 from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Sint16* value, Size count = 1) = 0;

    /// Reads values of type #mi::Sint32 from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Sint32* value, Size count = 1) = 0;

    /// Reads values of type #mi::Sint64 from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Sint64* value, Size count = 1) = 0;

    /// Reads values of type #mi::Float32 from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Float32* value, Size count = 1) = 0;

    /// Reads values of type #mi::Float64 from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Float64* value, Size count = 1) = 0;

    /// Reads values of type #mi::neuraylib::Tag_struct from the deserializer.
    ///
    /// \param value   The address of the values to be read.
    /// \param count   The number of values to be read.
    /// \return        \c true (The method does not fail.)
    virtual bool read( Tag_struct* value, Size count = 1) = 0;
};

/*@}*/ // end group mi_neuray_plugins / mi_neuray_dice

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IDESERIALIZER_H
