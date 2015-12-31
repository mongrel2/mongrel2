Hacking On Mongrel2
===================

This is organized more like a FAQ than a guide, but it gives all the
information you probably need to hack on the Mongrel2 source code. It talks
about getting the code, doing your first patch, coding guidelines, etc. These
aren't meant to be strict rules but more guidelines to follow.

I want to hack on Mongrel2. How do I join?
--------

As our famous tagline says, "This isn't a hippie commune like Github." We love
patches and contributions from people, but Mongrel2 is a very specific kind of
C code project we have to keep the contributor list small and awesome. That
means we can't just let anyone in, we have to make you earn it. That doesn't
mean you're not awesome if you don't get in, it just means we make it a little
harder to join than your average github project does.
To get in on Mongrel2 you have to prove you can write code by writing some code
doing this:

1. Follow the instructions in this document.
2. Commit your changes to your repository when you think they're good.
3. Then go find a_ticket_to_do and write code to fix it.
4. Join the mongrel2@librelist.com mailing list or join #mongrel2 on
  irc.freenode.org.
5. Finally, one of us will check out your changes and see if they're good to
  pull. If they are, and you want to get into the project, then just fix a
  few more bugs and we'll let you in.

Each of these steps is documented in this document, so just read it real quick
and get a good understanding of the project before you continue.

How do I get the source code?
--------

The source code is currently maintained using git and the code itself is hosted
on github. The project is at [https://github.com/mongrel2/mongrel2](https://github.com/mongrel2/mongrel2).

The two branches available are:

* master
* develop

How do I find things to do?
--------

We have a tickets system here and also at github that you can access. You
can get a_ticket_to_do and write code to fix it. Obviously, you'll have to
probably anonymous_login first if you want to do very much.

How do I make a change?
--------

Git works like most version control systems in that you make your changes
and "commit" them.
Since you are probably not a committer yet, you just have to do this:


We are also using git-flow to streamline our development efforts. Our model
of using git-flow is described in [http://jeffkreeftmeijer.com/2010/why-arent-you-using-git-flow/](http://jeffkreeftmeijer.com/2010/why-arent-you-using-git-flow/).

And also as shown in [https://github.com/nvie/gitflow](https://github.com/nvie/gitflow).

Here are some quick instructions:

1. Install git-flow as in the README (https://github.com/nvie/gitflow).
2. In the mongrel2 run: git flow init
3. Just hit enter for all the defaults. Don't get fancy.
4. Start your hacking using: git flow feature start BlahBlahBlah
5. Commit like normal, don't push or tag anything.
6. End this feature with: git flow feature finish BlahBlahBlah
7. Finally: git push origin develop 

After you do this you just have to send a pull request via github and you 
are done.

If I become a contributor how do I get mentioned?
---------

Everyone who submits a change using this method will have their username
mentioned in the timeline even if you're not registered as a core contributor.
Core contributors get mentioned on the home page.
After you become a contributor you then are thrown bags of money and caviar.

How do you prioritize what to work on?
----------

We usually have a discussion on the mongrel2@librelist.com_mailing_list to
figure out what to do next. Then we fill in the tickets with the stuff to do.
Then we do that stuff.

Who comes up with the vision and direction?
---------

Usually it's Zed, who wrote the majority of the content for this site.

What will get my contribution rejected?
----------

Generally if your change adds too much code, is poorly written, doesn't work on
multiple platforms, or doesn't have testing when it needs it. Don't worry
though, we'll tell you how to clean it up and make it nice so that you learn
good style. As a starting point, here's what we consider our style guide:

What is your style guide?
-----------

1. Keep your code clean and "flatter" with good use of white space for readability.
2. Refactor common blocks of code or complex branches into functions, probably "static inline" is good.
3. Aim for lines around 80 characters if possible.
4. Check for errors from EVERY function using the check() macro from dbg.h.
5. Check for memory allocation working with the check_mem() macro from dbg.h.
6. Every if-elseif-else should have an else clause, and if that else shouldn't happen use the sentinel() macro to give an error.
7. Same thing for case-switch, always have a default with sentinel() if that default should not happen.
8. When in doubt, read and re-read the man page for function calls to make sure you got the error returns and parameters right.
9. Don't use the C string functions, use bstring from bstring.h to do string operations.
10. Try to write a test for your code, which is hard sometimes in C, but attempt it at least.

In general the theme for Mongrel2 source is, "Don't code like an asshole." If
you write a piece of code and you didn't consider how another person will use
it, or just didn't care, then it'll probably get rejected.

How do I learn more about Mongrel2?
-----------

Check out the <a href="/wiki/">the documentation</a> for more information.

How do I see what's been changed on a file or view a diff?
-----------

After you log in you can use the timeline_checkins_only to see the list of
changes. Then you pick one and there's various diffing options.
To see how a file changed, browse for the file in in_Files and find the one you
want to look at. You can then do advanced change logging and diffing of that
file.

The general way fossil works is that you get a lot of features from the command
line, but complex operations like analyzing diffs is better done in the Web
GUI. You can get to your own web GUI any time by doing fossil ui or fossil
serve.
