/* Copyright 2020 NVIDIA Corporation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string>

#include "vtkMultiProcessController.h"
#include "vtkTimerLog.h"

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_sparse_volume_importer.h"

static const std::string LOG_svol_rvol_prefix = "Sparse volume importer: ";

//-------------------------------------------------------------------------------------------------
inline nv::index::Sparse_volume_voxel_format match_volume_format(const std::string& fmt_string)
{
  if (fmt_string == "char")
  {
    return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT8;
  }
  else if (fmt_string == "unsigned char")
  {
    return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT8;
  }
  else if (fmt_string == "unsigned short")
  {
    return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT16;
  }
  else if (fmt_string == "short")
  {
    return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT16;
  }
  else if (fmt_string == "float" || fmt_string == "double")
  {
    return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32;
  }

  return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_COUNT; // invalid format
}

//-------------------------------------------------------------------------------------------------
inline mi::Size volume_format_size(const nv::index::Sparse_volume_voxel_format fmt)
{
  switch (fmt)
  {
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT8:
      return sizeof(mi::Uint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT8:
      return sizeof(mi::Sint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT16:
      return sizeof(mi::Uint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT16:
      return sizeof(mi::Sint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32:
      return sizeof(mi::Float32);
    default:
      return 0;
  }
}

//-------------------------------------------------------------------------------------------------
vtknvindex_import_bricks::vtknvindex_import_bricks(
  const nv::index::ISparse_volume_subset_data_descriptor* subset_data_descriptor,
  nv::index::ISparse_volume_subset* volume_subset, const mi::Uint8* source_buffer,
  mi::Size vol_fmt_size, mi::Sint32 border_size, mi::Sint32 ghost_levels,
  const vtknvindex::util::Bbox3i& source_bbox, const vtknvindex_volume_neighbor_data* neighbor_data)
  : m_subset_data_descriptor(subset_data_descriptor)
  , m_volume_subset(volume_subset)
  , m_source_buffer(source_buffer)
  , m_vol_fmt_size(vol_fmt_size)
  , m_border_size(border_size)
  , m_ghost_levels(ghost_levels)
  , m_source_bbox(source_bbox)
  , m_neighbor_data(neighbor_data)
{
  m_nb_bricks = subset_data_descriptor->get_subset_number_of_data_bricks();
  m_nb_fragments = vtknvindex_sysinfo::get_sysinfo()->get_number_logical_cpu();

  // Use default if number of CPUs could not be determinated
  if (m_nb_fragments == 0)
  {
    const mi::Size DEFAULT_NB_FRAGMENTS = 8;
    m_nb_fragments = DEFAULT_NB_FRAGMENTS;
  }

  if (m_nb_fragments > m_nb_bricks)
    m_nb_fragments = m_nb_bricks;
}

namespace
{

inline mi::Size calc_offset(mi::Uint32 x, mi::Uint32 y, mi::Uint32 z,
  const mi::math::Vector<mi::Uint32, 3>& buffer_dims, mi::Size fmt_size)
{
  const mi::Size offset = static_cast<mi::Size>(x) + static_cast<mi::Size>(y) * buffer_dims.x +
    static_cast<mi::Size>(z) * buffer_dims.x * buffer_dims.y;
  return offset * fmt_size;
}

// Duplicate border voxels of src_bbox towards dst_bbox, effectively clamping the sub-volume. Voxels
// outside of dst_bbox and voxels inside of src_bbox will stay unchanged.
//
// brick_bbox_global: bbox of the entire destination brick (volume coords)
// src_bbox_global: data already written to the brick (volume coords)
// dst_bbox_global: data that should be filled, clamping to src_bbox_global
void clamp_to_border(const mi::math::Bbox<mi::Sint32, 3>& brick_bbox_global,
  const mi::math::Bbox<mi::Sint32, 3>& src_bbox_global,
  const mi::math::Bbox<mi::Sint32, 3>& dst_bbox_global, mi::Uint8* brick_buffer,
  mi::Size voxel_fmt_size)
{
  using namespace vtknvindex::util;

  if (src_bbox_global == brick_bbox_global)
  {
    return;
  }

  const Vec3u brick_dims(brick_bbox_global.extent());

  // Convert to brick coordinates and clamp
  const mi::math::Bbox<mi::Uint32, 3> src_bbox(
    Vec3u(
      mi::math::clamp(src_bbox_global.min - brick_bbox_global.min, Vec3i(0), Vec3i(brick_dims))),
    Vec3u(
      mi::math::clamp(src_bbox_global.max - brick_bbox_global.min, Vec3i(0), Vec3i(brick_dims))));

  const mi::math::Bbox<mi::Uint32, 3> dst_bbox(
    Vec3u(
      mi::math::clamp(dst_bbox_global.min - brick_bbox_global.min, Vec3i(0), Vec3i(brick_dims))),
    Vec3u(
      mi::math::clamp(dst_bbox_global.max - brick_bbox_global.min, Vec3i(0), Vec3i(brick_dims))));

  // Sanity check
  if (!dst_bbox.is_volume() || !src_bbox.is_volume() || dst_bbox == src_bbox)
  {
    return;
  }

  // Clamp to border in x-direction
  for (mi::Uint32 z = src_bbox.min.z; z < src_bbox.max.z; ++z)
  {
    for (mi::Uint32 y = src_bbox.min.y; y < src_bbox.max.y; ++y)
    {
      for (mi::Uint32 x = dst_bbox.min.x; x < src_bbox.min.x; ++x)
      {
        const mi::Size src = calc_offset(src_bbox.min.x, y, z, brick_dims, voxel_fmt_size);
        const mi::Size dst = calc_offset(x, y, z, brick_dims, voxel_fmt_size);
        memcpy(brick_buffer + dst, brick_buffer + src, voxel_fmt_size);
      }

      for (mi::Uint32 x = src_bbox.max.x; x < dst_bbox.max.x; ++x)
      {
        const mi::Size src = calc_offset(src_bbox.max.x - 1, y, z, brick_dims, voxel_fmt_size);
        const mi::Size dst = calc_offset(x, y, z, brick_dims, voxel_fmt_size);
        memcpy(brick_buffer + dst, brick_buffer + src, voxel_fmt_size);
      }
    }
  }

  // Length of a run in x-direction
  const mi::Size len_x = dst_bbox.max.x - dst_bbox.min.x;

  // Clamp to border in y-direction (including previously duplicated voxels in x)
  for (mi::Uint32 z = src_bbox.min.z; z < src_bbox.max.z; ++z)
  {

    for (mi::Uint32 y = dst_bbox.min.y; y < src_bbox.min.y; ++y)
    {
      const mi::Size src =
        calc_offset(dst_bbox.min.x, src_bbox.min.y, z, brick_dims, voxel_fmt_size);
      const mi::Size dst = calc_offset(dst_bbox.min.x, y, z, brick_dims, voxel_fmt_size);
      memcpy(brick_buffer + dst, brick_buffer + src, len_x * voxel_fmt_size);
    }

    for (mi::Uint32 y = src_bbox.max.y; y < dst_bbox.max.y; ++y)
    {
      const mi::Size src =
        calc_offset(dst_bbox.min.x, src_bbox.max.y - 1, z, brick_dims, voxel_fmt_size);
      const mi::Size dst = calc_offset(dst_bbox.min.x, y, z, brick_dims, voxel_fmt_size);
      memcpy(brick_buffer + dst, brick_buffer + src, len_x * voxel_fmt_size);
    }
  }

  // Clamp to border in z-direction (including previously duplicated voxels in x and y)
  for (mi::Uint32 z = dst_bbox.min.z; z < src_bbox.min.z; ++z)
  {
    for (mi::Uint32 y = dst_bbox.min.y; y < dst_bbox.max.y; ++y)
    {
      const mi::Size src =
        calc_offset(dst_bbox.min.x, y, src_bbox.min.z, brick_dims, voxel_fmt_size);
      const mi::Size dst = calc_offset(dst_bbox.min.x, y, z, brick_dims, voxel_fmt_size);
      memcpy(brick_buffer + dst, brick_buffer + src, len_x * voxel_fmt_size);
    }
  }

  for (mi::Uint32 z = src_bbox.max.z; z < dst_bbox.max.z; ++z)
  {
    for (mi::Uint32 y = dst_bbox.min.y; y < dst_bbox.max.y; ++y)
    {
      const mi::Size src =
        calc_offset(dst_bbox.min.x, y, src_bbox.max.z - 1, brick_dims, voxel_fmt_size);
      const mi::Size dst = calc_offset(dst_bbox.min.x, y, z, brick_dims, voxel_fmt_size);
      memcpy(brick_buffer + dst, brick_buffer + src, len_x * voxel_fmt_size);
    }
  }
}

} // namespace

//-------------------------------------------------------------------------------------------------

// Handle clamping against the outside border of the volume in a separated step and not in the main
// copying loop. This is required to support border handling with neighboring pieces without VTK
// ghosting.
#define VTKNVINDEX_CLAMP_SEPARATELY

void vtknvindex_import_bricks::execute_fragment(
  mi::neuraylib::IDice_transaction* /*dice_transaction*/, mi::Size index, mi::Size /*count*/,
  const mi::neuraylib::IJob_execution_context* /*context*/)
{
  using namespace nv::index;
  using namespace vtknvindex::util;

  const mi::Uint32 svol_attrib_index_0 = 0u;
  mi::Size nb_bricks_per_index = m_nb_bricks / m_nb_fragments;
  if (m_nb_bricks % m_nb_fragments > 0)
    nb_bricks_per_index++;

  mi::Uint32 min_brick_idx = static_cast<mi::Uint32>(index * nb_bricks_per_index);
  mi::Uint32 max_brick_idx = static_cast<mi::Uint32>((index + 1) * nb_bricks_per_index);
  if (max_brick_idx > m_nb_bricks)
    max_brick_idx = m_nb_bricks;

  for (mi::Uint32 brick_idx = min_brick_idx; brick_idx < max_brick_idx; brick_idx++)
  {
    const ISparse_volume_subset_data_descriptor::Data_brick_info brick_info =
      m_subset_data_descriptor->get_subset_data_brick_info(brick_idx);
    const ISparse_volume_subset::Data_brick_buffer_info brick_data_info =
      m_volume_subset->access_brick_data_buffer(brick_idx, svol_attrib_index_0);

    mi::Uint8* svol_brick_data_raw = reinterpret_cast<mi::Uint8*>(brick_data_info.data);
    if (svol_brick_data_raw == nullptr)
    {
      ERROR_LOG << LOG_svol_rvol_prefix
                << "Error accessing brick data pointer, brick id: " << brick_idx;
      return;
    }

    // Bounding box of the entire volume without outside border
    const Bbox3i entire_volume_bbox = m_subset_data_descriptor->get_dataset_lod_level_box(0);

    // Source: Bounding box of the source buffer (may include internal border/ghosting, but not
    // outside
    // boundary around the entire volume)
    const Vec3i source_dims = m_source_bbox.extent();

    // Destination: Bounding box of the brick where data will be written to (includes border
    const Vec3u dest_brick_dims = m_subset_data_descriptor->get_subset_data_brick_dimensions();
    const Vec3i dest_brick_position(brick_info.brick_position);
    const Bbox3i dest_brick_bbox(dest_brick_position, dest_brick_position + Vec3i(dest_brick_dims));

    const Bbox3i dest_brick_bbox_clipped_without_border(
      mi::math::clamp(dest_brick_bbox.min, m_source_bbox.min, m_source_bbox.max),
      mi::math::clamp(dest_brick_bbox.max, m_source_bbox.min, m_source_bbox.max));

    Bbox3i source_bbox_with_border_inner(m_source_bbox);
    for (mi::Uint32 i = 0; i < 3; ++i)
    {
      // Add inner border required by IndeX (adjusting for existing ghosting), but not outside
      // border
      const mi::Sint32 remaining_border = std::max(0, m_border_size - m_ghost_levels);
      source_bbox_with_border_inner.min[i] = std::max(
        source_bbox_with_border_inner.min[i] - remaining_border, entire_volume_bbox.min[i]);
      source_bbox_with_border_inner.max[i] = std::min(
        source_bbox_with_border_inner.max[i] + remaining_border, entire_volume_bbox.max[i]);
    }

    const Bbox3i dest_brick_bbox_clipped_with_border_inner(
      mi::math::clamp(
        dest_brick_bbox.min, source_bbox_with_border_inner.min, source_bbox_with_border_inner.max),
      mi::math::clamp(
        dest_brick_bbox.max, source_bbox_with_border_inner.min, source_bbox_with_border_inner.max));

    Bbox3i source_bbox_with_border_all(source_bbox_with_border_inner);
    for (mi::Uint32 i = 0; i < 3; ++i)
    {
      // Also add outside border (which is never covered by ghosting)
      if (source_bbox_with_border_all.min[i] == entire_volume_bbox.min[i])
        source_bbox_with_border_all.min[i] -= m_border_size;

      if (source_bbox_with_border_all.max[i] == entire_volume_bbox.max[i])
        source_bbox_with_border_all.max[i] += m_border_size;
    }

    // Defines what will be read from the source. If larger than the source bbox, then data will be
    // clamped/duplicated. This typically happens for outside border voxels or interior boundaries
    // when ghosting is not enabled.
    const Bbox3i dest_brick_bbox_clipped_with_border(
      mi::math::clamp(
        dest_brick_bbox.min, source_bbox_with_border_all.min, source_bbox_with_border_all.max),
      mi::math::clamp(
        dest_brick_bbox.max, source_bbox_with_border_all.min, source_bbox_with_border_all.max));

#ifdef VTKNVINDEX_CLAMP_SEPARATELY
    // Border clamping will be handled separately, use bbox without border in the main loop
    const Bbox3i dest_brick_bbox_clipped = dest_brick_bbox_clipped_without_border;
#else
    // Handle the border in the main loop
    const Bbox3i dest_brick_bbox_clipped = dest_brick_bbox_clipped_with_border;
#endif // VTKNVINDEX_CLAMP_SEPARATELY

    // Translate to local coordinates of the source buffer. Voxels with coordinates outside of
    // [0, source_dims - 1] will be clamped.
    const Bbox3i read_bbox(dest_brick_bbox_clipped.min - m_source_bbox.min,
      dest_brick_bbox_clipped.max - m_source_bbox.min);

    // Offset between start position in destination and source buffers
    const Vec3i dest_delta(m_source_bbox.min - dest_brick_position);

#if 0
    ERROR_LOG << "source_bbox=" << m_source_bbox << ", source_dims=" << source_dims
              << ", entire_volume_bbox=" << entire_volume_bbox
              << ", dest_brick_dims=" << dest_brick_dims << ", brick_bbox=" << dest_brick_bbox
              << ", read_bbox=" << read_bbox << ", read_in_full_bbox="
              << Bbox3i(read_bbox.min + m_source_bbox.min, read_bbox.max + m_source_bbox.min)
              << ", write_in_brick_bbox="
              << Bbox3i(read_bbox.min + dest_delta, read_bbox.max + dest_delta)
              << ", write_in_full_bbox=" << Bbox3i(read_bbox.min + dest_delta + dest_brick_position,
                                              read_bbox.max + dest_delta + dest_brick_position)
              << ", dest_delta=" << dest_delta;
#endif

    for (mi::Sint32 z = read_bbox.min.z; z < read_bbox.max.z; ++z)
    {
      // Clamp access outside of source bbox, potentially clamping border voxels
      const mi::Sint32 zz = mi::math::clamp(z, 0, source_dims.z - 1);
      const mi::Size src_off_z = static_cast<mi::Size>(zz) * source_dims.x * source_dims.y;

      const mi::Size dst_off_z =
        static_cast<mi::Size>(z + dest_delta.z) * dest_brick_dims.x * dest_brick_dims.y;

      for (mi::Sint32 y = read_bbox.min.y; y < read_bbox.max.y; ++y)
      {
        // Clamp access outside of source bbox, potentially clamping border voxels
        const mi::Sint32 yy = mi::math::clamp(y, 0, source_dims.y - 1);
        const mi::Size src_off_yz = static_cast<mi::Size>(yy) * source_dims.x + src_off_z;

        const mi::Size dst_off_yz =
          static_cast<mi::Size>(y + dest_delta.y) * dest_brick_dims.x + dst_off_z;

        // Clamp access outside of source bbox, potentially clamping border voxels.
        //
        // Not using "source_dims.x - 1" here since xmax is not used as an index but just to compute
        // the number of voxels to copy.
        const mi::Sint32 xmin = mi::math::clamp(read_bbox.min.x, 0, source_dims.x);
        const mi::Sint32 xmax = mi::math::clamp(read_bbox.max.x, 0, source_dims.x);

        // Read interior voxels
        if (xmax > xmin)
        {
          // destination offset
          const mi::Size dst_off = static_cast<mi::Size>(xmin + dest_delta.x) + dst_off_yz;

          // source offset
          const mi::Size src_off = static_cast<mi::Size>(xmin) + src_off_yz;

          memcpy(svol_brick_data_raw + dst_off * m_vol_fmt_size,
            m_source_buffer + src_off * m_vol_fmt_size,
            m_vol_fmt_size * static_cast<mi::Size>(xmax - xmin));
        }

        // Clamp lower bound voxels when x < 0
        for (mi::Sint32 x = read_bbox.min.x; x < 0; ++x)
        {
          // destination offset
          const mi::Size dst_off = static_cast<mi::Size>(x + dest_delta.x) + dst_off_yz;

          // source offset (fixed)
          const mi::Size src_off = src_off_yz;

          memcpy(svol_brick_data_raw + dst_off * m_vol_fmt_size,
            m_source_buffer + src_off * m_vol_fmt_size, m_vol_fmt_size);
        }

        // Clamp upper bound voxels when x >= source_dims.x
        for (mi::Sint32 x = source_dims.x; x < read_bbox.max.x; ++x)
        {
          // destination offset
          const mi::Size dst_off = static_cast<mi::Size>(x + dest_delta.x) + dst_off_yz;

          // source offset (fixed)
          const mi::Size src_off = static_cast<mi::Size>(source_dims.x - 1) + src_off_yz;

          memcpy(svol_brick_data_raw + dst_off * m_vol_fmt_size,
            m_source_buffer + src_off * m_vol_fmt_size, m_vol_fmt_size);
        }
      }
    }

    if (m_neighbor_data)
    {
      // Handle inside border, retrieve voxels from neighboring pieces
      for (const vtknvindex_volume_neighbor_data::Neighbor_info* neighbor : *m_neighbor_data)
      {
        const Vec3i& dir = neighbor->direction;

        // Check whether any voxels of the neighbor piece need to be accessed for this brick
        bool neighbor_access = false;
        for (int i = 0; i < 3; ++i)
        {
          if (dir[i] > 0)
          {
            if (dest_brick_bbox_clipped_with_border_inner.max[i] > dest_brick_bbox_clipped.max[i])
            {
              neighbor_access = true;
              break;
            }
          }
          else if (dir[i] < 0)
          {
            if (dest_brick_bbox_clipped_with_border_inner.min[i] < dest_brick_bbox_clipped.min[i])
            {
              neighbor_access = true;
              break;
            }
          }
        }

        if (!neighbor_access)
        {
          // Nothing to do
          continue;
        }

        if (dest_brick_bbox_clipped_with_border_inner.contains(neighbor->query_bbox.min) &&
          dest_brick_bbox_clipped_with_border_inner.contains(neighbor->query_bbox.max))
        {
#if 0
          ERROR_LOG << "** neighbor " << neighbor->direction << " for dest "
                    << dest_brick_bbox_clipped_with_border_inner << " with border_bbox "
                    << neighbor->border_bbox << " and source " << m_source_bbox << ", with query "
                    << neighbor->query_bbox << " ==> copy border_bbox" << neighbor->border_bbox
                    << " from neighbor " << neighbor->direction << " with data bbox " << neighbor->data_bbox
                    << " to " << dest_brick_bbox_clipped_with_border_inner << " in brick "
                    << dest_brick_bbox;
#endif

          if (neighbor->is_data_available())
          {
            neighbor->copy(svol_brick_data_raw, dest_brick_bbox, m_vol_fmt_size);
          }
          else
          {
            ERROR_LOG << LOG_svol_rvol_prefix << "Border data " << neighbor->border_bbox
                      << " is not available for brick " << dest_brick_bbox << ".";
          }
        }
      }
    }

    //
    // Clamp data collected so far to the outer border (of the entire volume, not of the piece)
    //
    clamp_to_border(dest_brick_bbox, dest_brick_bbox_clipped_with_border_inner,
      dest_brick_bbox_clipped_with_border, svol_brick_data_raw, m_vol_fmt_size);
  }
}

