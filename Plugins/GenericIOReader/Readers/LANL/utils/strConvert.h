/*=========================================================================

Copyright (c) 2017, Los Alamos National Security, LLC

All rights reserved.

Copyright 2017. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _GIO_PV_STR_CONV_H_
#define _GIO_PV_STR_CONV_H_

#include <sstream>

namespace GIOPvPlugin
{

inline float to_float(std::string value)
{
  std::stringstream sstr(value);
  float val;
  sstr >> val;
  return val;
}

inline double to_double(std::string value)
{
  std::stringstream sstr(value);
  double val;
  sstr >> val;
  return val;
}

inline int64_t to_int64(std::string value)
{
  std::stringstream sstr(value);
  int64_t val;
  sstr >> val;
  return val;
}

inline int32_t to_int32(std::string value)
{
  std::stringstream sstr(value);
  int32_t val;
  sstr >> val;
  return val;
}

inline int16_t to_int16(std::string value)
{
  std::stringstream sstr(value);
  int16_t val;
  sstr >> val;
  return val;
}

inline int8_t to_int8(std::string value)
{
  std::stringstream sstr(value);
  int16_t val;
  sstr >> val;
  return val;
}

inline uint64_t to_uint64(std::string value)
{
  std::stringstream sstr(value);
  uint64_t val;
  sstr >> val;
  return val;
}

inline uint32_t to_uint32(std::string value)
{
  std::stringstream sstr(value);
  uint32_t val;
  sstr >> val;
  return val;
}

inline uint16_t to_uint16(std::string value)
{
  std::stringstream sstr(value);
  uint16_t val;
  sstr >> val;
  return val;
}

inline uint8_t to_uint8(std::string value)
{
  std::stringstream sstr(value);
  uint8_t val;
  sstr >> val;
  return val;
}

} // GIOPvPlugin namespace

#endif
