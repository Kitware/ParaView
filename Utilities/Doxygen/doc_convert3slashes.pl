#!/usr/bin/env perl
#
# This program converts the /// style doxygen comments to /**  */ style comments.
# This is being used on the Qt subdirectory of the ParaView source tree for consistency
# with the rest of ParaView which is being changed to match VTK.
#

use Carp;
use Getopt::Long;
use File::Find;
use File::Basename;
use File::Path;

my $PROGNAME = $0;

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "help", "to=s", "relativeto=s");

my %default =
  (
   relativeto => "",
   to => "/tmp/doxygenoutput"
  );


if (exists $args{"help"}) {
    print <<"EOT";
Usage: $PROGNAME [--help] [--to path] [--relativeto path] [files|directories...]
  --help            : this message
  --to path         : use 'path' as destination directory (default: $default{to})
  --relativeto path : each file/directory to document is considered relative to 'path', where --to and --relativeto should be absolute (default: $default{relativeto})
EOT
    exit;
}

$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};
$args{"relativeto"} = $default{"relativeto"} if ! exists $args{"relativeto"};
$args{"relativeto"} =~ s/[\\\/]*$// if exists $args{"relativeto"};

my $encoding = ":encoding(UTF-8)";

my @files;
foreach my $file (@ARGV) {
    if (-f $file) {
        push @files, $file;
    } elsif (-d $file) {
        find sub { push @files, $File::Find::name; }, $file;
    }
}

if ($#files == 0) {
  croak "No files selected";
}

foreach my $source (@files) {
    next if $source !~ /[^\\\/]*\.h\Z/;
    my $dest;
    if (! exists $args{"to"}) {
        $dest = "doc_convert3slashes.tmp";
    } else {
        # if source has absolute path, just use the basename, unless a
        # relativeto path has been set
        if ($source =~ m/^(\/|[a-zA-W]\:[\/\\])/) {
            if ($args{"relativeto"}) {
                my ($dir, $absrel) = (abs_path(dirname($source)),
                                      abs_path($args{"relativeto"}));
                $dir =~ s/$absrel//;
                $dest = $args{"to"} . $dir . '/' . basename($source);
            } else {
                $dest = $args{"to"} . '/' . basename($source);
            }
        } else {
            my $source2 = $source;
            # let's remove the ../ component before the source filename, so
            # that it might be appended to the "to" directory
            $source2 =~ s/^(\.\.[\/\\])*//;
            $dest = $args{"to"} . '/' . $source2;
        }
        # Ensure both source and target are different
        if (!$os_is_win) {
            my ($i_dev, $i_ino) = stat $source;
            my ($o_dev, $o_ino) = stat $dest;
            croak "$PROGNAME: sorry, $source and $dest are the same file\n"
              if ($i_dev == $o_dev && $i_ino == $o_ino);
        }
    }

    open(HEADERFILE, "< $encoding", $source)
      or croak "$PROGNAME: unable to open $source\n";
    my @headerfile = <HEADERFILE>;
    close(HEADERFILE);

    my @converted = ();
    my $line;
    while ($line = shift @headerfile) {
        if ($line =~ /^(\s*)\/\/\/+(.*)$/) {
            push @converted, "$1\/**\n";
            push @converted, "$1*$2\n";
            while ($line = shift @headerfile) {
                if ($line =~ /^(\s*)\/\/\/+(.*)$/) {
                    push @converted, "$1*$2\n";
                } else {
                    push @converted, "$1*/\n";
                    push @converted, $line;
                    last;
                }
            }
        } elsif ($line =~ /^(\s*)\/\*!(.*)$/) {
            push @converted, "$1\/**$2\n";
        } else {
            push @converted, $line;
        }
    }

    # Open the target and create the missing directory if any

    if (!open(DEST_FILE, "> $encoding", $dest)) {
        my $dir = dirname($dest);
        mkpath($dir);
        open(DEST_FILE, "> $encoding", $dest)
          or croak "$PROGNAME: unable to open destination file $dest\n";
    }
    print DEST_FILE @converted;
    close(DEST_FILE);

    # If in-place conversion was requested, remove source and rename target
    # (or temp file) to source

    if (! exists $args{"to"}) {
        unlink($source)
          or carp "$PROGNAME: unable to delete original file $source\n";
        rename($args{"temp"}, $source)
          or carp "$PROGNAME: unable to rename ", $args{"temp"}, " to $source\n";
    }
}
