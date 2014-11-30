#!/usr/bin/perl -w

package call_control;
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
    @EXPORT=qw(handle_control execute_cmd);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;

sub handle_control {
    my ($conf, $operation) = @_;
    my $dest;
    my $cmd;
    
    if (exists($conf->{'TOP_DIR'})) {
        $dest = $conf->{'TOP_DIR'};
    }else {
        print_error("[Fail][CALL_CONTROL]","deploy_all Failed because:$!", __FILE__, __LINE__);
        print STDOUT "[Fail]Error Code: NDEC-995004\n";
        return -1;
    }
    my $control_dest = "$dest/matrix/bin/noah_control";
    if (exists($conf->{'CONTROL_PATH'})) {
        $control_dest = "$dest/".$conf->{'CONTROL_PATH'};
    }
    my $cmd;
    if (-e $control_dest) {
        $cmd = "cd $dest && $control_dest";
    }else {
        print_notice("[Notice][CALL_CONTROL]","No control so that need not control", __FILE__, __LINE__);
        return 0;
    }
    if (defined($operation) && $operation ne "" ) {
        $cmd .= " $operation";
    }
    my $ret = 0;
    if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
        print "[Check][execute:$cmd]\n";
    }else {
        print "Exec:$cmd\n";
        $ret = system("$cmd 2>&1");
    }
    
    if ($ret != 0) {
        print_error("[Fail][CALL_CONTROL]","handle_control Failed because:$!", __FILE__, __LINE__);
        print STDOUT "[Fail]Error Code: NDEC-995004\n";
        return -1;
    }
    return 0;
}

sub execute_cmd{
    my ($conf, $cmd_line) = @_;
    my $ret = 0;
    if (not exists($conf->{'ONLY_TEST'}) || $conf->{'ONLY_TEST'} eq "TRUE") {
        print_notice("[Notice][EXECUTE_CMD]","start to execute $cmd_line:", __FILE__, __LINE__);
        $ret = system("$cmd_line 2>&1");
    }
    else{
        print "[Check][execute cmd:$cmd_line]\n";
    }
    if (0 != $ret) {
        print_error("[Fail][EXECUTE_CMD]","execute $cmd_line Failed because:$!", __FILE__, __LINE__);
        print STDOUT "[Fail]Error Code: NDEC-995003 \n";
        return -1;
    }
    return 0;
}
