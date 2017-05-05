#ifndef H5PartPrivate_h
#define H5PartPrivate_h

#define H5PART_GROUPNAME_STEP "Step"

/*!
  The functions declared here are not part of the API, but may be used
  in extensions like H5Block. We name these functions "private".

  \note
  Private function may change there interface even in stable versions.
  Don't use them in applications!
*/

struct _iter_op_data {
 int stop_idx;
 int count;
 int type;
 char *name;
 size_t len;
 char *pattern;
};

h5part_int64_t
_H5Part_set_step (
 H5PartFile *f,
 const h5part_int64_t step
 );

h5part_int64_t
_H5Part_get_num_particles (
 H5PartFile *f
 );

herr_t
_H5Part_iteration_operator (
 hid_t group_id,
 const char *member_name,
 void *operator_data
 );


void
_H5Part_set_funcname (
 const char *fname
 );

const char *
_H5Part_get_funcname (
 void
 );

#define SET_FNAME( fname ) _H5Part_set_funcname( fname );

hid_t
_H5Part_normalize_h5_type (
 hid_t type
 );

h5part_int64_t
_H5Part_read_attrib (
 hid_t id,
 const char *attrib_name,
 void *attrib_value
 );

h5part_int64_t
_H5Part_write_attrib (
 hid_t id,
 const char *attrib_name,
 const hid_t attrib_type,
 const void *attrib_value,
 const hsize_t attrib_nelem
 );

h5part_int64_t
_H5Part_get_attrib_info (
 hid_t id,
 const h5part_int64_t attrib_idx,
 char *attrib_name,
 const h5part_int64_t len_attrib_name,
 h5part_int64_t *attrib_type,
 h5part_int64_t *attrib_nelem
 );

h5part_int64_t
_H5Part_get_num_objects (
 hid_t group_id,
 const char *group_name,
 const hid_t type
 );

h5part_int64_t
_H5Part_get_num_objects_matching_pattern (
 hid_t group_id,
 const char *group_name,
 const hid_t type,
 char * const pattern
 );

h5part_int64_t
_H5Part_get_object_name (
 hid_t group_id,
 const char *group_name,
 const hid_t type,
 const h5part_int64_t idx,
 char *obj_name,
 const h5part_int64_t len_obj_name
 );

void
_H5Part_vprint_error (
 const char *fmt,
 va_list ap
 );

void
_H5Part_print_error (
 const char *fmt,
 ... )
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

void
_H5Part_vprint_warn (
 const char *fmt,
 va_list ap
 );

void
_H5Part_print_warn (
 const char *fmt,
 ...
 )
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

void
_H5Part_vprint_info (
 const char *fmt,
 va_list ap
 );

void
_H5Part_print_info (
 const char *fmt,
 ...
 )
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

void
_H5Part_vprint_debug (
 const char *fmt,
 va_list ap
 );

void
_H5Part_print_debug (
 const char *fmt,
 ...
 )
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

char *
_H5Part_strdupfor2c (
 const char *s,
 const ssize_t len
 );

char *
_H5Part_strc2for (
 char * const str,
 const ssize_t l_str
 );

#endif
