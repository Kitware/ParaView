// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// A pipeline that throws during Initialize or Execute is disabled, and the
// other pipelines keep running on later steps.

#include "vtkInSituInitializationHelper.h"
#include "vtkInSituPipeline.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
class TestPipeline : public vtkInSituPipeline
{
public:
  static TestPipeline* New();
  vtkTypeMacro(TestPipeline, vtkInSituPipeline);

  bool Initialize() override
  {
    ++this->Initializations;
    if (this->ThrowOnInitialize)
    {
      throw std::runtime_error("thrown by the test during Initialize");
    }
    return true;
  }

  bool Execute(int, double) override
  {
    ++this->Executions;
    if (this->ThrowOnExecute)
    {
      throw std::runtime_error("thrown by the test during Execute");
    }
    return true;
  }

  bool ThrowOnInitialize = false;
  bool ThrowOnExecute = false;
  int Initializations = 0;
  int Executions = 0;
};
vtkStandardNewMacro(TestPipeline);

bool Expect(const char* what, int actual, int expected)
{
  if (actual != expected)
  {
    std::cerr << what << ": " << actual << ", expected " << expected << "\n";
    return false;
  }
  return true;
}
}

int TestInSituPipelineException(int, char*[])
{
  vtkInSituInitializationHelper::Initialize(0, {});

  vtkNew<TestPipeline> throwsOnInitialize;
  throwsOnInitialize->SetName("ThrowsOnInitialize");
  throwsOnInitialize->ThrowOnInitialize = true;

  vtkNew<TestPipeline> throwsOnExecute;
  throwsOnExecute->SetName("ThrowsOnExecute");
  throwsOnExecute->ThrowOnExecute = true;

  vtkNew<TestPipeline> works;
  works->SetName("Works");

  for (TestPipeline* pipeline : { throwsOnInitialize.Get(), throwsOnExecute.Get(), works.Get() })
  {
    vtkInSituInitializationHelper::AddPipeline(pipeline);
  }
  const std::vector<std::string> names{ "ThrowsOnInitialize", "ThrowsOnExecute", "Works" };

  // The disabled pipelines are reported as errors on every step, which would
  // fail the test.
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_OFF);

  int escaped = 0;
  const int numberOfSteps = 3;
  for (int step = 0; step < numberOfSteps; ++step)
  {
    try
    {
      vtkInSituInitializationHelper::ExecutePipelines(step, step, names, {});
    }
    catch (...)
    {
      ++escaped;
    }
  }

  bool ok = true;
  ok &= Expect("exceptions escaping ExecutePipelines", escaped, 0);
  ok &= Expect("Initialize calls of ThrowsOnInitialize", throwsOnInitialize->Initializations, 1);
  ok &= Expect("Execute calls of ThrowsOnInitialize", throwsOnInitialize->Executions, 0);
  ok &= Expect("Execute calls of ThrowsOnExecute", throwsOnExecute->Executions, 1);
  ok &= Expect("Execute calls of Works", works->Executions, numberOfSteps);

  vtkInSituInitializationHelper::Finalize();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
