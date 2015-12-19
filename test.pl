use strict;
use warnings;
use File::Find;
use File::Spec;
use Data::Dumper;
my $isWin = $^O =~ /win32/i;

my $command = $isWin ? 'como ' : './como.sh ';
my @tests;
my @errors;

print Dumper \@ARGV;


my $testFolder = "./tests";

if (@ARGV){
    $testFolder .= "/" . $ARGV[0];
}

find(sub {
    if ($_ =~ m/\.js$/){
        my $file = $File::Find::name;
        push @tests, $file;
    }
}, $testFolder);

if (!$ARGV[0] || $testFolder =~ /uv/){
    my $pid = fork();
    die "can't fork echo server" if $pid == -1;
    if ($pid == 0){
        my $ret = system($command . "tests/uv/echo-server.js");
        print $ret, "\n";
        exit(0);
    }
}

for my $test (@tests){

    if ($test =~ /echo-server\.js/){
        next;
    }

    #ignore fixtures folder
    if ($test =~ /fixtures/){
        next;
    }

    print "Testing .. $test \n\n";
    my $ret = system($command . $test);
    print "Done Testing .. $test \n\n";
    print "===========================\n\n";
    if ($ret > 0){
        push @errors, $test;
        print "===========================\n";
    }
}

print "Done Testing " . scalar @tests . " files \n";
if (@errors > 0){
    print "With " . scalar @errors . " Error\n";
    foreach my $err ( @errors ){
        print "\t .." . $err . "\n";
    }
    exit(1);
} else {
    print "All Tests Success\n";
}
