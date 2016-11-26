import zcov
import os

from setuptools import setup, find_packages

# setuptools expects to be invoked from within the directory of setup.py, but it
# is nice to allow:
#   python path/to/setup.py install
# to work (for scripts, etc.)
os.chdir(os.path.dirname(os.path.abspath(__file__)))

setup(
    name = "zcov",
    version = zcov.__version__,

    author = zcov.__author__,
    author_email = zcov.__email__,
    license = 'BSD',

    description = "A Code Coverage Reporting Tool for C/C++",
    keywords = 'code coverage C++ testing',
    long_description = """\
*zcov*
++++++

zcov is wrapper around the basic facilities of gcov for generating pretty
summaries for entire code bases. It is similar to lcov with an emphasis on nicer
HTML output (including, for example, branch coverage), and a greatly simplified
command line interface which tries to work around deficiencies in the metadata
provided by gcov.
""",

    packages = find_packages(),

    entry_points = {
        'console_scripts': [
            'zcov = zcov.main:main',
            ],
        },

    install_requires=[],
)
