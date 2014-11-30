#!/usr/bin/perl -w

package call_mon;
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
    @EXPORT=qw(handle_mon);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;

sub handle_mon {
    print_notice("[NOTICE][CALL_MON]","not support now.", __FILE__, __LINE__);
    return -1;

    if (not -e "./src/noahdes/mon.des") {
        print_notice("[NOTICE][CALL_MON]","no mon.des so will not handle it!!!", __FILE__, __LINE__);
        return 0;
    }

    my $monUrl = 'http://yx-testing-oped2008.vm.baidu.com:8090/noah/index.php?r=webfoot/ArcherNotice';
    my $containerId = $ENV{containerId};
    my $serviceName = $ENV{serviceName};
    my $version = $ENV{deployDescId};
    my $offset = $ENV{offset};
    my $instanceId = "$offset.$serviceName";
    my $deployPath = $ENV{deployPath};
    my $moduleDes = `cat ./src/noahdes/mon.des`;
    my $hostName = `hostname`;
    $hostName =~ s/\.baidu\.com$//g;

    my $tmpFile = "/tmp/archer_$instanceId";

    my @cmd = (
        'curl', '-s',
        '-d', "serviceName=$serviceName",
        '-d', "version=$version",
        '-d', "containerId=$containerId",
        '-d', "instanceId=$instanceId",
        '-d', "deployPath=$deployPath",
        '-d', "moduleDesc=$moduleDes",
        '-d', "hostName=$hostName",
        '-o', $tmpFile,
        $monUrl,
    );
    my $ret = system(@cmd);
    if ($ret){
        return -1;
    }

    $ret = `cat '$tmpFile'`;
    system('rm', '-rf', $tmpFile);
    print_notice("[NOTICE][CALL_MON]","mon return: $ret", __FILE__, __LINE__);
    $ret =~ s/ *:/:/g;
    $ret =~ s/: */:/g;
    if ($ret =~ /success:1/){
        return 0;
    }
    return -1;
}
