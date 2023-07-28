// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtknvindex_receiving_logger_h
#define vtknvindex_receiving_logger_h

#include <mi/base/ilogger.h>
#include <mi/dice.h>

// The class vtknvindex_forwarding_logger forwards warning/errors messages gathered
// by the NVIDIA IndeX library to ParaView's console.
class vtknvindex_receiving_logger : public mi::base::Interface_implement<mi::base::ILogger>
{
public:
  vtknvindex_receiving_logger();

  // Set message and severity values.
  void message(mi::base::Message_severity level, const char* category,
    const mi::base::Message_details& details, const char* message) override;

private:
  vtknvindex_receiving_logger(const vtknvindex_receiving_logger&) = delete;
  void operator=(const vtknvindex_receiving_logger&) = delete;
};

#endif // vtknvindex_receiving_logger_h
