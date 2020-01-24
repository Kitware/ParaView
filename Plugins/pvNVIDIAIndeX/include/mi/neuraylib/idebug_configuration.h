/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component for debugging settings.

#ifndef MI_NEURAYLIB_IDEBUG_CONFIGURATION_H
#define MI_NEURAYLIB_IDEBUG_CONFIGURATION_H

#include <mi/base/interface_declare.h>

namespace mi {

class IString;

namespace neuraylib {

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface represents an interface to set debug options.
class IDebug_configuration : public 
    mi::base::Interface_declare<0x7938887b,0x57c6,0x422f,0x84,0x03,0xdc,0x06,0xf2,0x26,0xd6,0x04>
{
public:
    /// Sets a particular debug option.
    ///
    /// \param option    The option to be set in the form \c key=value.
    /// \return
    ///                  -  0: Success.
    ///                  - -1: The option could not be successfully parsed. This happens for example
    ///                        if the option is not of the form key=value.
    virtual Sint32 set_option( const char* option) = 0;

    /// Returns the value of a particular debug option.
    ///
    /// \param key       The key of the debug option.
    /// \return          The value of the debug option, or \c NULL if the option is not set.
    virtual const IString* get_option( const char* key) const = 0;
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IDEBUG_CONFIGURATION_H
