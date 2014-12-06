use strict;
use warnings;
use File::Find;
use File::Spec;
my $isWin = $^O =~ /win32/i;

my $command = $isWin ? 'como ' : './como.sh ';
my @tests;
my @errors;
find(sub {
    if ($_ =~ m/\.js$/){
        my $file = $File::Find::name;
        push @tests, $file;
    }
}, './tests');

for my $test (@tests){
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
