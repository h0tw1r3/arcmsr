Areca driver included with the linux kernel is old (and buggy), and doesn't support the ARC12x4 series cards.  Last update was apparently in 2010.

August 2013, Areca submitted an update to linux.kernel, but initially [gave up after being asked to clean up the patch][1].  As of May 2014, Areca has submitted a [nice patchset][2] for review, but it has not been accepted yet.

I am personally using this code to drive 8x 3TB WD Reds on an ARC-1280 with a 3.1+ kernel.  Motivated by the stock kernel driver consistantly stalling on my Dual Xeon X5450 server.  Updated driver works *much* better.

The areca branch contains commits for every version of the standalone module source available from ftp.areca.tw.

[1]: https://groups.google.com/forum/#!topic/linux.kernel/d112jJYRLUA
[2]: http://article.gmane.org/gmane.linux.scsi/90483/match=arcmsr+patch+v1.2+0+16
