// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
//
// Regression test to handle reentrancy in vtkPVView::CallProcessViewRequest().
//
// A representation's update can trigger a nested vtkPVView::Update() call on
// the same view. This happens for real when a queued mouse event for an
// active interactive 3D widget gets delivered while a slow filter's progress
// callback is pumping the Qt event loop. The widget's Render() call then
// routes back into vtkPVView::Update().
//
// vtkPVView::CallProcessViewRequest() shares one vtkInformation object
// (vtkPVView::RequestInformation) across every representation in a pass. The
// nested call clobbers the VIEW() key on that shared object. Every
// representation processed after the nested call returns then finds VIEW()
// missing from inInfo.
//
// vtkPVView::CallProcessViewRequest() should refuse a reentrant call outright,
// silently. This test verifies that VIEW() was never missing from the outer pass.

#include "vtkInformation.h"
#include "vtkInitializationHelper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVView.h"
#include "vtkProcessModule.h"
#include "vtkSMSession.h"

#include <iostream>

namespace
{
// vtkPVSession::Activate()/DeActivate() are protected; a derived class can
// still reach them since they are inherited, so expose them here to make the
// session active without going through the full SM proxy/ExecuteStream
// machinery.
class TestSession : public vtkSMSession
{
public:
  static TestSession* New();
  vtkTypeMacro(TestSession, vtkSMSession);
  using vtkSMSession::Activate;
  using vtkSMSession::DeActivate;

protected:
  TestSession() = default;
  ~TestSession() override = default;

private:
  TestSession(const TestSession&) = delete;
  void operator=(const TestSession&) = delete;
};
vtkStandardNewMacro(TestSession);

// A minimal, concrete vtkPVView that does no actual rendering so this test
// can exercise vtkPVView::Update()/CallProcessViewRequest() without needing a
// render window.
class TestView : public vtkPVView
{
public:
  static TestView* New();
  vtkTypeMacro(TestView, vtkPVView);

  void StillRender() override {}
  void InteractiveRender() override {}

protected:
  TestView()
    : vtkPVView(/*create_render_window=*/false)
  {
  }
  ~TestView() override = default;

private:
  TestView(const TestView&) = delete;
  void operator=(const TestView&) = delete;
};
vtkStandardNewMacro(TestView);

// Stands in for a representation whose update reenters the owning view's
// update pass, the way an interactive widget's Render() call does.
class ReentrantRepresentation : public vtkPVDataRepresentation
{
public:
  static ReentrantRepresentation* New();
  vtkTypeMacro(ReentrantRepresentation, vtkPVDataRepresentation);

  int ProcessViewRequest(vtkInformationRequestKey* requestType, vtkInformation* vtkNotUsed(inInfo),
    vtkInformation* vtkNotUsed(outInfo)) override
  {
    // Trigger the nested Update() call exactly once. In the real bug, the
    // reentrant call comes from an *unrelated* widget-Render() path that
    // isn't itself in the representation list, so it does not recurse
    // further; a flag here reproduces that same "single reentry" shape
    // without needing an actual second view/widget.
    if (requestType == vtkPVView::REQUEST_UPDATE() && !this->Triggering)
    {
      this->Triggering = true;
      // Reenter the view's update pass while the outer pass is still in the
      // middle of processing representations.
      if (auto* view = vtkPVView::SafeDownCast(this->GetView()))
      {
        view->Update();
      }
      this->Triggering = false;
    }
    return 1;
  }

protected:
  ReentrantRepresentation() = default;
  ~ReentrantRepresentation() override = default;

private:
  bool Triggering = false;

  ReentrantRepresentation(const ReentrantRepresentation&) = delete;
  void operator=(const ReentrantRepresentation&) = delete;
};
vtkStandardNewMacro(ReentrantRepresentation);

// Records whether VIEW() was present on `inInfo` each time REQUEST_UPDATE is
// processed for it.
class ProbeRepresentation : public vtkPVDataRepresentation
{
public:
  static ProbeRepresentation* New();
  vtkTypeMacro(ProbeRepresentation, vtkPVDataRepresentation);

  bool SawMissingView = false;

  int ProcessViewRequest(vtkInformationRequestKey* requestType, vtkInformation* inInfo,
    vtkInformation* vtkNotUsed(outInfo)) override
  {
    if (requestType == vtkPVView::REQUEST_UPDATE())
    {
      if (!inInfo->Get(vtkPVView::VIEW()))
      {
        this->SawMissingView = true;
      }
    }
    return 1;
  }

protected:
  ProbeRepresentation() = default;
  ~ProbeRepresentation() override = default;

private:
  ProbeRepresentation(const ProbeRepresentation&) = delete;
  void operator=(const ProbeRepresentation&) = delete;
};
vtkStandardNewMacro(ProbeRepresentation);
}

extern int TestPVViewReentrantProcessViewRequest(int argc, char* argv[])
{
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT);
  TestSession* session = TestSession::New();
  vtkProcessModule::GetProcessModule()->RegisterSession(session);
  session->Activate();

  int retVal = 0;
  {
    vtkNew<TestView> view;
    vtkNew<ReentrantRepresentation> reentrantRepr;
    vtkNew<ProbeRepresentation> probeRepr;

    // ReentrantRepresentation must be processed before probeRepr so that its
    // nested Update() call completes -- and clears the shared
    // RequestInformation -- before the outer pass reaches probeRepr.
    view->AddRepresentation(reentrantRepr);
    view->AddRepresentation(probeRepr);

    view->Update();

    if (probeRepr->SawMissingView)
    {
      std::cerr << "ERROR: ProbeRepresentation observed a missing VIEW() key. A nested "
                   "vtkPVView::Update() call cleared the shared RequestInformation out from "
                   "under the outer, still-in-progress update pass."
                << std::endl;
      retVal = 1;
    }
  }

  session->DeActivate();
  session->Delete();
  vtkInitializationHelper::Finalize();
  return retVal;
}
