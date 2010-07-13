
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
    'version': '0.1',
    'scripts': [],
    'install_requires': ['nose', 'simplejson'],
    'packages': ['mongrel2'],
    'name': 'mongrel2-python'
}

setup(**config)

