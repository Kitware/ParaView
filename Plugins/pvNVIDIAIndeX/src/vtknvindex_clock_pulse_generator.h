// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtknvindex_clock_pulse_generator_h
#define vtknvindex_clock_pulse_generator_h

#include <mi/base/types.h>
#include <mi/dice.h>

#include <nv/index/itime_mapping.h>

// The class vtknvindex_clock_pulse_generator generates the time for playing animations.
// The application can implement a custom clock pulse generator to steer the
// visualization of time-series datasets.
class vtknvindex_clock_pulse_generator
  : public mi::neuraylib::Base<0x11169278, 0x9574, 0x402a, 0xab, 0xb0, 0x77, 0xcf, 0x51, 0x53, 0x74,
      0x46, nv::index::IClock_pulse_generator>
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
