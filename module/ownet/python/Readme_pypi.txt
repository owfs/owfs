ownet on pypi
=============

The ownet module has been registered on the `Python Cheese Shop
<http://www.python.org/pypi>`__ to allow for easier access. The Python
Cheese Shop is the central location for finding and installing various
Python programs, modules, tools, etc.


Updating ownet on pypi
----------------------

1) Register the latest version

$ python setup.py register       
running register
Using PyPI login from /Users/peter/.pypirc
Server response (200): OK
$


2) Upload the latest version

$ python setup.py sdist upload
running sdist
reading manifest file 'MANIFEST'
creating ownet-0.2
creating ownet-0.2/examples
creating ownet-0.2/ownet
making hard links in ownet-0.2...
hard linking README.txt -> ownet-0.2
hard linking setup.py -> ownet-0.2
hard linking examples/check_ow.py -> ownet-0.2/examples
hard linking examples/temperatures.py -> ownet-0.2/examples
hard linking ownet/__init__.py -> ownet-0.2/ownet
hard linking ownet/connection.py -> ownet-0.2/ownet
tar -cf dist/ownet-0.2.tar ownet-0.2
gzip -f9 dist/ownet-0.2.tar
removing 'ownet-0.2' (and everything under it)
running upload
Submitting dist/ownet-0.2.tar.gz to http://www.python.org/pypi
Server response (200): OK
$

