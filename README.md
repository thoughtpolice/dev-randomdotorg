# Linux kernel module for random.org

Is `/dev/urandom` just not good enough for you, and `/dev/random` just
sits in your base, stealing all your entropy? Yeah, I feel that way
too (except not *really*, but maybe you've been there!)

To that end, I've written a Linux kernel module that provides an
interface to [random.org](http://random.org), which is a service that
provides psuedo-random data based on samples of atmospheric noise. Now
all your software randomness can be provided by the skies themselves!

This module has been developed and tested primarily on **Ubuntu
12.04** which is sporting a **Linux 3.2 kernel**, but patches for
other distributions and kernel versions are certainly welcome!

# Installation

Clone the code, then just run:

```bash
./configure # will need perl
make
```

And afterwords you should now have a shiny `randomdotorg.ko` kernel
module! Load it with `insmod`:

    $ sudo insmod randomdotorg.ko

And make sure it's there:

    $ lsmod | grep randomdotorg
	randomdotorg           12432  0
	$ 

Now you should have a special device called `/dev/randomdotorg` that's
similar to `/dev/urandom` or `/dev/random` but powered by the chaos of
the atmosphere!

# Rational, Security and other caveats

This is obviously just a silly joke driver, mostly intended as a
simple example of a linux driver.

You should not use this module for generating secure random
numbers. Continue to use `/dev/random` or `/dev/urandom` for that (why
would you push secure random numbers off to a service?)

This driver does not use SSL. That would be ridiculous but as a result
you can be MITM'd, only reinforcing the point this shouldn't be used
for any secret kind of randomness.

That said, maybe you will find it educational or silly.

# Join in

File bugs in the GitHub [issue tracker][].

Master [git repository][gh]:

* `git clone https://github.com/thoughtpolice/dev-randomdotorg.git`

There's also a [BitBucket mirror][bb]:

* `git clone https://bitbucket.org/thoughtpolice/dev-randomdotorg.git`

# Authors

See [AUTHORS.txt](https://raw.github.com/thoughtpolice/dev-randomdotorg/master/AUTHORS.txt).

# License

Technically I release this software under the MIT license, although
`randomdotorg.c`'s `MODULE_LICENSE` field says "Dual MIT/GPL." This is
mostly because the kernels' module loading facilities don't recognize
"MIT" alone as a valid module license, and I want the module to be
warning free when building and loading. Maybe this is a stupid
concern, but nonetheless you could easily incorporate this code into
your own project under the GPL without any issue.

See
[LICENSE.txt](https://raw.github.com/thoughtpolice/dev-randomdotorg/master/LICENSE.txt)
for terms of copyright and redistribution.

[issue tracker]: http://github.com/thoughtpolice/dev-randomdotorg/issues
[gh]: http://github.com/thoughtpolice/dev-randomdotorg
[bb]: http://bitbucket.org/thoughtpolice/dev-randomdotorg
