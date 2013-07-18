#!/usr/bin/perl

use strict;
use warnings;

use Digest::MD5;

while (my $line = <>) {
    chomp($line);
    my ($md5, $len, $c) = split / /, $line;
    if (!defined $len || !defined $c || $md5 ne Digest::MD5::md5_hex($c)) {
        die "checksum ERROR\n$line";
    }
}

print "checksum OK\n";
