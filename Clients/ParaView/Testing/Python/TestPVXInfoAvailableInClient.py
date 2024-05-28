from paraview import servermanager
from paraview import simple


# Make sure the test driver know that process has properly started
print ("Process started")


def getHost(url):
   return url.split(':')[1][2:]


def getPort(url):
   return int(url.split(':')[2])


def runTest():
    options = servermanager.vtkRemotingCoreConfiguration.GetInstance()
    url = options.GetServerURL()

    simple.Connect(getHost(url), getPort(url))

    simple.Cone()
    simple.Show()
    renderView = simple.Render()

    assert renderView.GetIsInCAVE()

    # Ensure we correctly get the values from "../XML/LeftRight.pvx"

    lower_left_d0 = renderView.GetLowerLeft(0)
    assert len(lower_left_d0) == 3
    assert lower_left_d0[0] == -2
    assert lower_left_d0[1] == -1
    assert lower_left_d0[2] == -2

    lower_right_d0 = renderView.GetLowerRight(0)
    assert len(lower_right_d0) == 3
    assert lower_right_d0[0] == 0
    assert lower_right_d0[1] == -1
    assert lower_right_d0[2] == -2

    upper_right_d0 = renderView.GetUpperRight(0)
    assert len(upper_right_d0) == 3
    assert upper_right_d0[0] == 0
    assert upper_right_d0[1] == 1
    assert upper_right_d0[2] == -2

    lower_left_d1 = renderView.GetLowerLeft(1)
    assert len(lower_left_d1) == 3
    assert lower_left_d1[0] == 0
    assert lower_left_d1[1] == -1
    assert lower_left_d1[2] == -2

    lower_right_d1 = renderView.GetLowerRight(1)
    assert len(lower_right_d1) == 3
    assert lower_right_d1[0] == 2
    assert lower_right_d1[1] == -1
    assert lower_right_d1[2] == -2

    upper_right_d1 = renderView.GetUpperRight(1)
    assert len(upper_right_d1) == 3
    assert upper_right_d1[0] == 2
    assert upper_right_d1[1] == 1
    assert upper_right_d1[2] == -2

    simple.Disconnect()


if __name__ == "__main__":
    runTest()
