Design Criticisms
=================

The idea for Mongrel2 is slightly controversial, but it has potential.  Here's
some criticisms about the proposed design, and I'm looking for more.


Criticism: Windows Registry
---------------------------

"Using sqlite for the config file will be like working with the windows registry."

Mongrel2 now has support for configuring itself from anything, including the
default of a sqlite3 database.  After a year of using Mongrel2 in production I can
tell you this is freaking awesome.  It's nothing like the Windows Registry and 
people telling you that are just trying to trot out an old Window$-Hater Linux
Guy trope to scare you.


Criticism: Config Files Are Better
----------------------------------

I work in a fully automated developer operations world, and have found
that no, configuration files are not better.  Configuration files suck
unless you're some jerk who likes editing them.  The rest of us generate
them from a Chef or Puppet server and get on with our lives.


Criticism: It should be event based, not use coroutines or threads.
-------------------------------------------------------------------

It is event based in that it uses kqueue/epoll/poll/select, but it abstracts
the complexity of event systems away using coroutines.  You get the best of
both worlds.


Criticism: Flash sucks, use something else.
--------------------------------

JSSockets work, and they're reliable.  When WebSockets is established and in at
least 2 browsers I'll add that as well.  Mongrel2 works with long polling but
it's not that big of a deal, it's just how Mongrel2 works.  It's only a big
deal in web servers that suck.

We'll be also releasing a version with the latest "hybi" implementation of 
WebSockets because they don't suck.

Criticism: It needs to serve files.
------------------------

It does serve files, and since obviously that's the first vanity metric
everyone uses, it will serve them fast.  It's just Mongrel2 will favor
*language agnostic* applications over simple file serving.  Actually, I'm
not sure why people thought it wouldn't.


Criticism: People hate SQL and won't want to use it to configure Mongrel2.
--------------------------------------------------------------

They don't use SQL, they use a tool called *m2sh* or use any programming language
they want with any database they want, even flat files.


Criticism: SQLite3 is not Diff/Git friendly so I can't use it.
--------------------------------------------------

The default *m2sh* command uses a config file, so diff away.  If you want to
then use only config files you can write a new backend that gets rid of 
sqlite3, or uses redis.  Whatever you want.

Criticism: SQLite3 will be hard to change quickly so I'll be stuck in an emergency situation.
--------

It's a SQL database, it's all about changes, and you won't use SQL directly,
so this is just plain wrong.  Not only will you be able to change it
quickly, but you'll be able to replicate those changes to all your
servers in one shot, roll them back, and lots of other great features.


Criticism: It will suck for developers because so much emphasis is on operations.
--------

No, it will obviously be for both, it'll just be focused on operations needs
before developer needs.  You will of course still get your "5 minute quick
start" and be able to fire up your application quickly, just like all the
competitors.

Criticism: SQLite cannot handle hierarchical configs like in NGinx.
--------------------------------------------------------

We have completely smashed this criticism in the face.  We have working
code that not only can load a full hierarchical config from SQLite, but
also load it *from anything else*.


