/* Copyright 2019 NVIDIA Corporation. All rights reserved.
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

#ifndef vtknvindex_clock_pulse_generator_h
#define vtknvindex_clock_pulse_generator_h

#include <mi/base/interface_declare.h>
#include <mi/base/types.h>
#include <mi/dice.h>

#include <nv/index/itime_mapping.h>

class vtknvindex_clock_pulse_generator_base
  : public mi::base::Interface_declare<0x11169278, 0x9574, 0x402a, 0xab, 0xb0, 0x77, 0xcf, 0x51,
      0x53, 0x74, 0x46, nv::index::IClock_pulse_generator>
{
};

// The class vtknvindex_clock_pulse_generator generates the time for playing animations.
// The application can implement a custom clock pulse generator to steer the
// visualization of time-series datasets.
class vtknvindex_clock_pulse_generator
  : public mi::base::Interface_implement<vtknvindex_clock_pulse_generator_base>
{
public:
  vtknvindex_clock_pulse_generator(mi::Float64 start = 0., mi::Float64 end = 0.);

  void set_tick(mi::Float64 tick);

  // Implementation of Ivtknvindex_clock_pulse_generator.
  mi::Float64 get_tick() override { return m_t; }
  mi::Float64 get_start() const override { return m_t_start; }
  mi::Float64 get_end() const override { return m_t_end; }

private:
  mi::Float64 m_t;
  mi::Float64 m_t_start;
  mi::Float64 m_t_end;
};
#endif // vtknvindex_clock_pulse_generator_h
