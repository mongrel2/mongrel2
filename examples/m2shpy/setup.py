
try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup

config = {
    'description': 'm2sh command line utility for Mongrel2 - python version',
    'author': 'Zed A. Shaw',
    'url': 'http://pypi.python.org/pypi/mongrel2-m2sh-python',
    'download_url': 'http://pypi.python.org/pypi/mongrel2-m2sh-python',
    'author_email': 'zedshaw@zedshaw.com',
    'version': '1.0beta7',
    'scripts': ['bin/m2sh'],
    'install_requires': ['m2py', 'pyrepl', 'storm'],
    'packages': ['mongrel2', 'mongrel2.config'],
    'package_data': {'mongrel2': ['sql/config.sql']},
    'name': 'm2sh'
}

setup(**config)

