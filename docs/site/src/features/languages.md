Supported Languages And Platforms
=================================

Mongrel2 uses ZeroMQ and by using that it supports a ton of programming languages
and platforms, but more importantly supports them all in the *same* way with
the *same* configuration.

Programming Languages
---------------------

Mongrel2 fans have written quite a few handlers already for many different languages.  Here's the
list of currently supported languages and platforms, in alphabetical order:

* C++: <a href="http://github.com/akrennmair/mongrel2-cpp">mongrel2-cpp</a>
* C: <a href="https://github.com/derdewey/mongrel2_c_handler/">mongrel2_c_handler</a>
* Clojure: <a href="http://github.com/mikejs/ring/tree/master/ring-mongrel2-adapter/">ring-mongrel2-adapter</a>
* Common Lisp: <a href="http://github.com/vseloved/cl-mongrel2">cl-mongrel2</a> and <a href="https://github.com/galdor/m2cl">m2cl</a>
* Scheme: <a href="http://wiki.call-cc.org/eggref/4/mongrel2">Mongrel2 for Chicken Scheme</a>
* Haskell: <a href="http://github.com/cmoore/web-mongrel2">web-mongrel2</a>
* Java: <a href="https://github.com/kwo/mojaha">MoJaHa</a> and <a href="https://github.com/asinger/mongrel2j">Mongrel2J</a>
* Lua: <a href="http://github.com/jsimmons/mongrel2-lua/">mongrel2-lua</a>
* .NET: <a href="http://github.com/AustinWise/m2net">m2net</a>
* PHP: <a href="http://github.com/winks/m2php">m2php</a>
* Perl: <a href="http://github.com/lestrrat/Plack-Handler-Mongrel2">Plack-Handler-Mongrel2</a> and <a href="https://github.com/jrockway/anyevent-mongrel2/">AnyEvent-Mongrel2</a>
* Python: <a href="http://wsgid.com">wsgid</a> and <a href="http://github.com/berry/Mongrel2-WSGI-Handler">Mongrel2-WSGI-Handler (old?)</a>
* Ruby: <a href="http://github.com/perplexes/m2r">m2r</a> and <a href="http://github.com/darkhelmet/rack-mongrel2">rack-mongrel2</a>
* Node.js: <a href="https://github.com/dan-manges/m2node">m2node</a>.


Frameworks
----------

Mongrel2 works with man WSGI, Rack, Plack and similar framework
middleware systems, so most framework should operate with it out of the box.
In addition to that, there's Mongrel2 *specific* frameworks that
don't require any middleware and take advantage of Mongrel2 new 
protocol features:

* <a href="http://tir.mongrel2.org/">Tir</a> is a Lua framework written to show how to do direct frameworks with Mongrel2.
* <a href="http://github.com/j2labs/brubeck">Brubeck</a> is an asynchronous, non-blocking web framework written in Python.
* <a href="https://github.com/daogangtang/bamboo">Bamboo</a> is a Tir derivative with a bunch more features.
* <a href="http://www.photon-project.com">Photon</a> is a PHP framework similar to Tir and *way* faster than PHP.

<p>It is very easy to create a handler, you should read the <a href="http://mongrel2.org/manual/book-final.html">manual about handlers</a> for 
information on writing one for your language.</p>


Examples
--------


Soon...
