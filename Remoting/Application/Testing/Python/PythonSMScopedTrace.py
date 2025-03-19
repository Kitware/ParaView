from paraview.simple import *
from paraview import smtrace
from paraview import print_error

def testScopedTrace():
    tracer = smtrace.ScopedTracer()

    with tracer:
        Sphere()

    sphere_trace = tracer.last_trace()
    first_sphere_creation = """sphere1 = Sphere(registrationName='Sphere1')"""
    if not first_sphere_creation in sphere_trace:
        print_error("sphere trace does not contains the sphere creation:")
        print(sphere_trace)
        print("\n")

    with tracer:
        tracer.config.SetSkipRenderingComponents(True)
        Sphere()
        Show()
        Render()

    no_render_trace = tracer.last_trace()

    if first_sphere_creation in no_render_trace:
        print_error("Second trace should not contain code from the previous trace")
        print(no_render_trace)
        print("\n")

    if not """UpdatePipeline(time=0.0, proxy=sphere2)""" in no_render_trace:
        print_error("trace without render should call UpdatePipeline")
        print(no_render_trace)
        print("\n")

testScopedTrace()
