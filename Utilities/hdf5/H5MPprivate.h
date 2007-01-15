/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Header file for private definitions of the MPE instrumentation.
 */
#ifndef _H5MPprivate_H
#define _H5MPprivate_H

#ifdef H5_HAVE_MPE
/*------------------------------------------------------------------------
 * Purpose:    Begin to collect MPE log information for a function. It should
 *             be ahead of the actual function's process.
 *
 * Programmer: Long Wang
 *
 *------------------------------------------------------------------------
 */
#include "mpe.h"
/*
 * #define eventa(func_name)   h5_mpe_ ## func_name ## _a
 * #define eventb(func_name)   h5_mpe_ ## func_name ## _b
 */
#define eventa(func_name)   h5_mpe_eventa
#define eventb(func_name)   h5_mpe_eventb
#define COLOR(func_name)    color_ ## func_name
#define MPE_LOG_VARS(func_name)                                               \
    static int eventa(func_name) = -1;                                        \
    static int eventb(func_name) = -1;                                        \
    char* p_end_funcname = #func_name;                                        \
    char* p_event_start = "start" #func_name;

#define BEGIN_MPE_LOG(func_name)                                              \
  if (H5_MPEinit_g){                    \
    if (H5_MPEinit_g && eventa(func_name) == -1 && eventb(func_name) == -1) { \
  char* p_color = COLOR(func_name);              \
         eventa(func_name)=MPE_Log_get_event_number();                        \
         eventb(func_name)=MPE_Log_get_event_number();                        \
         MPE_Describe_state(eventa(func_name), eventb(func_name), p_end_funcname,p_color); \
    }                                                                         \
    MPE_Log_event(eventa(func_name), 0, p_event_start);                 \
  }


/*------------------------------------------------------------------------
 * Purpose:   Finish the collection of MPE log information for a function.
 *            It should be after the actual function's process.
 *
 * Programmer: Long Wang
 */
#define FINISH_MPE_LOG                                                       \
    if (H5_MPEinit_g) {                                                      \
        MPE_Log_event(eventb(func_name), 0, p_end_funcname);                 \
    }



/* color for each public HDF5 API function */
#define color_H5_init_library "red"

#define color_H5open "red"
#define color_H5close "red"
#define color_H5dont_atexit "red"
#define color_H5garbage_collect "red"
#define color_H5set_free_list_limits "red"
#define color_H5get_libversion "red"
#define color_H5check_version "red"

#define color_H5Dcreate "red"
#define color_H5Dopen "red"
#define color_H5Dclose "red"
#define color_H5Dget_space "red"
#define color_H5Dget_space_status "red"
#define color_H5Dget_type "red"
#define color_H5Dget_create_plist "red"
#define color_H5Dget_storage_size "red"
#define color_H5Dread "red"
#define color_H5Dwrite "red"
#define color_H5Dextend "red"
#define color_H5Diterate "red"
#define color_H5Dvlen_reclaim "red"
#define color_H5Dvlen_get_buf_size "red"
#define color_H5Dfill "red"
#define color_H5Ddebug "red"
#define color_H5Dset_extent "red"
#define color_H5Dget_offset "red"

#define color_H5Eset_auto "red"
#define color_H5Eget_auto "red"
#define color_H5Eclear "red"
#define color_H5Eprint "red"
#define color_H5Ewalk "red"
#define color_H5Eget_major "red"
#define color_H5Eget_minor "red"
#define color_H5Epush "red"

#define color_H5Fis_hdf5 "red"
#define color_H5Fcreate "red"
#define color_H5Fopen "red"
#define color_H5Freopen "red"
#define color_H5Fflush "red"
#define color_H5Fclose "red"
#define color_H5Fget_create_plist "red"
#define color_H5Fget_access_plist "red"
#define color_H5Fget_obj_count "red"
#define color_H5Fget_vfd_handle "red"
#define color_H5Fget_obj_ids "red"
#define color_H5Fmount "red"
#define color_H5Funmount "red"
#define color_H5Fget_freespace "red"
#define color_H5Fget_filesize "red"
#define color_H5Fget_name "red"

