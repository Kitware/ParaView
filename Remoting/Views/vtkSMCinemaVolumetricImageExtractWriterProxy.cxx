/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCinemaVolumetricImageExtractWriterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCinemaVolumetricImageExtractWriterProxy.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMExtractsController.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionProxy.h"

#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cassert>
#include <sstream>
#include <vector>

namespace
{
bool IsClose(double v1, double v2, double tol)
{
  return std::abs(v1 - v2) < tol;
}

// Based on algorithm at https://en.m.wikipedia.org/wiki/Combinatorial_number_system to figure
// out which functions are active for this local_option. This is a recursive method.
void AppendWhichFunctions(
  int local_option, int number_of_options, std::vector<int>& which_functions)
{
  int counter = number_of_options - 1;
  int number_of_options_this_level = 0;
  int number_of_previous_options = 0;
  while (number_of_options_this_level <= local_option)
  {
    counter++;
    number_of_previous_options = number_of_options_this_level;
    vtkTypeInt64 tmp = vtkMath::Factorial(counter) /
      (vtkMath::Factorial(number_of_options) * vtkMath::Factorial(counter - number_of_options));
    number_of_options_this_level = static_cast<int>(tmp);
  }
  if (counter > 0)
  {
    which_functions.push_back(counter - 1);
  }
  if (number_of_options > 0)
  {
    AppendWhichFunctions(
      local_option - number_of_previous_options, number_of_options - 1, which_functions);
  }
}

std::vector<double> GetOpacityTransferFunction(bool full_combination, int number_of_functions,
  int number_of_opacity_levels, int maximum_number_of_functions, int option, double* lut_range,
  double max_opacity_value)
{
  if (full_combination)
  {
    if (number_of_opacity_levels > 1)
    {
      std::vector<int> opacity_levels;
      std::vector<double> scales;
      std::vector<int> which_triangles;
      int my_sum = 0;
      for (int r = 1; r < maximum_number_of_functions; r++) // ACB took out the +1 here
      {
        // numcombinationsthislevel is the number of combinations of functions only
        int num_combinations_this_level = vtkMath::Factorial(number_of_functions) /
          (vtkMath::Factorial(r) * vtkMath::Factorial(number_of_functions - r));
        // num_options_this_level is the total number of options for the given number of active
        // functions
        // at this level.
        int num_options_this_level =
          num_combinations_this_level * std::pow(number_of_opacity_levels, r);
        if (my_sum + num_options_this_level > option)
        {
          // r is the number of non-zero functions
          int local_option = option - my_sum; // option for this number of functions
          int local_function_option = local_option / std::pow(number_of_opacity_levels, r);
          AppendWhichFunctions(local_function_option, r, which_triangles);
          std::reverse(
            which_triangles.begin(), which_triangles.end()); // ids were in highest to lowest order
          int local_opacity_option =
            local_option % static_cast<int>(std::pow(number_of_opacity_levels, r));
          for (int i = 0; i < r; i++)
          {
            int opacity_level = local_opacity_option % number_of_opacity_levels;
            opacity_levels.push_back(opacity_level);
            double scale =
              max_opacity_value * static_cast<double>(1 + opacity_level) / number_of_opacity_levels;
            scales.push_back(scale);
            local_opacity_option = local_opacity_option / number_of_opacity_levels;
          }
          break;
        }
        my_sum += num_options_this_level;
      }
      // build up the opacity transfer functions (otf) now that we know which ones are active for
      // this option.
      // build the otf from smallest field value to largest field value
      double total_range = (lut_range[1] - lut_range[0]);
      double triangle_half_range = total_range / (number_of_functions - 1);
      std::vector<double> full_otf;
      // easiest algorithm is to add in all of the values and then remove unneeded duplicates
      int counter = 0;
      for (int i = 0; i < number_of_functions; i++)
      {
        if (std::find(which_triangles.begin(), which_triangles.end(), i) != which_triangles.end())
        {
          full_otf.insert(
            full_otf.end(), { lut_range[0] + i * triangle_half_range, scales[counter], 0.5, 0 });
          counter++;
        }
        else
        {
          full_otf.insert(full_otf.end(), { lut_range[0] + i * triangle_half_range, 0., 0.5, 0 });
        }
      }

      std::vector<double> otf = { full_otf[0], full_otf[1], full_otf[2], full_otf[3] };
      for (int i = 1; i < number_of_functions - 1; i++)
      {
        // if this one isn't the average of the one before and after in the full_otf then we need to
        // add it our
        // merged otf. the average works here since the points are equally spaced.
        double ave = 0.5 * (full_otf[(i - 1) * 4 + 1] + full_otf[(i + 1) * 4 + 1]);
        if (!::IsClose(ave, full_otf[i * 4 + 1], .001))
        {
          // otf.push_back(full_otf[i*4:(i+1)*4]);
          // otf.push_back(fullotf[-4:]);
          otf.insert(otf.end(), &full_otf[i * 4], &full_otf[(i + 1) * 4]);
          otf.insert(otf.end(), full_otf.end() - 4, full_otf.end());
        }
      }
      otf.insert(otf.end(), full_otf.end() - 4, full_otf.end());

      return otf;
      // end full combination with number of opacity levels > 1
    }
    double scale = max_opacity_value; // ACB had to make this change -- make it elsewhere too...

    std::vector<int> which_triangles;
    int my_sum = 0;
    for (int r = 1; r < maximum_number_of_functions; r++)
    {
      int num_combinations_this_level = vtkMath::Factorial(number_of_functions) /
        (vtkMath::Factorial(r) * vtkMath::Factorial(number_of_functions - r));
      if (my_sum + num_combinations_this_level > option)
      {
        // r is the number of non-zero functions
        int local_option = option - my_sum; // option for this number of functions
        AppendWhichFunctions(local_option, r, which_triangles);
        std::reverse(
          which_triangles.begin(), which_triangles.end()); // ids were in highest to lowest order
        break;
      }
      my_sum += num_combinations_this_level;
    }
    // build up the opacity transfer functions (otf) now that we know which ones are active for this
    // option.
    // build the otf from smallest field value to largest field value
    double total_range = lut_range[1] - lut_range[0];
    double triangle_half_range = total_range / (number_of_functions - 1);
    std::vector<double> full_otf;
    // easiest algorithm is to add in all of the values and then remove unneeded duplicates
    for (int i = 0; i < number_of_functions; i++)
    {
      if (std::find(which_triangles.begin(), which_triangles.end(), i) != which_triangles.end())
      {
        full_otf.insert(full_otf.end(), { lut_range[0] + i * triangle_half_range, scale, 0.5, 0 });
      }
      else
      {
        full_otf.insert(full_otf.end(), { lut_range[0] + i * triangle_half_range, 0., 0.5, 0 });
      }
    }
    std::vector<double> otf = { full_otf[0], full_otf[1], full_otf[2], full_otf[3] };
    for (int i = 1; i < number_of_functions - 1; i++)
    {
      // if this one isn't the same as the one before in the full_otf and after we need to add it
      // our
      // merged otf
      if (full_otf[(i - 1) * 4 + 1] != full_otf[i * 4 + 1] ||
        full_otf[(i + 1) * 4 + 1] != full_otf[i * 4 + 1])
      {
        otf.insert(otf.end(), &full_otf[i * 4], &full_otf[(i + 1) * 4]);
      }
    }
    otf.insert(otf.end(), full_otf.end() - 4, full_otf.end());

    return otf;
  }
  else
  {
    // this is for only a single function that's ever active at a time with all of the
    // others inactive
    // numbering scheme is that the first number of opacity levels are for the triangle
    // at the lowest field value, then number of opacity levels for the next lowest
    // triangle field value, ...

    int opacity_level = 1;
    double scale = max_opacity_value;
    if (number_of_opacity_levels > 1)
    {
      opacity_level = option % number_of_opacity_levels;
      scale = max_opacity_value * static_cast<double>(1 + opacity_level) / number_of_opacity_levels;
    }

    double total_range = lut_range[1] - lut_range[0];

    int which_triangle = option / number_of_opacity_levels;
    std::vector<double> otf;
    if (which_triangle == 0)
    {
      // half triangle with the maximum opacity at the minimum field value
      otf.insert(otf.end(),
        { lut_range[0], scale, 0.5, 0, total_range / (number_of_functions - 1) + lut_range[0], 0,
          0.5, 0, lut_range[1], 0, 0.5, 0 });
    }
    else if (which_triangle == number_of_functions - 1)
    {
      // half triangle with the maximum opacity at the maximum field value
      otf.insert(otf.end(),
        { lut_range[0], 0, 0.5, 0, lut_range[1] - total_range / (number_of_functions - 1), 0, 0.5,
          0, lut_range[1], scale, 0.5, 0 });
    }
    else
    {
      // full triangle with the maximum opacity between the minimum and maximum field value.
      // the triangle should also be essentially zero between the minimum and maximum field value.
      if (which_triangle != 1)
      {
        otf.insert(otf.end(), { lut_range[0], 0, 0.5, 0 });
      }
      otf.insert(otf.end(),
        { lut_range[0] + (which_triangle - 1) * total_range / (number_of_functions - 1), 0, 0.5, 0,
          lut_range[0] + which_triangle * total_range / (number_of_functions - 1), scale, 0.5, 0,
          lut_range[0] + (which_triangle + 1) * total_range / (number_of_functions - 1), 0, 0.5,
          0 });

      if (which_triangle != number_of_functions - 2)
      {
        otf.insert(otf.end(), { lut_range[1], 0, 0.5, 0 });
      }
    }
    return otf;
  }
  vtkGenericWarningMacro("vtkSMCinemaVolumetricImageExtractWriterProxy: ERROR couldn't compute a "
                         "opacity transfer function");
  std::vector<double> otf;
  return otf;
}
}

