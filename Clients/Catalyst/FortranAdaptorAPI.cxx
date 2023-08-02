// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// CATALYST_FORTRAN_USING_MANGLING is defined when Fortran names are mangled. If
// not, then we don't add another implementation for the API routes thus avoid
// duplicate implementations.
#ifdef CATALYST_FORTRAN_USING_MANGLING

#include "FortranAdaptorAPI.h"
#include "CAdaptorAPI.cxx"

#endif
