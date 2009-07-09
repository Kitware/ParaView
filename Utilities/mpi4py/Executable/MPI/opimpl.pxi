cdef object _op_MAX(object x, object y):
    """maximum"""
    return max(x, y)

cdef object _op_MIN(object x, object y):
    """minimum"""
    return min(x, y)

cdef object _op_SUM(object x, object y):
    """sum"""
    return x + y

cdef object _op_PROD(object x, object y):
    """product"""
    return x * y

cdef object _op_BAND(object x, object y):
    """bit-wise and"""
    return x & y

cdef object _op_BOR(object x, object y):
    """bit-wise or"""
    return x | y

cdef object _op_BXOR(object x, object y):
    """bit-wise xor"""
    return x ^ y

cdef object _op_LAND(object x, object y):
    """logical and"""
    return bool(x) & bool(y)

cdef object _op_LOR(object x, object y):
    """logical or"""
    return bool(x) | bool(y)

cdef object _op_LXOR(object x, object y):
    """logical xor"""
    return bool(x) ^ bool(y)

cdef object _op_MAXLOC(object x, object y):
    """maximum and location"""
    u, i = x
    v, j = y
    w = max(u, v)
    k = None
    if u == v:
        k = min(i, j)
    elif u <  v:
        k = j
    else:
        k = i
    return (w, k)

cdef object _op_MINLOC(object x, object y):
    """minimum and location"""
    u, i = x
    v, j = y
    w = min(u, v)
    k = None
    if u == v:
        k = min(i, j)
    elif u <  v:
        k = i
    else:
        k = j
    return (w, k)

cdef object _op_REPLACE(object x, object y):
    """replace,  (x, y) -> x"""
    return x
