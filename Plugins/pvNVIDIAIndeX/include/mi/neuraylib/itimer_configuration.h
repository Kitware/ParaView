/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component that allows to start timers to elapse at configured timer
///        times triggering callbacks.

#ifndef MI_NEURAYLIB_ITIMER_CONFIGURATION_H
#define MI_NEURAYLIB_ITIMER_CONFIGURATION_H

#include <mi/base/interface_declare.h>
#include <mi/base/interface_implement.h>
#include <mi/base/handle.h>
#include <mi/neuraylib/iserializer.h>

namespace mi {

namespace neuraylib {

class ISerializable;

/** \addtogroup mi_neuray_configuration
@{
*/

/// This is an abstract interface class for a timer.
///
/// Its aim is to be derived from by the application writer. The concrete implementation can then be
/// registered with the \neurayApiName and will after that be used by \neurayLibraryName.
///
/// The timer class must be serializable since a timer will potentially be sent over the cluster to
/// different nodes. If the timer class is only ever used locally, then the serialization function
/// can be left empty.
///
/// \note Instances of this interface should not be created on the stack, since this might lead
/// to premature destruction of such instances while still being in use by the \neurayLibraryName.
class ITimer : public
    mi::base::Interface_declare<0x654218a4,0xc9af,0x4c4c,0xba,0x9a,0x3a,0x16,0x8a,0xdd,0x8a,0x0f,
    neuraylib::ISerializable>
{
public:
    /// This function is called when the timer elapses.
    ///
    /// Note that the function will be called from a separate thread, not from the thread that
    /// was used to start the timer.
    virtual void timer_callback() = 0;
};

/// The registration of a timer class requires a factory which constructs an instance during
/// deserialization. In most cases it should be sufficient to use the supplied default
/// implementation in #mi::neuraylib::Timer_class_factory.
class ITimer_class_factory : public
    mi::base::Interface_declare<0xed2c17d1,0xdaf0,0x4aa2,0x95,0xaf,0xfc,0x1b,0x6f,0x6a,0xf6,0x2b>
{
public:
    /// Creates an instance of the class for which the factory was registered.
    /// \return              An instance of the class, or \c NULL on failure.
    virtual ITimer* create() = 0;
};

/// This mixin class provides a default implementation of the #mi::neuraylib::ITimer_class_factory
/// interface.
template <class T>
class Timer_class_factory : public
    mi::base::Interface_implement<neuraylib::ITimer_class_factory>
{
public:
    /// Creates an instance of the class for which the factory was registered.
    ///
    /// This default implementation simply calls the default constructor of T without arguments.
    ///
    /// \return              An instance of the class, or \c NULL on failure.
    ITimer* create() { return new T; }
};

/// This class is used to start and stop timers.
class ITimer_configuration : public
    mi::base::Interface_declare<0xedad8937,0x015e,0x4ee7,0x9e,0x49,0x6a,0x22,0x83,0xbd,0xc4,0x41>
{
public:
    /// Starts a cluster timer to elapse once.
    ///
    /// The timer is either executed locally only or scheduled to be executed on one host in the
    /// cluster chosen by the timer system. In the latter case the timer system will guarantee that
    /// the timer will be executed even if the host starting it left the cluster before the timer
    /// elapsed.
    ///
    /// If a named timer is started which is already running then the \em later time of the running
    /// and newly started timer will prevail. That is if a timer is running and would elapse after
    /// two seconds and a new #start_timer() call would set it to elapse after one second, then it
    /// will elapse after two seconds. If that is not the desired behavior the timer must be stopped
    /// before starting it with a new value.
    ///
    /// If timers are not running locally then the execution of the timer will be delayed by the
    /// time necessary to transport the timer to the remote host.
    ///
    /// If a timer is to be executed remotely then it must be registered with the
    /// #register_timer_class() function before starting it.
    ///
    /// This method can only be used while \neurayProductName is running.
    ///
    /// \param name     The name of the timer, used to identify the timer in the cluster.
    /// \param timer    The timer which holds the callback to be called when the timer elapses.
    /// \param delay    Time in seconds after which #mi::neuraylib::ITimer::timer_callback() will be
    ///                 called.
    /// \param local    Specifies if this is a timer which is running in the cluster (default) or
    ///                 only on the local node.
    /// \return
    ///                 -  0: Success.
    ///                 - -1: Invalid parameters (\c NULL pointer).
    ///                 - -2: The timer class is not registered.
    ///                 - -3: The method cannot be called at this point of time.
    virtual Sint32 start_timer(
        const char* name, ITimer* timer, Float64 delay, bool local = false) = 0;

    /// Stops a timer.
    ///
    /// \param name     The name of the timer to be stopped.
    /// \return
    ///                 -  0: Success.
    ///                 - -1: Invalid parameters (e.g. unknown name).
    virtual Sint32 stop_timer( const char* name) = 0;

    /// Registers a timer class with the timer configuration.
    //
    /// Registration is strictly necessary for non-local timers.
    ///
    /// \param class_id     The class ID of the class that shall be registered for serialization.
    /// \param factory      The class factory.
    ///
    /// \return             \c true if the class of was successfully registered
    ///                     for serialization, and \c false otherwise.
    virtual bool register_timer_class( base::Uuid class_id, ITimer_class_factory* factory) = 0;

    /// Registers a timer class with the timer configuration.
    //
    /// Registration is strictly necessary for non-local timers.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It uses \c T::IID() as UUID and the default class factory
    /// #mi::neuraylib::Timer_class_factory specialized for T.
    ///
    /// \return             \c true if the class of was successfully registered
    ///                     for serialization, and \c false otherwise.
    template <class T>
    bool register_timer_class()
    {
        mi::base::Handle<ITimer_class_factory> factory( new Timer_class_factory<T>());
        return register_timer_class( typename T::IID(), factory.get());
    }
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ITIMER_CONFIGURATION_H
