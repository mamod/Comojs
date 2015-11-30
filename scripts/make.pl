use strict;
use warnings;
use Data::Dumper;
use File::Spec;
use File::Copy;
use lib './scripts';
require 'helper.pl';

if (isWin()){
    system("gcc -c -w include/ansi/ANSI.c");
}

my @files = (
    getFile("../include/duktape/duktape.c"),
    getFile("../include/thread/tinycthread.c"),
    getFile("../include/http/http_parser.c"),
    getFile("../src/loop/core.c"),
);

my $mbedFolder = '../include/mbedtls/';
opendir(DIR, getFile( $mbedFolder . 'library')) or die $!;
while (my $file = readdir(DIR)) {
    next if ($file eq "." || $file eq ".." || $file eq "mbedtls");
    my $f = getFile( $mbedFolder . 'library/' . $file);
    push @files, $f;
}

my $build = join ' ', @files;

my $buildStr = "gcc -Wall -Werror -Wno-missing-braces"
             . " -I./include"
             . " -I./include/mbedtls/library"
             . " -L."
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
#              . " -I./include"
#              . " -I./include/mbedtls/library"
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
