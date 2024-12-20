# /usr/bin/env python
from paraview.simple import *


def check_active(source, view):
    assert view is GetActiveView()
    assert source is GetActiveSource()


view_1 = CreateRenderView()
view_2 = CreateRenderView()
view_3 = CreateRenderView()

source_1 = Sphere()
source_2 = Cone()
source_3 = Box()

check_active(source_3, view_3)

SetActiveView(None)
SetActiveSource(None)

check_active(None, None)


with source_1, view_1:
    check_active(source_1, view_1)

    with source_2:
        check_active(source_2, view_1)

        with source_3:
            check_active(source_3, view_1)

        check_active(source_2, view_1)

    with view_2:
        check_active(source_1, view_2)

    check_active(source_1, view_1)

check_active(None, None)
