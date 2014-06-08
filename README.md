Areca driver included with the linux kernel is old (and buggy), and doesn't support the ARC12x4 series cards.  Last update was apparently in 2010.

August 2013, Areca submitted an update to linux.kernel, but apparently [gave up after being asked to clean up their patch][1].

The source code in this repo has been merged from various Areca sources I've found.  While I believe it to be the latest available, Areca's release versioning leaves much to be desired.

I am personally using this code to drive 8x 3TB WD Reds on an ARC-1280 with a 3.1+ kernel.  Motivated by the stock kernel consistantly stalling on my Dual Xeon X5450 server.  Updated driver works much better (at least no more card resets).

[1]: https://groups.google.com/forum/#!topic/linux.kernel/d112jJYRLUA