vtkStandardNewMacro(vtkSMCinemaVolumetricImageExtractWriterProxy);
//----------------------------------------------------------------------------
vtkSMCinemaVolumetricImageExtractWriterProxy::vtkSMCinemaVolumetricImageExtractWriterProxy() =
  default;

//----------------------------------------------------------------------------
vtkSMCinemaVolumetricImageExtractWriterProxy::~vtkSMCinemaVolumetricImageExtractWriterProxy() =
  default;

//----------------------------------------------------------------------------
bool vtkSMCinemaVolumetricImageExtractWriterProxy::WriteInternal(
  vtkSMExtractsController* extractor, const SummaryParametersT& params)
{
  auto writer = vtkSMSaveScreenshotProxy::SafeDownCast(this->GetSubProxy("Writer"));
  assert(writer != nullptr);

  auto view = vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(writer, "View").GetAsProxy());
  assert(view != nullptr);

  // if we're doing cinema volume rendering we save the original transfer functions as well
  std::vector<double> old_otf, old_ctf;
  vtkSMTransferFunctionProxy* colorTFProxy = nullptr;
  vtkSMTransferFunctionProxy* opacityTFProxy = nullptr;
  std::vector<double> otf_points, ctf_rgbpoints;

  int number_of_otfs = 1;
  int number_of_functions = vtkSMPropertyHelper(this, "Functions").GetAsInt();
  int number_of_opacity_levels = vtkSMPropertyHelper(this, "OpacityLevels").GetAsInt();
  bool single_function_only = vtkSMPropertyHelper(this, "SingleFunctionOnly").GetAsInt() != 0;
  int maximum_number_of_functions =
    vtkSMPropertyHelper(this, "MaximumNumberOfFunctions").GetAsInt();
  double maximum_opacity_value = vtkSMPropertyHelper(this, "MaximumOpacityValue").GetAsDouble();
  bool specified_range = vtkSMPropertyHelper(this, "SpecifiedRange").GetAsInt() != 0;
  double lut_range[2] = { 0, 1 };
  bool exportTransferFunctions =
    vtkSMPropertyHelper(this, "ExportTransferFunctions").GetAsInt() != 0;
  if (specified_range)
  {
    lut_range[0] = vtkSMPropertyHelper(this, "Range").GetAsDouble(0);
    lut_range[1] = vtkSMPropertyHelper(this, "Range").GetAsDouble(1);
  }

  if (single_function_only)
  {
    number_of_otfs = number_of_functions * number_of_opacity_levels;
  }
  else
  {
    // the total number of combinations is the sum of the number of active functions which is
    // "r" below and at
    // https://www.calculator.net/permutation-and-combination-calculator.html?cnv=6&crv=6&x=76&y=20.
    // "n" is the number_of_functions. for not single_function_only we have to do our sum over the
    // entire range of active
    // functions, e.g. only 1 active functions, then two active functions, ...
    // for when number_of_opacity_levels is greater than 1 that adds number_of_opacity_levels**r
    // for each combination
    // of functions
    if (maximum_number_of_functions > number_of_functions)
    {
      maximum_number_of_functions = number_of_functions;
    }
    int sum = 0;
    for (int r = 1; r < maximum_number_of_functions; r++)
    {
      vtkTypeInt64 tmp = vtkMath::Factorial(number_of_functions) /
        (vtkMath::Factorial(r) * vtkMath::Factorial(number_of_functions - r));
      sum += std::pow(number_of_opacity_levels, r) * static_cast<int>(tmp);
    }
    number_of_otfs = sum;
  }

  bool status = true;
  SummaryParametersT tparams = params;
  for (int otf_index = 0; otf_index < number_of_otfs && status == true; otf_index++)
  {
    if (!colorTFProxy && !opacityTFProxy)
    {
      vtkSMPropertyHelper helper(view, "Representations");
      for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
      {
        if (vtkSMPVRepresentationProxy* repr =
              vtkSMPVRepresentationProxy::SafeDownCast(helper.GetAsProxy(cc)))
        {
          if (vtkSMPropertyHelper(repr, "Visibility", /*quiet*/ true).GetAsInt() ==
            1) // && (!reprType || (reprType && !strcmp(repr->GetXMLName(), reprType))))
          {
            // repr->SetRepresentationType("Volume"); setter but not getter available
            vtkSMPropertyHelper helper2(repr, "Representation");
            auto the_type = helper2.GetAsString();
            if (the_type && strcmp(the_type, "Volume") == 0)
            {
              if (opacityTFProxy == nullptr && colorTFProxy == nullptr)
              {
                vtkSMPropertyHelper color_array_helper(repr, "ColorArrayName");
                std::string color_array_name = color_array_helper.GetInputArrayNameToProcess();
                vtkNew<vtkSMTransferFunctionManager> mgr;
                colorTFProxy =
                  vtkSMTransferFunctionProxy::SafeDownCast(mgr->GetColorTransferFunction(
                    color_array_name.c_str(), repr->GetSessionProxyManager()));

                opacityTFProxy =
                  vtkSMTransferFunctionProxy::SafeDownCast(mgr->GetOpacityTransferFunction(
                    color_array_name.c_str(), repr->GetSessionProxyManager()));
              }

              otf_points = vtkSMPropertyHelper(opacityTFProxy, "Points").GetDoubleArray();
              ctf_rgbpoints = vtkSMPropertyHelper(colorTFProxy, "RGBPoints").GetDoubleArray();
              if (!specified_range)
              {
                lut_range[0] = ctf_rgbpoints[0];
                lut_range[1] = ctf_rgbpoints[ctf_rgbpoints.size() - 4];
              }
              if (old_otf.empty())
              {
                old_otf = otf_points;
              }
              if (old_ctf.empty())
              {
                old_ctf = ctf_rgbpoints;
              }
              break;
            }
          }
        }
      }
    } // if (!colorTFProxy && !opacityTFProxy)
    otf_points = GetOpacityTransferFunction(!single_function_only, number_of_functions,
      number_of_opacity_levels, maximum_number_of_functions, otf_index, lut_range,
      maximum_opacity_value);
    vtkSMPropertyHelper otf_helper(opacityTFProxy, "Points");
    otf_helper.SetNumberOfElements(static_cast<unsigned int>(otf_points.size()));
    otf_helper.Set(otf_points.data(), static_cast<unsigned int>(otf_points.size()));
    opacityTFProxy->UpdateVTKObjects();

    if (exportTransferFunctions)
    {
      std::ostringstream str;
      str << "/cinemavolume_" << otf_index << ".json";
      std::string name = vtksys::SystemTools::JoinPath(
        { extractor->GetRealExtractsOutputDirectory(), str.str().c_str() });
      vtkSMTransferFunctionProxy::ExportTransferFunction(
        colorTFProxy, opacityTFProxy, "cinemavolume", name.c_str());
    }

    tparams["opacity_transfer_function"] = std::to_string(otf_index);
    status = this->Superclass::WriteInternal(extractor, tparams);
  }

  if (!old_otf.empty())
  {
    vtkSMPropertyHelper otf_helper(opacityTFProxy, "Points");
    otf_helper.SetNumberOfElements(static_cast<unsigned int>(old_otf.size()));
    otf_helper.Set(old_otf.data(), static_cast<unsigned int>(old_otf.size()));
    opacityTFProxy->UpdateVTKObjects();
  }
  if (!old_ctf.empty())
  {
    vtkSMPropertyHelper ctf_helper(colorTFProxy, "RGBPoints");
    ctf_helper.SetNumberOfElements(static_cast<unsigned int>(old_ctf.size()));
    ctf_helper.Set(old_ctf.data(), static_cast<unsigned int>(old_ctf.size()));
    colorTFProxy->UpdateVTKObjects();
  }

  return status;
}

//----------------------------------------------------------------------------
const char* vtkSMCinemaVolumetricImageExtractWriterProxy::GetShortName(const std::string& key) const
{
  if (key == "opacity_transfer_function")
  {
    return "otf";
  }
  return this->Superclass::GetShortName(key);
}

//----------------------------------------------------------------------------
void vtkSMCinemaVolumetricImageExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
