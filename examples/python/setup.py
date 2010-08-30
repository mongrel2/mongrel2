
try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup

config = {
    'description': 'Python Connector for Mongrel2',
    'author': 'Zed A. Shaw',
    'url': 'http://pypi.python.org/pypi/mongrel2-python',
    'download_url': 'http://pypi.python.org/pypi/mongrel2-python',
    'author_email': 'zedshaw@zedshaw.com',
    'version': '1.0beta5',
    'scripts': ['bin/m2sh'],
    'install_requires': ['nose', 'simplejson', 'pyrepl', 'storm'],
    'packages': ['mongrel2', 'mongrel2.config'],
    'package_data': {'mongrel2': ['sql/config.sql']},
    'name': 'm2py'
}

setup(**config)

