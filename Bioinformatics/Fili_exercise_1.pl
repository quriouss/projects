#!/usr/local/bin/perl
use strict;
use warnings;

open(IN, "<transmem_proteins.txt" ) or die "i can not open the transmem_proteins.swiss, $!";
open(OUT, ">results.txt" );

my ($id, $ac, $aa, $sequence, $transmem_str ) = ( "", "", "", "", "" );    # initialize values
my @tm_table = ();      # initialize table

while (<IN>) {

     if ( $_ =~ m/^ID\s{3}(.+?)\s+/) { # first pattern
        $id = $1;
     }

     if ($_ =~ m/^AC\s{3}(.*?)\;/) {   # second pattern
        $ac = $1;
     }

     if ($_ =~ m/^FT\s+TRANSMEM\s+(\d+)\s+(\d+)\s+/) {   # third pattern
       my ($tm_start, $tm_end) = ($1, $2);
       my $tm_length = $tm_end - $tm_start;
       push @tm_table, [$tm_start, $tm_length];
     }


     if ( $_ =~ m/^SQ\s+SEQUENCE\s+(\d+)\sAA;/) {        # fourth pattern
        $aa = $1;
        $transmem_str = "-" x $aa;

        foreach my $tm (@tm_table) {
            my ($tm_start, $tm_length) = ($tm->[0], $tm->[1]);
            substr( $transmem_str, $tm_start, $tm_length, "M" x $tm_length );
        }

     }

     if ($_ =~ m/^\s{5}(.*)\n/) {                        # fifth pattern
        $sequence = $sequence . $1;
        $sequence =~ s/\s//g;
     }

     if ( $_ =~ /^\/\// ) {                              # sixth pattern
        print OUT ">$id|$ac|$aa" . "aa\n";
        print OUT "$sequence\n";
        print OUT "$transmem_str\n";
        print OUT"//\n";

        ($id, $ac, $aa, $sequence, $transmem_str ) = ( "", "", "", "", "" );     # free the values
        @tm_table = ();
     }
   
}