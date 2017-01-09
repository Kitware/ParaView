/*=========================================================================

Program:   ParaView
Module:    TestAdjustRange.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMCoreUtilities.h"
#include "vtkTestUtilities.h"

#include <assert.h>
#include <cmath>
#include <sstream>

namespace
{

bool valid_range(double range[2])
{
  const bool range_equal = (range[0] == range[1]);
  const bool range_spans_pos_neg = (range[0] < 0 && range[1] > 0);

  if (range_spans_pos_neg)
  { // if the range spans both positive and negative the Adjustment will fail.
    return (vtkSMCoreUtilities::AdjustRange(range) == false);
  }

  if (range_equal)
  { // if the range is equal the values will be shifted
    const bool adjusted = vtkSMCoreUtilities::AdjustRange(range);
    return adjusted && (range[0] != range[1]);
  }

  // okay lastly we need to determine at least how far the range will move
  // we use looping with nextafter to compute roughly where the range
  // should at least be pushed out too, guarding against not pushing
  // past the original range max value
  double original_range[2] = { range[0], range[1] };
  double next_value = original_range[0];
  for (std::size_t i = 0; i < 65536 && next_value < original_range[1]; ++i)
  {
    next_value = std::nextafter(next_value, original_range[1]);
  }

  // verify that AdjustRange range has at least 65k different valid
  // values between the min and max. It could be more if the input
  // range had more, or if we started as a denormal value
  const bool adjusted = vtkSMCoreUtilities::AdjustRange(range);
  return (original_range[0] == range[0]) && (next_value <= range[1]);
}
}

int TestAdjustRange(int argc, char* argv[])
{

  // Simple set of tests to validate that vtkSMCoreUtilities::AdjustRange
  // behaves as we expect

  double zeros[2] = { 0.0, 0.0 };
  double ones[2] = { 1.0, 1.0 };
  double nones[2] = { -1.0, -1.0 };
  double zero_one[2] = { 0.0, 1.0 };
  double none_one[2] = { -1.0, 1.0 };

  double small[2] = { -12, -4 };
  double large[2] = { 1e12, 1e12 + 1 };
  double large_exact[2] = { 1e12, 1e12 };
  double real_small[2] = { 1e-20, 1e-19 };

  int exit_code = EXIT_SUCCESS;
  if (!valid_range(zeros))
  {
    cerr << "Failed testing zeros" << endl;
    exit_code = EXIT_FAILURE;
  }
  if (!valid_range(ones))
  {
    cerr << "Failed at testing ones" << endl;
    exit_code = EXIT_FAILURE;
  }
  if (!valid_range(nones))
  {
    cerr << "Failed at testing nones" << endl;
    exit_code = EXIT_FAILURE;
  }
  if (!valid_range(zero_one))
  {
    cerr << "Failed at testing zero_one" << endl;
    exit_code = EXIT_FAILURE;
  }
  if (!valid_range(none_one))
  {
    cerr << "Failed at testing none_one" << endl;
    exit_code = EXIT_FAILURE;
  }
  if (!valid_range(small))
  {
    cerr << "Failed at testing small" << endl;
    exit_code = EXIT_FAILURE;
  }
  if (!valid_range(large))
  {
    cerr << "Failed at testing large" << endl;
    exit_code = EXIT_FAILURE;
  }
  if (!valid_range(large_exact))
  {
    cerr << "Failed at testing large_exact" << endl;
    exit_code = EXIT_FAILURE;
  }
  if (!valid_range(real_small))
  {
    cerr << "Failed at testing real_small" << endl;
    exit_code = EXIT_FAILURE;
  }

  return exit_code;
}