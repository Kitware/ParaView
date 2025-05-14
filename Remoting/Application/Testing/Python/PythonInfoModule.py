from paraview.info import env, runtime_env, build_env

environment = env()
build_environment = build_env()
runtime_environment = runtime_env()

# check for some keys that we know exist for sure
assert "ParaView Version" in environment.keys()
assert "OpenGL Vendor" in environment.keys()


assert "ParaView Version" in build_environment.keys()
assert "OpenGL Vendor" in runtime_environment.keys()
