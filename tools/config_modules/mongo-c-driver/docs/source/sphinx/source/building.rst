Building the MongoDB C Driver
=============================

First checkout the version you want to build. *Always build from a particular tag, since HEAD may be
a work in progress.* For example, to build version 0.3, run:

.. code-block:: bash

    git checkout v0.4

Then follow the build steps below.

Building with SCons:
--------------------

This driver builds using the Python build utility, SCons_.
Make sure you've installed SCons, and then from the project root, enter:

.. _SCons: http://www.scons.org/

.. code-block:: bash

    scons

This will build static and dynamic libraries for both ``BSON`` and for the
the driver as a complete package. It's recommended that you build in C99 mode
with optimizations enabled:

.. code-block:: bash

    scons --c99

Once you're built the libraries, you can compile a program with ``gcc`` like so:

.. code-block:: bash

    gcc --std=c99 -Isrc src/*.c example.c

Platform-specific features
--------------------------

Certain behaviors, such as timeouts, have platform-specific implementations. All
that interacts with the OS and platform is located in ``platform/net.c``.
The default ``net.c`` is written to be as platform-generic as possible; it lacks
support for timeouts, for example.
However, the file ``platform/linux/net.c`` does implement timeouts
for Linux-based systems. You can compile the driver with this implementation like so:

.. code-block:: bash

    scons --use-platform=LINUX

You can write your own ``net.c`` to support platform-specific features by implementing
the interface defined in ``net.h``. Then name the files ``platform/custom/net.h`` and
``platform/custom.net.c`` and compile thusly:

.. code-block:: bash

    scons --use-platform=CUSTOM

Compile options with custom defines
----------------------------------

You can take advantage of special compile options by defining the following
constants at compile time:

For big-endian support, define:

- ``MONGO_BIG_ENDIAN``

If your compiler has a plain ``bool`` type, define:

- ``MONGO_HAVE_BOOL``

Alternatively, if you must include ``stdbool.h`` to get ``bool``, define:

- ``MONGO_HAVE_STDBOOL``

If you're not using C99, then you must choose your 64-bit integer type by
defining one of these:

- ``MONGO_HAVE_STDINT`` - Define this if you have ``<stdint.h>`` for int64_t.
- ``MONGO_HAVE_UNISTD`` - Define this if you have ``<unistd.h>`` for int64_t.
- ``MONGO_USE__INT64``  - Define this if ``__int64`` is your compiler's 64bit type (MSVC).
- ``MONGO_USE_LONG_LONG_INT`` - Define this if ``long long int`` is your compiler's 64-bit type.

Dependencies
------------

The driver itself has no dependencies, but one of the tests shows how to create a JSON-to-BSON
converter. For that test to run, you'll need JSON-C_.

.. _JSON-C: http://oss.metaparadigm.com/json-c/

Test suite
----------

Make sure that you're running mongod on 127.0.0.1 on the default port (27017). The replica set
test assumes a replica set with at least three nodes running at 127.0.0.1 and starting at port
30000. Note that the driver does not recognize 'localhost' as a valid host name.

To compile and run the tests:

.. code-block:: bash

    scons test

You may optionally specify a remote server:

.. code-block:: bash

    scons test --test-server=123.4.5.67

You may also specify an alternate starting port for the replica set members:

.. code-block:: bash

    scons test --test-server=123.4.5.67 --seed-start-port=40000

