/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/dice.h
/// \brief \NeurayApiName
///
/// See \ref mi_neuray.

#ifndef MI_DICE_H
#define MI_DICE_H

#include <mi/base.h>
#include <mi/math.h>

#include <mi/neuraylib/assert.h>
#include <mi/neuraylib/dice.h>
#include <mi/neuraylib/factory.h>
#include <mi/neuraylib/http.h>
#include <mi/neuraylib/iallocator.h>
#include <mi/neuraylib/iarray.h>
#include <mi/neuraylib/ibbox.h>
#include <mi/neuraylib/ibridge_client.h>
#include <mi/neuraylib/ibridge_server.h>
#include <mi/neuraylib/ibridge_video_client.h>
#include <mi/neuraylib/ibridge_video_server.h>
#include <mi/neuraylib/ibuffer.h>
#include <mi/neuraylib/icache_manager.h>
#include <mi/neuraylib/icanvas.h>
#include <mi/neuraylib/icanvas_cuda.h>
#include <mi/neuraylib/icluster.h>
#include <mi/neuraylib/icluster_factory.h>
#include <mi/neuraylib/icluster_manager_configuration.h>
#include <mi/neuraylib/icolor.h>
#include <mi/neuraylib/icompound.h>
#include <mi/neuraylib/idata.h>
#include <mi/neuraylib/idatabase.h>
#include <mi/neuraylib/idatabase_configuration.h>
#include <mi/neuraylib/idebug_configuration.h>
#include <mi/neuraylib/ideserializer.h>
#include <mi/neuraylib/idistcache.h>
#include <mi/neuraylib/idynamic_array.h>
#include <mi/neuraylib/ienum.h>
#include <mi/neuraylib/ienum_decl.h>
#include <mi/neuraylib/iextension_api.h>
#include <mi/neuraylib/ifactory.h>
#include <mi/neuraylib/igeneral_configuration.h>
#include <mi/neuraylib/igpu_description.h>
#include <mi/neuraylib/ihost_callback.h>
#include <mi/neuraylib/ihost_properties.h>
#include <mi/neuraylib/iimage_api.h>
#include <mi/neuraylib/ilibrary_authentication.h>
#include <mi/neuraylib/ilogging_configuration.h>
#include <mi/neuraylib/imap.h>
#include <mi/neuraylib/imatrix.h>
#include <mi/neuraylib/inetwork_configuration.h>
#include <mi/neuraylib/inetwork_statistics.h>
#include <mi/neuraylib/ineuray.h>
#include <mi/neuraylib/inode_manager.h>
#include <mi/neuraylib/inumber.h>
#include <mi/neuraylib/iplugin.h>
#include <mi/neuraylib/iplugin_api.h>
#include <mi/neuraylib/iplugin_configuration.h>
#include <mi/neuraylib/ipointer.h>
#include <mi/neuraylib/irdma_context.h>
#include <mi/neuraylib/iref.h>
#include <mi/neuraylib/ischeduling_configuration.h>
#include <mi/neuraylib/iscope.h>
#include <mi/neuraylib/iserializer.h>
#include <mi/neuraylib/ispectrum.h>
#include <mi/neuraylib/istring.h>
#include <mi/neuraylib/istructure.h>
#include <mi/neuraylib/istructure_decl.h>
#include <mi/neuraylib/itile.h>
#include <mi/neuraylib/itile_cuda.h>
#include <mi/neuraylib/itimer_configuration.h>
#include <mi/neuraylib/itransaction.h>
#include <mi/neuraylib/iuser_class.h>
#include <mi/neuraylib/iuser_class_factory.h>
#include <mi/neuraylib/iuuid.h>
#include <mi/neuraylib/ivector.h>
#include <mi/neuraylib/iversion.h>
#include <mi/neuraylib/ivideo_plugin.h>
#include <mi/neuraylib/rtmp.h>
#include <mi/neuraylib/type_traits.h>
#include <mi/neuraylib/typedefs.h>
#include <mi/neuraylib/version.h>

namespace mi
{

/// Namespace for the \neurayApiName.
/// \ingroup mi_neuray
namespace neuraylib
{

/// \defgroup mi_neuray \NeurayApiName
/// \brief \NeurayApiName
///
/// \par Include File:
/// <tt> \#include <mi/dice.h></tt>

} // namespace neuraylib

} // namespace mi

#endif // MI_DICE_H
