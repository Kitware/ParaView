# trace generated using paraview version 5.11.0
import paraview

paraview.compatibility.major = 5
paraview.compatibility.minor = 12

from paraview.simple import *
from paraview import smtesting

smtesting.ProcessCommandLineArguments()


def CheckPVSM():
    LoadState(
        smtesting.DataDir + "/Testing/Data/BackwardCompatibility/LegacyStreakLine.pvsm"
    )
    sources = GetSources()
    for proxy_name, proxy in sources.items():
        if proxy.GetXMLName() == "LegacyStreakLine":
            return True
    print("State file should have loaded LegacyStreakLine")
    return False


assert CheckPVSM()

wavelet1 = Wavelet(registrationName="Wavelet1")
gradient1 = Gradient(registrationName="Gradient1", Input=wavelet1)
generateTimeSteps1 = GenerateTimeSteps(
    registrationName="GenerateTimeSteps1", Input=gradient1
)
generateTimeSteps1.TimeStepValues = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0]
pointSource1 = PointSource(registrationName="PointSource1")
streakLine1 = StreakLine(
    registrationName="StreakLine1", Input=generateTimeSteps1, SeedSource=pointSource1
)

assert streakLine1.GetXMLName() == "LegacyStreakLine"
