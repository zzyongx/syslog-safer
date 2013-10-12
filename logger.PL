#!/usr/bin/perl

use warnings;
use strict;
use Sys::Syslog qw(:DEFAULT);

# perl's syslog has bug, may lose leading domain on el5

my ($facility, $priority) = getopt();

openlog("", "", $facility);
while (my $log = <STDIN>) {
    syslog($priority, $log);
}
closelog;

sub getopt
{
    my ($facility, $priority) = ("user", "notice");
    for (my $i = 0; $i < @ARGV; $i++) {
        if ($ARGV[$i] eq "-p") {
            ($facility, $priority) = split(/\./, $ARGV[$i+1]);
        }
    }
    return ($facility, $priority);
}
