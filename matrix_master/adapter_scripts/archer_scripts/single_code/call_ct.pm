#!/usr/bin/perl -w

package call_ct;
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
    @EXPORT=qw(handle_ct);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;

sub handle_ct {
    my $conf = shift;
    my $cmd = "cd ct && sh ./deployCT.sh";

    if (not -e "./src/noahdes/ct.des") {
        print_notice("[NOTICE][CALL_CT]","no ct.des so will not handle it!!!", __FILE__, __LINE__);
        return 0;
    }else {
        $cmd .= " -c ".$g_tmp_basedir."src/noahdes/ct.des";
    }
    if (exists($conf->{'SCM_MODULE'}) && !($conf->{'SCM_MODULE'} eq "") ) {
        $cmd .= " -m ".$conf->{'SCM_MODULE'};
    }else {
        print_error("[Fail][CALL_CT]","can not call CT,no service name found!!!", __FILE__, __LINE__);
        return -1;
    }
    if (exists($ENV{'CONTAINER_PATH'}) && !($ENV{'CONTAINER_PATH'} eq "") ) {
        $cmd .= " -p ".$ENV{'CONTAINER_PATH'};
    }else {
        print_error("[Fail][CALL_CT]","no container path!!!", __FILE__, __LINE__);
        return -1;
    }

    # must in jail
    $cmd .= " -j";

    my $ret = 0;
    if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
        print "[Check][execute:$cmd]\n";
    }else {
        print "[Check][execute:$cmd]\n";
        $ret = system("$cmd 2>&1");
    }
    if ($ret != 0) {
        $ret = $ret >> 8;
        print_error("[Fail][CALL_CT]","handle_ct Failed because:$ret", __FILE__, __LINE__);
        return -1;
    }
}
