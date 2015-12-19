use strict;
use warnings;
use Data::Dumper;

use lib './scripts';
require 'helper.pl';

my $dll;
if ( isWin() ){
    $dll = 'duktape.dll';
} else {
    $dll = 'libduktape.so';
}

my $duksource = getFile('../libs/duktape/duktape.c');

if (!-f $duksource){
    die;
}

print Dumper $duksource;

system('gcc -c ' . $duksource) && die $!;
system('gcc -shared duktape.o -o ' . $dll) && die $!;

unlink 'duktape.o';

# print Dumper $duk;
