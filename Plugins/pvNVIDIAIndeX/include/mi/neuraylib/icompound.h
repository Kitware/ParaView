/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Compounds type, i.e., vectors, matrices, colors, spectrums, and bounding boxes.

#ifndef MI_NEURAYLIB_ICOMPOUND_H
#define MI_NEURAYLIB_ICOMPOUND_H

#include <mi/neuraylib/idata.h>

namespace mi {

/** \defgroup mi_neuray_compounds Compounds
    \ingroup mi_neuray_collections

    This module lists all interfaces and typedefs related to compounds.

    It exist only to split the very high number of interfaces related to types into smaller subsets.
    See \ref mi_neuray_types for an explanation of the type system.
*/

/** \addtogroup mi_neuray_compounds
@{
*/

/// This interface represents compounds, i.e., vectors, matrices, colors, spectrums, and bounding
/// boxes.
///
/// It can be used to represent vectors and matrices by an interface derived from
/// #mi::base::IInterface. In contrast to specialized interfaces for particular vector and matrix
/// types it has a generic interface for vectors and matrices of any size and any primitive type as
/// element type. See \ref mi_neuray_types for description of the type system.
///
/// This interface treats vectors as column vectors, i.e., as matrices with a single column. Colors
/// are treated as vectors of length 3 or 4, spectrums as vectors of length 3, and bounding boxes as
/// matrices of 2 rows and 3 columns.
///
/// In case of vectors and matrices the type name of a compound is the type name of the element
/// followed by \c "<", the size of the compound, and finally \c ">". In case of vectors, the size
/// is specified by a single positive integer. In case of matrices the size is specified as two
/// positive integers separated by a comma. The first and second integer indicate the number of rows
/// and columns, respectively. Examples of valid type names are \c "Sint32<3>" or \c "Float32<2,2>".
/// The type names of the two color types are \c "Color" and \c "Color3", the type name of spectrums
/// is \c "Spectrum", and the type name of bounding boxes is \c "Bbox3".
///
/// The following keys are supported with the methods of #mi::IData_collection are the following:
/// vectors support the keys \c "x", \c "y", \c "z", and \c "w" (or subsets thereof, depending on
/// the dimension). Matrices support the keys \c "xx", \c "xy", \c "xz", \c "xw", \c "yx", \c "yy",
/// \c "yz", \c "yw", \c "zx", \c "zy", \c "zz", \c "zw", \c "wx", \c "wy", \c "wz", and \c "ww" (or
/// subsets thereof, depending on the dimension). Colors support the keys \c "r", \c "g", \c "b",
/// and \c "a" (the type \c "Color3" lacks the \c "a" key). Spectrums support the keys \c "0",
/// \c "1", and \c "2". Finally, bounding boxes support the keys \c "min_x", \c "min_y",
/// \c "min_z", \c "max_x", \c "max_y", and \c "max_z".
///
/// \note
///   Currently the element type is restricted to be either \c bool, #mi::Sint32, #mi::Float32, or
///   #mi::Float64. If used as an attribute, matrices are restricted to #mi::Float32 as element type
///   with the exception of 4 x 4 matrices of elements of type #mi::Float64.
class ICompound : public
    base::Interface_declare<0x65437cd6,0x9727,0x488c,0xa9,0xc5,0x92,0x42,0x43,0xf5,0x5b,0xa0,
                            IData_collection>
{
public:
    /// Returns the number of rows of the represented matrix or vector.
    virtual Size get_number_of_rows() const = 0;

    /// Returns the number of columns of the represented matrix.
    ///
    /// Always returns 1 in case of vectors.
    virtual Size get_number_of_columns() const = 0;

    /// Returns the total number of elements.
    ///
    /// This value is the product of #get_number_of_rows() and #get_number_of_columns().
    virtual Size get_length() const = 0;

    /// Returns the type name of elements of the compound.
    virtual const char* get_element_type_name() const = 0;

    /// Accesses the (\p row, \p column)-th element.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    virtual bool get_value( Size row, Size column, bool& value) const = 0;

    /// Accesses the (\p row, \p column)-th element.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    virtual bool get_value( Size row, Size column, Sint32& value) const = 0;

    /// Accesses the (\p row, \p column)-th element.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    virtual bool get_value( Size row, Size column, Uint32& value) const = 0;

    /// Accesses the (\p row, \p column)-th element.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    virtual bool get_value( Size row, Size column, Float32& value) const = 0;

    /// Accesses the (\p row, \p column)-th element.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    virtual bool get_value( Size row, Size column, Float64& value) const = 0;

    /// Accesses the (\p row, \p column)-th element.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    template<class T>
    T get_value( Size row, Size column) const
    {
        T value;
        bool result = get_value( row, column, value);
        return result ? value : static_cast<T>( 0);
    }

    /// Sets the (\p row, \p column)-th element to \p value.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    virtual bool set_value( Size row, Size column, bool value) = 0;

    /// Sets the (\p row, \p column)-th element to \p value.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    virtual bool set_value( Size row, Size column, Sint32 value) = 0;

    /// Sets the (\p row, \p column)-th element to \p value.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    virtual bool set_value( Size row, Size column, Uint32 value) = 0;

    /// Sets the (\p row, \p column)-th element to \p value.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    virtual bool set_value( Size row, Size column, Float32 value) = 0;

    /// Sets the (\p row, \p column)-th element to \p value.
    ///
    /// \pre \p row < #get_number_of_rows(), \p column < #get_number_of_columns()
    virtual bool set_value( Size row, Size column, Float64 value) = 0;

    /// Accesses the elements of the compound.
    ///
    /// \param values   The values of the compound elements are written to this buffer. The size of
    ///                 the buffer has to be at least #get_length() times \c sizeof(bool).
    virtual void get_values( bool* values) const = 0;

    /// Accesses the elements of the compound.
    ///
    /// \param values   The values of the compound elements are written to this buffer. The size of
    ///                 the buffer has to be at least #get_length() times \c sizeof(Sint32).
    virtual void get_values( Sint32* values) const = 0;

    /// Accesses the elements of the compound.
    ///
    /// \param values   The values of the compound elements are written to this buffer. The size of
    ///                 the buffer has to be at least #get_length() times \c sizeof(Uint32).
    virtual void get_values( Uint32* values) const = 0;

    /// Accesses the elements of the compound.
    ///
    /// \param values   The values of the compound elements are written to this buffer. The size of
    ///                 the buffer has to be at least #get_length() times \c sizeof(Float32).
    virtual void get_values( Float32* values) const = 0;

    /// Accesses the elements of the compound.
    ///
    /// \param values   The values of the compound elements are written to this buffer. The size of
    ///                 the buffer has to be at least #get_length() times \c sizeof(Float64).
    virtual void get_values( Float64* values) const = 0;

    /// Sets the elements of the compound.
    ///
    /// \param values   The new values of the compound elements are read to this buffer. The size of
    ///                 the buffer has to be at least #get_length() times \c sizeof(bool).
    virtual void set_values( const bool* values) = 0;

    /// Sets the elements of the compound.
    ///
    /// \param values   The new values of the compound elements are read to this buffer. The size of
    ///                 the buffer has to be at least #get_length() times \c sizeof(Sint32).
    virtual void set_values( const Sint32* values) = 0;

    /// Sets the elements of the compound.
    ///
    /// \param values   The new values of the compound elements are read to this buffer. The size of
    ///                 the buffer has to be at least #get_length() times \c sizeof(Uint32).
    virtual void set_values( const Uint32* values) = 0;

    /// Sets the elements of the compound.
    ///
    /// \param values   The new values of the compound elements are read to this buffer. The size of
    ///                 the buffer has to be at least #get_length() times \c sizeof(Float32).
    virtual void set_values( const Float32* values) = 0;

    /// Sets the elements of the compound.
    ///
    /// \param values   The new values of the compound elements are read to this buffer. The size of
    ///                 the buffer has to be at least #get_length() times \c sizeof(Float64).
    virtual void set_values( const Float64* values) = 0;

    using IData_collection::get_value;

    using IData_collection::set_value;
};

/*@}*/ // end group mi_neuray_compounds

} // namespace mi

#endif // MI_NEURAYLIB_ICOMPOUND_H
