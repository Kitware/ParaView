#include "H5Vprivate.h"
  
H5_inline hsize_t UNUSED
H5V_vector_reduce_product(unsigned n, const hsize_t *v)
{
    size_t                  ans = 1;

    if (n && !v) return 0;
    while (n--) ans *= *v++;
    return ans;
}

H5_inline htri_t UNUSED
H5V_vector_zerop_u(int n, const hsize_t *v)
{
    if (!v) return TRUE;
    while (n--) {
        if (*v++) return FALSE;
    }
    return TRUE;
}

H5_inline htri_t UNUSED
H5V_vector_zerop_s(int n, const hssize_t *v)
{
    if (!v) return TRUE;
    while (n--) {
        if (*v++) return FALSE;
    }
    return TRUE;
}

H5_inline int UNUSED
H5V_vector_cmp_u (int n, const hsize_t *v1, const hsize_t *v2)
{
    if (v1 == v2) return 0;
    while (n--) {
        if ((v1 ? *v1 : 0) < (v2 ? *v2 : 0)) return -1;
        if ((v1 ? *v1 : 0) > (v2 ? *v2 : 0)) return 1;
        if (v1) v1++;
        if (v2) v2++;
    }
    return 0;
}

H5_inline int UNUSED
H5V_vector_cmp_s (unsigned n, const hssize_t *v1, const hssize_t *v2)
{
    if (v1 == v2) return 0;
    while (n--) {
        if ((v1 ? *v1 : 0) < (v2 ? *v2 : 0)) return -1;
        if ((v1 ? *v1 : 0) > (v2 ? *v2 : 0)) return 1;
        if (v1) v1++;
        if (v2) v2++;
    }
    return 0;
}

H5_inline void UNUSED
H5V_vector_inc(int n, hsize_t *v1, const hsize_t *v2)
{
    while (n--) *v1++ += *v2++;
}

