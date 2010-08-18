
try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup

config = {
    'description': 'zcov',
    'author': 'Daniel Dunbar',
    'url': 'http://minormatter.com/zcov/',
    'download_url': 'http://minormatter.com/zcov/',
    'author_email': '',
    'version': '0.2',
    'scripts': ['bin/zcov-genhtml', 
                'bin/zcov-merge', 'bin/zcov-scan',
                'bin/zcov-summarize'],
    'install_requires': [],
    'packages': ['zcov'],
    'package_data': {'zcov': [
        'data/js/sorttable.js',
        'data/js/sourceview.js',
        'data/style.css'
    ]},
    'name': 'zcov'
}

setup(**config)

