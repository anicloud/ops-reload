#!/usr/bin/perl -w

package operations_map;
my $g_tmp_basedir;
BEGIN {
    use Cwd 'realpath';
    $g_tmp_basedir = __FILE__ ;
    $g_tmp_basedir = realpath($g_tmp_basedir);
    $g_tmp_basedir =~ s/[^\/]+$//;

    unshift(@INC, $g_tmp_basedir);
    use Exporter();
    use vars qw($VERSION @ISA @EXPORT);
    @ISA=qw(Exporter);
    @EXPORT=qw(init_operations_map);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;
use backup_and_deploy;
use call_control;
use derive_conf;
use download_bin;
use download_data;
use blacklist_tool;
use call_ct;
use rollback;
use call_mon;

sub init_operations_map {
    my $conf = shift;
    
    my %operations_map = ();
    
    $operations_map{'DERIVE_CONF'} = \&handle_conf;
    $operations_map{'DOWNLOAD_BIN'} = \&handle_bin;
    $operations_map{'DOWNLOAD_DATA'} = \&handle_data;
    $operations_map{'BACKUP_AND_DEPLOY'} = \&backup_and_deploy_all;
    $operations_map{'CALL_CONTROL'} = \&handle_control;
    $operations_map{'CALL_CT'} = \&handle_ct;
    $operations_map{'EXECUTE_CMD'} = \&execute_cmd;
    $conf->{'operations_map'} = \%operations_map;
    return 0;
}
