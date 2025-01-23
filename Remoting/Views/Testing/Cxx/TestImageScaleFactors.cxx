// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSMSaveScreenshotProxy.h"
#include "vtkVector.h"

#include <cassert>

namespace
{
void Test(const vtkVector2i& target, const vtkVector2i& maxsize, bool expect_approx = false)
{
  bool approx;
  vtkVector2i size(maxsize);
  vtkVector2i mag = vtkSMSaveScreenshotProxy::GetScaleFactorsAndSize(target, size, &approx);

  cout << "----------------------------------------------------" << endl;
  cout << " Target: " << target << " Max size: " << maxsize << endl;
  cout << " Achieved: " << (size * mag) << " Approx: " << approx << endl;
  cout << " New size: " << size << " scale: " << mag << endl << endl;
  if (size * mag != target && expect_approx == false)
  {
    throw false;
  }
}
}

// Tests code in vtkSMSaveScreenshotProxy to compute scale factors when saving
// images at target resolution.
extern int TestImageScaleFactors(int, char*[])
{
  try
  {
    // totally crazy sizes.
    Test(vtkVector2i(2188, 1236), vtkVector2i(538, 638), true);

    // preserves aspect ratio
    Test(vtkVector2i(1280, 800), vtkVector2i(800, 800));

    // let's try a prime target.
    Test(vtkVector2i(1280, 811), vtkVector2i(800, 800), true);

    // let's try a prime max.
    Test(vtkVector2i(1280, 815), vtkVector2i(811, 811));
  }
  catch (bool)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
