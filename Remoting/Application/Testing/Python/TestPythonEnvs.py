# Tests the paraview.envs module (uv-backed virtual environments for
# pvpython): creates an environment from a requirements.txt, checks it is
# listed, runs a script inside it with --use and finally removes it.
# The test is skipped when no `uv` executable is available.

import os
import shutil
import subprocess
import sys
import tempfile

VTK_SKIP_RETURN_CODE = 125

if shutil.which("uv") is None:
    print("'uv' executable not found on PATH, skipping.")
    sys.exit(VTK_SKIP_RETURN_CODE)

tmpdir = tempfile.mkdtemp(prefix="pv-envs-home-")

# Isolate paraview.envs storage (~/.config/ParaView on unix,
# %APPDATA%/ParaView on Windows) inside the test's temporary directory.
env = dict(os.environ)
env["HOME"] = tmpdir
env["APPDATA"] = tmpdir

pvpython = shutil.which(sys.orig_argv[0])
env_name = "pv-envs-test"


def envs(*args):
    cmd = [pvpython, "--dr", "--", "-m", "paraview.envs"] + list(args)
    print("Running:", " ".join(cmd))
    result = subprocess.run(cmd, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, universal_newlines=True)
    print(result.stdout)
    assert result.returncode == 0, \
        "'%s' exited with %d" % (" ".join(cmd), result.returncode)
    return result.stdout


try:
    # Depend on a small, pure-Python package so the test also checks that
    # `uv pip install -r requirements.txt` actually populated the venv, not
    # just that `uv venv` created one.
    requirements = os.path.join(tmpdir, "requirements.txt")
    with open(requirements, "w") as f:
        f.write("six==1.16.0\n")

    marker = os.path.join(tmpdir, "marker.txt")
    script = os.path.join(tmpdir, "check_env.py")
    with open(script, "w") as f:
        f.write("import os\n"
                "def main():\n"
                "    try:\n"
                "        import six\n"
                "        six_path = os.path.abspath(six.__file__)\n"
                "    except ImportError:\n"
                "        six_path = 'MISSING'\n"
                "    with open(%r, 'w') as f:\n"
                "        f.write(os.environ.get('VIRTUAL_ENV', '') + '\\n' + six_path)\n"
                % marker)

    # Create an environment and check it is listed.
    envs("--create", env_name, requirements)
    assert env_name in envs("--list"), \
        "environment '%s' not found in --list output" % env_name

    # Run a script inside the environment; it records VIRTUAL_ENV and where
    # `six` got imported from.
    envs("--use", env_name, "--", script)
    assert os.path.exists(marker), \
        "script did not run inside the environment"
    with open(marker) as f:
        virtual_env, six_path = f.read().splitlines()
    assert env_name in virtual_env, \
        "VIRTUAL_ENV (%r) does not point to environment '%s'" \
        % (virtual_env, env_name)
    # `six` may also be importable from pvpython's own bundled site-packages,
    # so check it resolved to the *venv's* copy, proving `uv pip install`
    # actually installed the requirement rather than failing silently.
    assert env_name in six_path, \
        "'six' was not installed inside environment '%s' (imported from %r "\
        "instead)" % (env_name, six_path)

    # Remove the environment and check it is gone.
    envs("--remove", env_name)
    assert env_name not in envs("--list"), \
        "environment '%s' still listed after --remove" % env_name
finally:
    shutil.rmtree(tmpdir, ignore_errors=True)
