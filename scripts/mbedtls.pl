use lib './scripts';
use Data::Dumper;
use warnings;
use strict;
use File::Spec;
use File::Copy;
use Cwd qw[abs_path cwd];

require 'helper.pl';

my $dllfile = 'mbedtls.dll';
my $extra = "";
if (!isWin()){
    $dllfile = 'libmbedtls.so';
} else {
    $extra .= "-lws2_32";
}

my $mbedFolder = '../include/mbedtls/';

opendir(DIR, getFile( $mbedFolder . 'library')) or die $!;

print "Building mbedtls\n";

my @objects;
while (my $file = readdir(DIR)) {
    next if ($file eq "." || $file eq ".." || $file eq "mbedtls");

    my $f = getFile( $mbedFolder . 'library/' . $file);
    my $object = $file;
    $object = getFile( $mbedFolder . 'build/' . $object);

    $object =~ s/\.c/\.o/;
    system('gcc -Wall -c ' . $f . ' -o ' . $object) && die $!;
    push @objects, $object;
}

my $objects = join ' ', @objects;
my $build = "gcc -L. -shared -fPIC -Wl,-soname,$dllfile -o $dllfile " . $objects . " " . $extra;
system($build) && die $!;

for (@objects){
    unlink $_ or die $!;
}

1;
