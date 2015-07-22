use warnings;
use strict;
use Data::Dumper;
use File::Spec;
use File::Copy;
use Cwd qw[abs_path cwd];
my $isWin = ($^O =~ /win32/i);
my $file = abs_path($0);

$file =~ s/(.*)\/(.*?)\.pl/$1/;

my $dir = File::Spec->canonpath( $file );
sub getFile {
    my $path = join '/', @_;
    $file = File::Spec->canonpath( $dir . '/' . $path );
    return $file;
}

sub isWin { $isWin; }

1;
