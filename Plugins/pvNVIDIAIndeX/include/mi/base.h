/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base.h
/// \brief Base API.
///
/// See \ref mi_base.

#ifndef MI_BASE_H
#define MI_BASE_H

#include <mi/base/assert.h>
#include <mi/base/atom.h>
#include <mi/base/condition.h>
#include <mi/base/config.h>
#include <mi/base/default_allocator.h>
#include <mi/base/enums.h>
#include <mi/base/handle.h>
#include <mi/base/iallocator.h>
#include <mi/base/ilogger.h>
#include <mi/base/iinterface.h>
#include <mi/base/interface_declare.h>
#include <mi/base/interface_implement.h>
#include <mi/base/interface_merger.h>
#include <mi/base/lock.h>
#include <mi/base/plugin.h>
#include <mi/base/std_allocator.h>
#include <mi/base/types.h>
#include <mi/base/uuid.h>
#include <mi/base/version.h>

/// Common namespace for APIs of NVIDIA Advanced Rendering Center GmbH.
/// \ingroup mi_base
namespace mi {

/// Namespace for the Base API.
/// \ingroup mi_base
namespace base {

/// \defgroup mi_base Base API
/// \brief Basic types, configuration, and assertion support.
///
/// \par Include File:
/// <tt> \#include <mi/base.h></tt>

} // namespace base

} // namespace mi

#endif // MI_BASE_H