#define color_H5Gcreate "red"
#define color_H5Gopen "red"
#define color_H5Gclose "red"
#define color_H5Giterate "red"
#define color_H5Gmove2 "red"
#define color_H5Glink2 "red"
#define color_H5Gunlink "red"
#define color_H5Gget_objinfo "red"
#define color_H5Gget_linkval "red"
#define color_H5Gset_comment "red"
#define color_H5Gget_comment "red"
#define color_H5Gget_num_objs "red"
#define color_H5Gget_objname_by_idx "red"
#define color_H5Gget_objtype_by_idx "red"

#define color_H5Idec_ref "red"
#define color_H5Iget_file_id "red"
#define color_H5Iget_name "red"
#define color_H5Iget_ref "red"
#define color_H5Iget_type "red"
#define color_H5Iinc_ref "red"

#define color_H5Rdereference "red"
#define color_H5Rcreate "red"
#define color_H5Rget_region "red"
#define color_H5Rget_object_type "red"
#define color_H5Rget_obj_type "red"

#define color_H5Topen "red"
#define color_H5Tcreate "red"
#define color_H5Tcopy "red"
#define color_H5Tclose "red"
#define color_H5Tequal "red"
#define color_H5Tlock "red"
#define color_H5Tcommit "red"
#define color_H5Tcommitted "red"
#define color_H5Tinsert "red"
#define color_H5Tpack "red"
#define color_H5Tenum_create "red"
#define color_H5Tenum_insert "red"
#define color_H5Tenum_nameof "red"
#define color_H5Tenum_valueof "red"
#define color_H5Tvlen_create "red"
#define color_H5Tarray_create "red"
#define color_H5Tis_variable_str "red"
#define color_H5Tget_array_dims "red"
#define color_H5Tget_array_ndims "red"
#define color_H5Tset_tag "red"
#define color_H5Tget_tag "red"
#define color_H5Tget_super "red"
#define color_H5Tget_class "red"
#define color_H5Tdetect_class "red"
#define color_H5Tget_size "red"
#define color_H5Tget_order "red"
#define color_H5Tget_precision "red"
#define color_H5Tget_offset "red"
#define color_H5Tget_pad "red"
#define color_H5Tget_sign "red"
#define color_H5Tget_fields "red"
#define color_H5Tget_ebias "red"
#define color_H5Tget_norm "red"
#define color_H5Tget_inpad "red"
#define color_H5Tget_strpad "red"
#define color_H5Tget_nmembers "red"
#define color_H5Tget_num_members "red"
#define color_H5Tget_member_name "red"
#define color_H5Tget_member_index "red"
#define color_H5Tget_member_offset "red"
#define color_H5Tget_member_class "red"
#define color_H5Tget_member_value "red"
#define color_H5Tget_member_type "red"
#define color_H5Tget_cset "red"
#define color_H5Tset_size "red"
#define color_H5Tset_order "red"
#define color_H5Tset_precision "red"
#define color_H5Tset_offset "red"
#define color_H5Tset_pad "red"
#define color_H5Tset_sign "red"
#define color_H5Tset_fields "red"
#define color_H5Tset_ebias "red"
#define color_H5Tset_norm "red"
#define color_H5Tset_inpad "red"
#define color_H5Tset_cset "red"
#define color_H5Tset_strpad "red"
#define color_H5Tregister "red"
#define color_H5Tunregister "red"
#define color_H5Tfind "red"
#define color_H5Tconvert "red"
#define color_H5Tget_overflow "red"
#define color_H5Tset_overflow "red"
#define color_H5Tget_native_type "red"

