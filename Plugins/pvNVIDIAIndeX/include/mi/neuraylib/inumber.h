/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Numeric types.

#ifndef MI_NEURAYLIB_INUMBER_H
#define MI_NEURAYLIB_INUMBER_H

#include <mi/neuraylib/idata.h>

namespace mi {

/** \addtogroup mi_neuray_simple_types
@{
*/

/// This interface represents simple numeric types.
///
/// The methods get_value() and set_value() are overloaded for the various numeric types.
/// If necessary a conversion as defined by the C/C++ standard is performed.
class INumber :
    public base::Interface_declare<0x07366a82,0x3d0c,0x46e9,0x88,0x0e,0xed,0x65,0xba,0xde,0xef,0x2b,
                                   IData_simple>
{
public:
    /// Returns the value of the object as bool.
    virtual void get_value( bool& val) const = 0;

    /// Sets the value of the object via a parameter of type bool.
    virtual void set_value( bool val) = 0;

    /// Returns the value of the object as #mi::Uint8.
    virtual void get_value( Uint8& val) const = 0;

    /// Sets the value of the object via a parameter of type #mi::Uint8.
    virtual void set_value( Uint8 val) = 0;

    /// Returns the value of the object as #mi::Uint16.
    virtual void get_value( Uint16& val) const = 0;

    /// Sets the value of the object via a parameter of type #mi::Uint16.
    virtual void set_value( Uint16 val) = 0;

    /// Returns the value of the object as #mi::Uint32.
    virtual void get_value( Uint32& val) const = 0;

    /// Sets the value of the object via a parameter of type #mi::Uint32.
    virtual void set_value( Uint32 val) = 0;

    /// Returns the value of the object as #mi::Uint64.
    virtual void get_value( Uint64& val) const = 0;

    /// Sets the value of the object via a parameter of type #mi::Uint64.
    virtual void set_value( Uint64 val) = 0;

    /// Returns the value of the object as #mi::Sint8.
    virtual void get_value( Sint8& val) const = 0;

    /// Sets the value of the object via a parameter of type #mi::Sint8.
    virtual void set_value( Sint8 val) = 0;

    /// Returns the value of the object as #mi::Sint16.
    virtual void get_value( Sint16& val) const = 0;

    /// Sets the value of the object via a parameter of type #mi::Sint16.
    virtual void set_value( Sint16 val) = 0;

    /// Returns the value of the object as #mi::Sint32.
    virtual void get_value( Sint32& val) const = 0;

    /// Sets the value of the object via a parameter of type #mi::Sint32.
    virtual void set_value( Sint32 val) = 0;

    /// Returns the value of the object as #mi::Sint64.
    virtual void get_value( Sint64& val) const = 0;

    /// Sets the value of the object via a parameter of type #mi::Sint64.
    virtual void set_value( Sint64 val) = 0;

    /// Returns the value of the object as #mi::Float32.
    virtual void get_value( Float32& val) const = 0;

    /// Sets the value of the object via a parameter of type #mi::Float32.
    virtual void set_value( Float32 val) = 0;

    /// Returns the value of the object as #mi::Float64.
    virtual void get_value( Float64& val) const = 0;

    /// Sets the value of the object via a parameter of type #mi::Float64.
    virtual void set_value( Float64 val) = 0;

    /// Returns the value of the object.
    ///
    /// The type of the object represented by the interface is indicated by the template parameter.
    ///
    /// This templated member function is a wrapper of the other functions of the same name for the
    /// user's convenience. It allows you to write
    /// \code
    /// mi::Uint32 x = ivalue->get_value<mi::Uint32>();
    /// \endcode
    /// instead of
    /// \code
    /// mi::Uint32 x;
    /// ivalue->get_value( x)
    /// \endcode
    template<class T>
    T get_value() const
    {
        T value;
        get_value( value);
        return value;
    }
};

/// This interface represents \c bool.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
class IBoolean :
    public base::Interface_declare<0x121441c4,0xdf23,0x44f7,0xbb,0x34,0xd6,0xa8,0x24,0x66,0x6f,0x84,
                                   INumber>
{
};

/// This interface represents #mi::Uint8.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Uint8
class IUint8 :
    public base::Interface_declare<0x1ac0f46d,0x0b99,0x4228,0x82,0x9c,0x7c,0xc7,0x3c,0xe2,0x99,0x4a,
                                   INumber>
{
};

/// This interface represents #mi::Uint16.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Uint16
class IUint16 :
    public base::Interface_declare<0x30139db0,0x6539,0x48b3,0x8f,0xe0,0xf8,0x8b,0x74,0x10,0x9d,0x97,
                                   INumber>
{
};

/// This interface represents #mi::Uint32.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Uint32
class IUint32 :
    public base::Interface_declare<0x4504ecf0,0x7cb3,0x4396,0xa6,0x78,0xea,0xbe,0xf5,0x48,0x84,0x58,
                                   INumber>
{
};

/// This interface represents #mi::Uint64.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Uint64
class IUint64 :
    public base::Interface_declare<0x736a2345,0xd6d7,0x4681,0x80,0xd4,0xaf,0x74,0xf7,0x54,0x39,0x13,
                                   INumber>
{
};

/// This interface represents #mi::Sint8.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Sint8
class ISint8 :
    public base::Interface_declare<0x800b88cc,0xe9ac,0x4c47,0x8c,0x5c,0x11,0x8c,0x89,0x79,0x07,0x56,
                                   INumber>
{
};

/// This interface represents #mi::Sint16.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Sint16
class ISint16 :
    public base::Interface_declare<0x950c56be,0x37be,0x4be3,0x87,0xe9,0xf2,0x63,0xe4,0xa3,0xbf,0x02,
                                   INumber>
{
};

/// This interface represents #mi::Sint32.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Sint32
class ISint32 :
    public base::Interface_declare<0x9a756f1c,0x3733,0x4230,0xa9,0x36,0x2e,0x5b,0x57,0xf3,0x4b,0x09,
                                   INumber>
{
};

/// This interface represents #mi::Sint64.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Sint64
class ISint64 :
    public base::Interface_declare<0x9b84869f,0x3ac6,0x4a93,0x93,0x68,0x37,0x45,0x6c,0xd2,0xe3,0x34,
                                   INumber>
{
};

/// This interface represents #mi::Float32.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Float32
class IFloat32 :
    public base::Interface_declare<0xd12231d8,0x9d61,0x4fa1,0xb6,0xca,0xdc,0x2a,0xb2,0x7e,0x54,0xbd,
                                   INumber>
{
};

/// This interface represents #mi::Float64.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Float64
class IFloat64 :
    public base::Interface_declare<0xd3a0571b,0x2b7b,0x4c20,0xbf,0xbe,0xbb,0xe0,0xe7,0xa6,0x05,0x08,
                                   INumber>
{
};

/// This interface represents #mi::Size.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Size
class ISize :
    public base::Interface_declare<0xf86edd31,0x01fa,0x4b66,0x8f,0xe2,0x8b,0xc4,0xf6,0x1b,0x16,0xce,
                                   INumber>
{
};

/// This interface represents #mi::Difference.
///
/// It does not have specific methods, see #mi::INumber and its parent interfaces.
///
/// \see #mi::Difference
class IDifference :
    public base::Interface_declare<0xfbff3d24,0x06a1,0x4031,0x85,0xd9,0x83,0x94,0xc0,0x6b,0x4d,0xae,
                                   INumber>
{
};

/*@}*/ // end group mi_neuray_simple_types

} // namespace mi

#endif // MI_NEURAYLIB_INUMBER_H
