#!/usr/bin/perl -w
package download_bin;

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
    @EXPORT=qw(handle_bin);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;
use lib::download_bin_and_data;

sub handle_bin{
    my $conf = shift;
    my $ret;
    if(not -e "./src/noahdes/bin.des"){
        print_notice("[Notice][DOWNLOAD_BIN]","no bin.des so will not handle it!",__FILE__,__LINE__);
        return 0;
    }
    if(exists($conf->{'DIFF'}) && $conf->{'DIFF'} eq 'TRUE'){
        $ret = download_bin_and_data($conf,'bin');
    }
    else{
        $ret = deal_bin($conf);
    }
    if($ret ne 0){
        print_error("[Fail][DOWNLOAD_BIN]","download_bin Failed!",__FILE__,__LINE__);
        return -1;
    }
    return 0;
}

sub deal_bin{
    my $conf = shift;
    my $cmd;
    if (not -e "./src/noahdes/bin.des") {
        print_log("[Notice]no bin.des so will not handle it!!!\n", __FILE__, __LINE__);
        return 0;
    }
    if(exists($conf->{'YAML'})){
        $cmd = "cd lib && perl ./download_construct.pl -m bin";
        if (exists($conf->{'BIN_DESCRIBE'}) && !($conf->{'BIN_DESCRIBE'} eq "") ) {
            $cmd .= " -c ".$conf->{'BIN_DESCRIBE'};
        }
        else{
            $cmd .= " -c ../src/noahdes/bin.des";
        }

        if(exists($conf->{'DEPLOY_METHOD'}) && ($conf->{'DEPLOY_METHOD'} eq "ALL")){
            $cmd .= " -f 1";
        }
        else{
            $cmd .= " -f 0";
        }
        if (exists($conf->{'LIMIT_RATE'}) && !($conf->{'LIMIT_RATE'} eq "") ) { 
            $cmd .= " -l ".$conf->{'LIMIT_RATE'};
        }      
        if (exists($conf->{'TOP_DIR'}) && !($conf->{'TOP_DIR'} eq "") ) {
            $cmd .= " -p ".$conf->{'TOP_DIR'};
        }      
        else{
            print_error("[Notice]no topdir!!!\n", __FILE__, __LINE__);
            return -1;
        }
        print "===>$cmd\n";
        my $ret = system("$cmd 2>&1");
        if ($ret != 0) {
            print_error("[Fail]handle_bin failed because:$!\n", __FILE__, __LINE__);
            return -1;
        }
    }
    else{
        $cmd = "cd bin && python ./bin_construct";
        if (exists($conf->{'LIMIT_RATE'}) && !($conf->{'LIMIT_RATE'} eq "") ) {
            $cmd .= " -l ".$conf->{'LIMIT_RATE'};
        }
        my $ret = system("$cmd 2>&1");
        if ($ret != 0) {
            print_error("[Fail]handle_bin failed because:$!\n",__FILE__, __LINE__);
            return -1;
        }
    }
    return 0;
}

