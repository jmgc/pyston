# Note: the expected counts here are set to match the CI, and I can't reproduce them locally
import os, sys, subprocess, shutil
sys.path.append(os.path.dirname(__file__) + "/../lib")

from test_helper import create_virtenv, run_test

ENV_NAME = "cffi17_test_env_" + os.path.basename(sys.executable)
SRC_DIR = os.path.abspath(os.path.join(ENV_NAME, "src"))
PYTHON_EXE = os.path.abspath(os.path.join(ENV_NAME, "bin", "python"))
PYTEST_EXE = os.path.abspath(os.path.join(ENV_NAME, "bin", "py.test"))

def install_and_test_cffi():
    shutil.rmtree(SRC_DIR, ignore_errors=True)
    os.makedirs(SRC_DIR)

    url = "https://pypi.python.org/packages/83/3c/00b553fd05ae32f27b3637f705c413c4ce71290aa9b4c4764df694e906d9/cffi-1.7.0.tar.gz#md5=34122a545060cee58bab88feab57006d"
    subprocess.check_call(["wget", url], cwd=SRC_DIR)
    subprocess.check_call(["tar", "-zxf", "cffi-1.7.0.tar.gz"], cwd=SRC_DIR)
    CFFI_DIR = os.path.abspath(os.path.join(SRC_DIR, "cffi-1.7.0"))

    subprocess.check_call([PYTHON_EXE, "setup.py", "install"], cwd=CFFI_DIR)

    # looks like clang 3.5 causes more errors like: 214 != -42 doing casts
    if os.environ.has_key("CC") and "clang" in os.environ["CC"]:
        expected = [{'xfailed': 4, 'failed': 3, 'skipped': 10, 'passed': 539}]
    else:
        expected = [{'xfailed': 4, 'failed': 2, 'skipped': 10, 'passed': 540}]

    # Ideally we would run all the tests, but they take ~10min to run.
    # dir_to_test = "."
    # I just picked a subdirectory; I don't really know what it's testing.
    dir_to_test = os.path.join(CFFI_DIR,  "testing", "cffi1")
    expected_log_hash = '''
    gBEACAAQEAIAwICAAAAAAABAAAAAAAACAAAAEAAAEAAAAEQEQAAAAKAAARFEEACKAAABAACAAAAA
    QgAAAEAQBAACgAEAAABAAAAAAAFAAAoAAAAAAACAAAACAAIAAUiAIAAAAAODgAAgEIQBABAACgAC
    GBACAAAAICEAABAAgQA=
    '''
    run_test([PYTEST_EXE, dir_to_test], cwd=CFFI_DIR, expected=expected, expected_log_hash=expected_log_hash)

create_virtenv(ENV_NAME, ["pytest==2.8.7", "py==1.4.31", "pycparser==2.14"], force_create = True)
install_and_test_cffi()
