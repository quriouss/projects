#!/usr/local/bin/perl
use strict;
use warnings;

open( IN,  "<ADBR2_HUMAN.txt" ) or die "i can not open the transmem_proteins.swiss, $!";
open( OUT, ">results2.txt" );

# initialize values and tables
my ($sequence, $transmem_str, $condition ) = ( "", "", "" );
my @tm_table   = ();
my @hydrovalue = ();
my @table = ();

my %hyd = (      # hash udrofovikothtas
    R=>'-4.5',	
	K=>'-3.9',
	N=>'-3.5',
	D=>'-3.5',
	Q=>'-3.5',
	E=>'-3.5',
	H=>'-3.2',
	P=>'-1.6',
	Y=>'-1.3',
	W=>'-0.9',
	S=>'-0.8',
	T=>'-0.7',
	G=>'-0.4',
	A=>'1.8',
	M=>'1.9',
	C=>'2.5',
	F=>'2.8',
	L=>'3.8',
	V=>'4.2',
	I=>'4.5'
);
my %prognosis = (   # hash prognosis
    TP=>'0',	
	TN=>'0',
	FP=>'0',
	FN=>'0',
);
my $counter = 0;

print "Please enter the size of the window: ";
my $k = <STDIN>;

while (<IN>) {

    if ( $_ =~ m/^FT\s+TRANSMEM\s+(\d+)\.\.(\d+)/ ) {
        my ( $tm_start, $tm_end ) = ( $1, $2);
        my $tm_length = $tm_end - $tm_start;
        push @tm_table, [ $tm_start, $tm_length ];
    }

    if ( $_ =~ m/^SQ\s+SEQUENCE\s+(\d+)\sAA;/ ) {
        my $aa           = $1;
        $transmem_str = "-" x $aa;

		foreach my $tm (@tm_table) {
    		my ( $tm_start, $tm_length ) = ( $tm->[0], $tm->[1] );
    		substr( $transmem_str, $tm_start, $tm_length, "M" x $tm_length );
		}
    }

    if ( $_ =~ m/^\s+(.*)\n/ ) {
        $sequence = $sequence . $1;
        $sequence =~ s/\s//g;
    }

    if ( $_ =~ /^\/\// ) {
        for ( my $i = $k; $i < length($sequence) - $k; $i++ ) {
            my $q = 0;

            for ( my $j = $i - $k; $j <= $i + $k; $j++ ) {
                my $a = substr( $sequence, $j, 1 );

                # print OUT "$a";                  # prints the window

                my $p = $hyd{$a};                  # calculates the hydrophobicities of each amino acid 
                $q = $q + $p;                      # adds the hydrophobicities of all the amino acids in one window
            }

            $q = $q / ( 2 * $k + 1 );              # finds the mean value of the hydrophobicity in one window
            $hydrovalue[$i] = $q;                  # puts the mean value into an array and matches the mean value with the amino acid that is located in the certain of the window
            my $t = substr( $sequence, $i, 1 );    # finds the mean amino acid in a window

            my $m = substr($transmem_str, $counter + $k, 1);

            # calculate the TP, TN, FP, FN in order to use them later
            if ($m eq "M" && $hydrovalue[$i] ge "0") {      #TP
                $condition = "TP";
            } elsif ($m eq "-" && $hydrovalue[$i] lt "0") { #TN
                $condition = "TN";
            } elsif ($m eq "-" && $hydrovalue[$i] ge "0") { #FN
                $condition = "FN";
            } elsif ($m eq "M" && $hydrovalue[$i] lt "0") { #FP
                $condition = "FP";
            }

            $prognosis{$condition}++;

            print OUT "$i\t$t\t$hydrovalue[$i]\t$m\t$condition\n";   # prints the position of the amino acid, the mean value of the amino acid and the mean value of hydrophobicity of the window around this particular amino acid
            $counter++;
        }
    }
}

# Calculation of the accuracy and the Matthews correlation coefficient (MCC).
my $accuracy = ($prognosis{"TP"} + $prognosis{"TN"}) / ($prognosis{"TP"} + $prognosis{"FP"} + $prognosis{"TN"} + $prognosis{"FN"});
my $MCC = ($prognosis{"TP"} * $prognosis{"TN"} - $prognosis{"FP"} * $prognosis{"FN"}) / ((sqrt(($prognosis{"TP"} + $prognosis{"FP"}) * ($prognosis{"TP"} + $prognosis{"FN"}) * ($prognosis{"TN"} + $prognosis{"FP"}) * ($prognosis{"TN"} + $prognosis{"FN"}))));

print "The accuracy of the prognosis is :$accuracy\n";
print "The correlation factor of Matthews is :$MCC";