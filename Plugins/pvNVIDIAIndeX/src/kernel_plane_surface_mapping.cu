/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#define SLOT_VOLUME   1
#define SLOT_COLORMAP 2

class Surface_sample_program
{
    NV_IDX_SURFACE_SAMPLE_PROGRAM
public:
    NV_IDX_DEVICE_INLINE_MEMBER
    void initialize() {}

    NV_IDX_DEVICE_INLINE_MEMBER
    int execute(
        const Sample_info_self& sample_info,        // read-only
              Sample_output&    sample_output)      // write-only
    {
        using namespace nv::index;

        if (!state.scene.is_valid_element<xac::Sparse_volume>(SLOT_VOLUME)) {
            return NV_IDX_PROG_DISCARD_SAMPLE;
        }

        const auto      svol      = state.scene.access<xac::Sparse_volume>(SLOT_VOLUME);
        const float3    svol_spos = transform_point(svol.get_scene_to_object_transform(), sample_info.scene_position);

        const float3    svol_spos_offset = svol_spos + make_float3(0.0f, 0.0f, 0.0f);

        const auto svol_sampler = svol.generate_sampler<float,
                                                        xac::Volume_filter_mode::TRILINEAR,
                                                        xac::Volume_classification_mode::POST_INTERPOLATION>(
                                                            0u);

        // Get a sample at a shifted position to make sure access happens outside of boundary
        // of local sparse volume.

        const auto colormap = state.scene.access<xac::Colormap>(SLOT_COLORMAP);

        const float4 svol_sample = svol_sampler.fetch_sample_classify(svol_spos_offset, colormap);

        sample_output.set_color(svol_sample);

        return NV_IDX_PROG_OK;
    }
};
