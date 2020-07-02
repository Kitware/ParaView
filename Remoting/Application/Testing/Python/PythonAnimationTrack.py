from paraview import smtesting
from paraview.simple import *
import sys

Sphere()
Show()
Render()

AnimationScene1 = GetAnimationScene()
PythonAnimationCue1 = PythonAnimationCue()
PythonAnimationCue1.Script= """
from paraview.simple import *

def start_cue(self):
  print("Start Cue")

def tick(self):
  GetActiveSource().StartTheta = 180 * self.GetAnimationTime()
  Render()

def end_cue(self):
  print("end")
"""
AnimationScene1.Cues.append(PythonAnimationCue1)
AnimationScene1.Play()

AnimationScene1.GoToFirst()
AnimationScene1.GoToNext()
AnimationScene1.GoToNext()
AnimationScene1.GoToNext()

# delete the cue
AnimationScene1.Cues = []
Delete(PythonAnimationCue1)
del PythonAnimationCue1

if not smtesting.DoRegressionTesting(GetActiveView().SMProxy):
  # This will lead to VTK object leaks.
  sys.exit(1)
