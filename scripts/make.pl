use strict;
use warnings;
use Data::Dumper;
use File::Spec;
use File::Copy;
use lib './scripts';
require 'helper.pl';

my %compileOptions;
my $options = '';
for (@ARGV){
    if ($_ =~ /^-D/){
        $options .= $_ . " ";
        $compileOptions{$_} = 1;
    }
}

my $cc = "gcc";

#FIXME: building ansi.c seperately
#since it produce errors
#maybe we need to write our own
if (isWin()){
    system("$cc -c -w libs/ansi/ANSI.c");
}

my @files = (
    getFile("../libs/duktape/duktape.c"),
    getFile("../libs/http/http_parser.c"),
    getFile("../src/loop/core.c")
);

if (!$compileOptions{'-DCOMO_NO_THREADS'}){
    push @files, getFile("../libs/thread/tinycthread.c");
}

#tls is not supported
my $mbedFolder = '../libs/mbedtls/';
if (!$compileOptions{'-DCOMO_NO_TLS'}){
    opendir(DIR, getFile( $mbedFolder . 'library')) or die $!;
    while (my $file = readdir(DIR)) {
        next if ($file eq "." || $file eq ".." || $file eq "mbedtls");
        my $f = getFile( $mbedFolder . 'library/' . $file);
        push @files, $f;
    }
} else {
    #if no tls support we still need some crypto & hashing
    push @files, getFile($mbedFolder . "library/md5.c");
    push @files, getFile($mbedFolder . "library/sha1.c");
    push @files, getFile($mbedFolder . "library/sha256.c");
    push @files, getFile($mbedFolder . "library/sha512.c");
}

my $build = join ' ', @files;

my $buildStr = "$cc -Wall -Werror -Wno-missing-braces"
             . " -I./libs"
             . " -I./libs/mbedtls/library"
             . " -L."
             . ($options ? " " . $options : "")
             . " " . $build
             . " -o como" . (isWin() ? ".exe" : "");
            if (isWin()){
                $buildStr .= " ANSI.o"
            } else {}

            $buildStr .= " src/main.c";

            if (isWin()){
                $buildStr .= " -lws2_32";
            } else {
                $buildStr .= " -lrt -lpthread -lm  -ldl";
            }

# my $buildStr = "gcc -Wall -Werror -Wno-missing-braces -shared "
#              . " -I./libs"
#              . " -I./libs/mbedtls/library"
#              . " -L."
#              . " " . $build
#              . " -o como" . (isWin() ? ".dll" : ".so");
#             if (isWin()){
#                 $buildStr .= " ANSI.o"
#             } else {}

#             $buildStr .= " src/main.c";

#             if (isWin()){
#                 $buildStr .= " -lws2_32";
#             } else {
#                 $buildStr .= " -lrt -lpthread -lm  -ldl";
#             }

system($buildStr) && die $!;
if (isWin()){ unlink "ANSI.o";}
1;
