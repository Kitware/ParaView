// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtknvindex_clock_pulse_generator.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_utilities.h"

//----------------------------------------------------------------------------
vtknvindex_clock_pulse_generator::vtknvindex_clock_pulse_generator(
  mi::Float64 start_time, mi::Float64 end_time)
  : m_t(0.0)
  , m_t_start(start_time)
  , m_t_end(end_time)
{
  // empty
}

//----------------------------------------------------------------------------
void vtknvindex_clock_pulse_generator::set_tick(mi::Float64 tick)
{
  m_t = tick;
}