#define color_H5Acreate "red"
#define color_H5Aopen_name "red"
#define color_H5Aopen_idx "red"
#define color_H5Awrite "red"
#define color_H5Aread "red"
#define color_H5Aclose "red"
#define color_H5Aget_space "red"
#define color_H5Aget_type "red"
#define color_H5Aget_name "red"
#define color_H5Aget_num_attrs "red"
#define color_H5Aget_storage_size "red"
#define color_H5Aiterate "red"
#define color_H5Adelete "red"
#define color_H5Arename "red"

#define color_H5FDregister "red"
#define color_H5FDunregister "red"
#define color_H5FDopen "red"
#define color_H5FDclose "red"
#define color_H5FDcmp "red"
#define color_H5FDquery "red"
#define color_H5FDalloc "red"
#define color_H5FDfree "red"
#define color_H5FDrealloc "red"
#define color_H5FDget_eoa "red"
#define color_H5FDset_eoa "red"
#define color_H5FDget_eof "red"
#define color_H5FDget_vdf_handle "red"
#define color_H5FDread "red"
#define color_H5FDwrite "red"
#define color_H5FDflush "red"
#define color_H5FDget_vfd_handle "red"
#define color_H5Pset_fapl_core "red"
#define color_H5Pget_fapl_core "red"
#define color_H5Pset_fapl_family "red"
#define color_H5Pget_fapl_family "red"
#define color_H5Pset_fapl_log "red"
#define color_H5Pget_fapl_log "red"
#define color_H5Pset_fapl_mpio "red"
#define color_H5Pget_fapl_mpio "red"
#define color_H5Pset_dxpl_mpio "red"
#define color_H5Pget_dxpl_mpio "red"
#define color_H5Pset_fapl_mpiposix "red"
#define color_H5Pget_fapl_mpiposix "red"
#define color_H5Pset_fapl_sec2 "red"
#define color_H5Pget_fapl_sec2 "red"
#define color_H5Pset_fapl_stream "red"
#define color_H5Pget_fapl_stream "red"
#define color_H5Pget_filter "red"
#define color_H5Pset_btree_ratios "red"
#define color_H5Pget_btree_ratios "red"
#define color_H5Pset_shuffle "red"

