/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Spectrum type.

#ifndef MI_NEURAYLIB_ISPECTRUM_H
#define MI_NEURAYLIB_ISPECTRUM_H

#include <mi/math/spectrum.h>
#include <mi/neuraylib/icompound.h>
#include <mi/neuraylib/typedefs.h>

namespace mi {

/** \addtogroup mi_neuray_compounds
@{
*/

/// This interface represents spectrums.
///
/// It can be used to represent spectrums by an interface derived from #mi::base::IInterface.
///
/// \see #mi::Spectrum_struct
class ISpectrum :
    public base::Interface_declare<0x127293dc,0x1fad,0x4df5,0x94,0x38,0xe3,0x48,0xda,0x7b,0x8c,0xf6,
                                   ICompound>
{
public:
    /// Returns the spectrum represented by this interface.
    virtual Spectrum_struct get_value() const = 0;

    /// Returns the spectrum represented by this interface.
    virtual void get_value( Spectrum_struct& value) const = 0;

    /// Sets the spectrum represented by this interface.
    virtual void set_value( const Spectrum_struct& value) = 0;

    using ICompound::get_value;

    using ICompound::set_value;
};

/*@}*/ // end group mi_neuray_compounds

} // namespace mi

#endif // MI_NEURAYLIB_ISPECTRUM_H
