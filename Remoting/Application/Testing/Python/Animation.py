# Test to exercise the ParaView Python Animation API

from paraview.simple import *

from paraview import smtesting
smtesting.ProcessCommandLineArguments()

filename = smtesting.DataDir + '/Testing/Data/can.ex2'
can_ex2 = OpenDataFile(filename)

AnimationScene1 = GetAnimationScene()
AnimationScene1.EndTime = 0.004299988504499197
AnimationScene1.PlayMode = 'Snap To TimeSteps'

can_ex2.PointVariables = ['ACCL', 'DISPL', 'VEL']
can_ex2.ElementVariables = ['EQPS']
can_ex2.GlobalVariables = ['KE', 'NSTEPS', 'TMSTEP', 'XMOM', 'YMOM', 'ZMOM']
can_ex2.ApplyDisplacements = 0
can_ex2.ElementBlocks = ['Unnamed block ID: 1 Type: HEX Size: 4800', 'Unnamed block ID: 2 Type: HEX Size: 2352']

RenderView1 = GetRenderView()
DataRepresentation1 = Show()

RenderView1.CameraPosition = [0.21706008911132812, 4.0, 46.629626614745732]
RenderView1.CameraFocalPoint = [0.21706008911132812, 4.0, -5.1109471321105957]
RenderView1.CameraParallelScale = 13.391445890217907
RenderView1.CenterOfRotation = [0.21706008911132812, 4.0, -5.1109471321105957]

Shrink1 = Shrink()

CameraAnimationCue1 = GetCameraTrack()
CameraAnimationCue1.Mode = 'Path-based'

TimeAnimationCue1 = GetTimeTrack()

KeyFrame873 = CameraKeyFrame( FocalPathPoints=[0.21706, 4.0, -5.1109499999999999], FocalPoint=[0.21706008911132812, 4.0, -5.1109471321105957], PositionPathPoints=[49.66236, 4.0, -5.1109499999999999, 31.045707518817604, 42.657886443137336, -5.1109499999999999, -10.785536331926442, 52.205607205455664, -5.1109499999999999, -44.331603945406023, 25.45353660283736, -5.1109499999999999, -44.331631988993045, -17.453478369714951, -5.1109499999999999, -10.785599345302717, -44.205592823045158, -5.1109499999999999, 31.0456569859913, -34.657926741682985, -5.1109499999999999], ClosedPositionPath=1, Position=[0.21706008911132812, 55.740573746856327, -5.1109471321105957], ViewUp=[0.0, 0.0, 1.0] )
KeyFrame873.KeyTime = 0.0

KeyFrame875 = CameraKeyFrame( Position=[0.21706008911132812, 55.740573746856327, -5.1109471321105957], ViewUp=[0.0, 0.0, 1.0], FocalPoint=[0.21706008911132812, 4.0, -5.1109471321105957] )
KeyFrame875.KeyTime = 1.0

RenderView1.CameraViewUp = [0.0, 0.0, 1.0]
RenderView1.CameraPosition = [0.21706008911132812, 55.740573746856327, -5.1109471321105957]
RenderView1.CameraFocalPoint = [0.21706008911132812, 4.0, -5.1109471321105957]

CameraAnimationCue1.KeyFrames = [ KeyFrame873, KeyFrame875 ]

DataRepresentation2 = Show()
DataRepresentation2.ScalarOpacityUnitDistance = 1.3643905269222947
DataRepresentation2.Texture = []
DataRepresentation2.EdgeColor = [0.0, 0.0, 0.50000762951094835]

DataRepresentation1.Visibility = 0

KeyFrameAnimationCue1 = GetAnimationTrack( 'ShrinkFactor' )

KeyFrame1330 = CompositeKeyFrame( KeyTime=0.0 )

KeyFrame1335 = CompositeKeyFrame( KeyTime=1.0, KeyValues=[1.0] )

KeyFrameAnimationCue1.KeyFrames = [ KeyFrame1330, KeyFrame1335 ]

KeyFrame1330.KeyValues = [0.40000000000000002]

KeyFrame1335.KeyValues = [1.0]

Sphere1 = Sphere()

Sphere1.Radius = 5.0

DataRepresentation3 = Show()
DataRepresentation3.EdgeColor = [0.0, 0.0, 0.50000762951094835]

KeyFrameAnimationCue2 = GetAnimationTrack( 'Opacity' )

KeyFrame1734 = CompositeKeyFrame(KeyTime=0)

KeyFrame1739 = CompositeKeyFrame( KeyTime=1.0, KeyValues=[1.0] )

KeyFrameAnimationCue2.KeyFrames = [ KeyFrame1734, KeyFrame1739 ]

Render()

AnimationScene1.GoToFirst()
Render()
AnimationScene1.GoToNext()
Render()
AnimationScene1.GoToLast()
Render()
AnimationScene1.GoToPrevious()
Render()
AnimationScene1.GoToNext()
Render()

if not smtesting.DoRegressionTesting(RenderView1.SMProxy):
  # This will lead to VTK object leaks.
  import sys
  sys.exit(1)