#define color_H5Pcreate_class "red"
#define color_H5Pget_class_name "red"
#define color_H5Pcreate "red"
#define color_H5Pregister "red"
#define color_H5Pinsert "red"
#define color_H5Pset "red"
#define color_H5Pexist "red"
#define color_H5Pget_size "red"
#define color_H5Pget_nprops "red"
#define color_H5Pget_class "red"
#define color_H5Pget_class_parent "red"
#define color_H5Pget "red"
#define color_H5Pequal "red"
#define color_H5Pisa_class "red"
#define color_H5Piterate "red"
#define color_H5Pcopy_prop "red"
#define color_H5Premove "red"
#define color_H5Punregister "red"
#define color_H5Pclose_class "red"
#define color_H5Pclose "red"
#define color_H5Pcopy "red"
#define color_H5Pget_version "red"
#define color_H5Pset_userblock "red"
#define color_H5Pget_userblock "red"
#define color_H5Pset_alignment "red"
#define color_H5Pget_alignment "red"
#define color_H5Pset_sizes "red"
#define color_H5Pget_sizes "red"
#define color_H5Pset_sym_k "red"
#define color_H5Pget_sym_k "red"
#define color_H5Pset_istore_k "red"
#define color_H5Pget_istore_k "red"
#define color_H5Pset_layout "red"
#define color_H5Pget_layout "red"
#define color_H5Pset_chunk "red"
#define color_H5Pget_chunk "red"
#define color_H5Pset_external "red"
#define color_H5Pget_external_count "red"
#define color_H5Pget_external "red"
#define color_H5Pset_driver "red"
#define color_H5Pget_driver "red"
#define color_H5Pget_driver_info "red"
#define color_H5Pset_family_offset "red"
#define color_H5Pget_family_offset "red"
#define color_H5Pset_multi_type "red"
#define color_H5Pget_multi_type "red"
#define color_H5Pset_buffer "red"
#define color_H5Pget_buffer "red"
#define color_H5Pset_preserve "red"
#define color_H5Pget_preserve "red"
#define color_H5Pall_filters_avail "red"
#define color_H5Pset_filter "red"
#define color_H5Pmodify_filter "red"
#define color_H5Pget_nfilters "red"
#define color_H5Pget_filter "red"
#define color_H5Pget_filter_by_id "red"
#define color_H5Premove_filter "red"
#define color_H5Pset_deflate "red"
#define color_H5Pset_fletcher32 "red"
#define color_H5Pset_szip "red"
#define color_H5Pset_cache "red"
#define color_H5Pget_cache "red"
#define color_H5Pset_hyper_cache "red"
#define color_H5Pget_hyper_cache "red"
#define color_H5Pset_htree_ratios "red"
#define color_H5Pget_htree_ratios "red"
#define color_H5Pset_fill_value "red"
#define color_H5Pget_fill_value "red"
#define color_H5Pfill_value_defined "red"
#define color_H5Pset_alloc_time "red"
#define color_H5Pget_alloc_time "red"
#define color_H5Pset_fill_time "red"
#define color_H5Pget_fill_time "red"
#define color_H5Pset_gc_references "red"
#define color_H5Pget_gc_references "red"
#define color_H5Pset_fclose_degree "red"
#define color_H5Pget_fclose_degree "red"
#define color_H5Pset_vlen_mem_manager "red"
#define color_H5Pget_vlen_mem_manager "red"
#define color_H5Pset_meta_block_size "red"
#define color_H5Pget_meta_block_size "red"
#define color_H5Pset_sieve_buf_size "red"
#define color_H5Pget_sieve_buf_size "red"
#define color_H5Pset_hyper_vector_size "red"
#define color_H5Pget_hyper_vector_size "red"
#define color_H5Pset_small_data_block_size "red"
#define color_H5Pget_small_data_block_size "red"
#define color_H5Pset_edc_check "red"
#define color_H5Pget_edc_check "red"
#define color_H5Pset_filter_callback "red"

#define color_H5Screate "red"
#define color_H5Screate_simple "red"
#define color_H5Sset_extent_simple "red"
#define color_H5Scopy "red"
#define color_H5Sclose "red"
#define color_H5Sget_simple_extent_npoints "red"
#define color_H5Sget_simple_extent_ndims "red"
#define color_H5Sget_simple_extent_dims "red"
#define color_H5Sis_simple "red"
#define color_H5Sset_space "red"
#define color_H5Sget_select_npoints "red"
#define color_H5Sselect_hyperslab "red"
#define color_H5Scombine_hyperslab "red"
#define color_H5Sselect_select "red"
#define color_H5Scombine_select "red"
#define color_H5Sselect_elements "red"
#define color_H5Sget_simple_extent_type "red"
#define color_H5Sset_extent_none "red"
#define color_H5Sextent_copy "red"
#define color_H5Sselect_all "red"
#define color_H5Sselect_none "red"
#define color_H5Soffset_simple "red"
#define color_H5Sselect_valid "red"
#define color_H5Sget_select_hyper_nblocks "red"
#define color_H5Sget_select_elem_npoints "red"
#define color_H5Sget_select_hyper_blocklist "red"
#define color_H5Sget_select_elem_pointlist "red"
#define color_H5Sget_select_bounds "red"
#define color_H5Sget_select_type "red"

#define color_H5Zregister "red"
#define color_H5Zfilter_avail "red"
#define color_H5Zunregister "red"
#define color_H5Zget_filter_info "red"

#else
#define MPE_LOG_VARS(func_name) /* void */
#define BEGIN_MPE_LOG(func_name) /* void */
#define FINISH_MPE_LOG   /* void */

#endif

#endif