//-------------------------------------------------------------------------------------------------
mi::Size vtknvindex_import_bricks::get_nb_fragments() const
{
  return m_nb_fragments;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_sparse_volume_importer::vtknvindex_sparse_volume_importer(
  const mi::math::Vector_struct<mi::Uint32, 3>& volume_size, mi::Sint32 border_size,
  mi::Sint32 ghost_levels, const std::string& scalar_type)
  : m_border_size(border_size)
  , m_ghost_levels(ghost_levels)
  , m_volume_size(volume_size)
  , m_scalar_type(scalar_type)
  , m_cluster_properties(nullptr)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_sparse_volume_importer::vtknvindex_sparse_volume_importer()
  : m_border_size(0)
  , m_ghost_levels(0)
  , m_cluster_properties(nullptr)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
// Give an estimate of the size in bytes of this subcube.
mi::Size vtknvindex_sparse_volume_importer::estimate(
  const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box,
  mi::neuraylib::IDice_transaction* /*dice_transaction*/) const
{
  const mi::Size dx = bounding_box.max.x - bounding_box.min.x;
  const mi::Size dy = bounding_box.max.y - bounding_box.min.y;
  const mi::Size dz = bounding_box.max.z - bounding_box.min.z;
  const mi::Size volume_brick_size = dx * dy * dz;
  mi::Size scalar_size = 0;

  if (m_scalar_type == "double")
  {
    // Double scalar data is converted to float before passing to IndeX
    scalar_size = sizeof(mi::Float32);
  }
  else
  {
    scalar_size = vtknvindex_regular_volume_properties::get_scalar_size(m_scalar_type);
  }

  if (scalar_size > 0)
  {
    return volume_brick_size * scalar_size;
  }
  else
  {
    ERROR_LOG << "Failed to give an estimate for the unsupported scalar_type: " << m_scalar_type
              << ".";
    return 0;
  }
}

//-------------------------------------------------------------------------------------------------
nv::index::IDistributed_data_subset* vtknvindex_sparse_volume_importer::create(
  const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box, mi::Uint32 time_step,
  nv::index::IData_subset_factory* factory,
  mi::neuraylib::IDice_transaction* dice_transaction) const
{
  using namespace nv::index;
  using namespace vtknvindex::util;
  using mi::base::Handle;

  // Setup the attribute-set descriptor
  Handle<ISparse_volume_attribute_set_descriptor> svol_attrib_set_desc(
    factory->create_attribute_set_descriptor<ISparse_volume_attribute_set_descriptor>());
  if (!svol_attrib_set_desc.is_valid_interface())
  {
    ERROR_LOG << LOG_svol_rvol_prefix
              << "Unable to create a sparse-volume attribute-set descriptor.";
    return nullptr;
  }

  const mi::Uint32 svol_attrib_index_0 = 0u;
  ISparse_volume_attribute_set_descriptor::Attribute_parameters svol_attrib_param_0;
  svol_attrib_param_0.format = match_volume_format(m_scalar_type);

  if (svol_attrib_param_0.format == nv::index::SPARSE_VOLUME_VOXEL_FORMAT_COUNT)
  {
    ERROR_LOG << LOG_svol_rvol_prefix << "Invalid volume format '" << m_scalar_type << "'.";
    return nullptr;
  }

  svol_attrib_set_desc->setup_attribute(svol_attrib_index_0, svol_attrib_param_0);

  // Create sparse volume data subset
  Handle<ISparse_volume_subset> svol_data_subset(
    factory->create_data_subset<ISparse_volume_subset>(svol_attrib_set_desc.get()));

  if (!svol_data_subset.is_valid_interface())
  {
    ERROR_LOG << LOG_svol_rvol_prefix << "Unable to create a sparse-volume data-subset.";
    return nullptr;
  }

  const nv::index::Sparse_volume_voxel_format vol_fmt = svol_attrib_param_0.format;
  const mi::Size vol_fmt_size = volume_format_size(vol_fmt);

  // Input the required data-bricks into the subset
  Handle<const ISparse_volume_subset_data_descriptor> svol_subset_desc(
    svol_data_subset->get_subset_data_descriptor());

  // Bounding box of the IndeX subset (without border). This should match (or be contained in) the
  // bounding box of the corresponding shared memory buffer.
  const mi::math::Bbox<mi::Float32, 3> subset_subregion_bbox =
    svol_subset_desc->get_subregion_scene_space();

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  const mi::Sint32 rankid = controller ? controller->GetLocalProcessId() : 0;

  vtknvindex_host_properties* host_props = m_cluster_properties->get_host_properties(rankid);

  // Fetch local or shared memory details from host properties
  const vtknvindex_host_properties::shm_info* shm_info;
  const mi::Uint8* subset_data_buffer =
    host_props->get_subset_data_buffer(subset_subregion_bbox, time_step, &shm_info);

  if (!shm_info)
  {
    ERROR_LOG << LOG_svol_rvol_prefix << "Failed to retrieve shared memory info for subset "
              << subset_subregion_bbox << " on rank " << rankid << ".";
    return nullptr;
  }

  const Bbox3i shm_bbox(shm_info->m_shm_bbox); // no rounding necessary, only uses integer values

  if (shm_info->m_shm_name.empty() || shm_bbox.empty())
  {
    ERROR_LOG << LOG_svol_rvol_prefix
              << "Failed to open shared memory shmname: " << shm_info->m_shm_name
              << " with bbox: " << shm_bbox << ".";
    return nullptr;
  }
  if (!subset_data_buffer)
  {
    ERROR_LOG << LOG_svol_rvol_prefix
              << "Could not retrieve data for shared memory: " << shm_info->m_shm_name << " box "
              << shm_bbox << " on rank " << rankid << ".";
    return nullptr;
  }

  bool free_buffer = false;
  // Convert double scalar data to float, but only if the data is local. Data in shared memory will
  // already have been converted at this point.
  if (shm_info->m_rank_id == rankid && m_scalar_type == "double")
  {
    mi::Size nb_voxels = shm_info->m_size / sizeof(mi::Float64);

    mi::Float32* voxels_float = new mi::Float32[nb_voxels];
    const mi::Float64* voxels_double = reinterpret_cast<const mi::Float64*>(subset_data_buffer);

    for (mi::Size i = 0; i < nb_voxels; ++i)
      voxels_float[i] = static_cast<mi::Float32>(voxels_double[i]);

    subset_data_buffer = reinterpret_cast<mi::Uint8*>(voxels_float);
    free_buffer = true;
  }

  if (shm_info->m_neighbors)
  {
    // Ensure border data is available
    shm_info->m_neighbors->fetch_data(host_props, time_step);
  }

  INFO_LOG << "Importing volume data from " << (rankid == shm_info->m_rank_id ? "local" : "shared")
           << " "
           << "memory (" << shm_info->m_shm_name << ") on rank " << rankid << ", "
           << (m_scalar_type == "double" ? "converted to float, " : "") << "data bbox " << shm_bbox
           << ", "
           << "importer bbox " << bounding_box << ", border " << m_ghost_levels << "/"
           << m_border_size << ".";

  // Import all bricks for the subregion in parallel
  vtknvindex_import_bricks import_bricks_job(svol_subset_desc.get(), svol_data_subset.get(),
    subset_data_buffer, vol_fmt_size, m_border_size, m_ghost_levels, shm_bbox,
    shm_info->m_neighbors.get());

  dice_transaction->execute_fragmented(&import_bricks_job, import_bricks_job.get_nb_fragments());

  host_props->set_read_flag(time_step, shm_info->m_shm_name);

  if (free_buffer)
  {
    // Free temporary data buffer (used for double-float conversion)
    delete[] subset_data_buffer;
  }

  svol_data_subset->retain();
  return svol_data_subset.get();
}

//-------------------------------------------------------------------------------------------------
nv::index::IDistributed_data_subset* vtknvindex_sparse_volume_importer::create(
  const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box,
  nv::index::IData_subset_factory* factory,
  mi::neuraylib::IDice_transaction* dice_transaction) const
{
  return create(bounding_box, 0u, factory, dice_transaction);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_sparse_volume_importer::set_cluster_properties(
  vtknvindex_cluster_properties* cluster_properties)
{
  m_cluster_properties = cluster_properties;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_sparse_volume_importer::serialize(mi::neuraylib::ISerializer* serializer) const
{
  vtknvindex::util::serialize(serializer, m_scalar_type);
  serializer->write(&m_volume_size.x, 3);
  serializer->write(&m_border_size);
  serializer->write(&m_ghost_levels);

  const mi::Uint32 instance_id = m_cluster_properties->get_instance_id();
  serializer->write(&instance_id);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_sparse_volume_importer::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  vtknvindex::util::deserialize(deserializer, m_scalar_type);
  deserializer->read(&m_volume_size.x, 3);
  deserializer->read(&m_border_size);
  deserializer->read(&m_ghost_levels);

  mi::Uint32 instance_id;
  deserializer->read(&instance_id);
  m_cluster_properties = vtknvindex_cluster_properties::get_instance(instance_id);
}

//-------------------------------------------------------------------------------------------------
mi::base::Uuid vtknvindex_sparse_volume_importer::subset_id() const
{
  return nv::index::ISparse_volume_subset::IID();
}
