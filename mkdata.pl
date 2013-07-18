#!/usr/bin/perl

use strict;
use warnings;

use Digest::MD5;

my $opt = getopt();

for (my $i = 0; $i < $opt->{num}; $i++) {
    my $size = int(rand($opt->{max})) + $opt->{min};

    my $s;
    for (my $j = 0; $j < $size; $j++) {
        $s .= int(rand(10));
    }

    my $md5 = Digest::MD5::md5_hex($s);

    print $md5, " ", length($s), " ", $s, "\n";
}

sub getopt
{
    my $opt = {
        min => 256,
        max => 80 * 8192,
        num => 10000,
    };

    foreach my $arg (@ARGV) {
        if ($arg =~ /--(\w+)=(.+)/) {
            $opt->{$1} = $2;
        }
    }

    return $opt;
}

