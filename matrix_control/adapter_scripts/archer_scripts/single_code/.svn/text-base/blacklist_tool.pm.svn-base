#!/usr/bin/perl -w

package blacklist_tool;
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
    @EXPORT=qw(block_alarm_blacklist unblock_alarm_blacklist);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;
use config;

sub block_alarm_blacklist{
    print_notice("[Notice][BLOCK]","Not support now. ", __FILE__, __LINE__);
    return -1;

    my ($conf, $is_block_alarm) = @_;
    if(defined($is_block_alarm) && $is_block_alarm ne "TRUE"){
        print_notice("[Notice][BLOCK]","No need to block alarm,BLOCK_ALARM conf is $is_block_alarm", __FILE__, __LINE__);
        return 0;
    }

#获取基础配置信息
    my $single_internal_conf = get_basic_config();
    if(!defined($single_internal_conf->{'BLOCK_HOST_URL'}) ||
            trim($single_internal_conf->{'BLOCK_HOST_URL'}) eq ''){
        print_error("[Fail][BLOCK_MACHINE]","undefined BLOCK_HOST_URL in the config.pm",__FILE__,__LINE__);
        return -1;
    }
    my $block_url = $single_internal_conf->{'BLOCK_HOST_URL'};
    
#获取本机机器名信息
    my $host_name = "";
    if(get_local_host_name(\$host_name) != 0){
        return -1;
    }

#调用block工具，屏蔽报警
    my $cmd = "curl -s -d \"hostName=$host_name\" -d \"notice=0\" $block_url" ;
    my $moduleccess = 0;
    my $i = 0;
    my $cmd_output;
    for (; $i<3; $i++) {
        $cmd_output = `$cmd`;
        if (trim($cmd_output) eq "OK!") {
            $moduleccess = 1;
            last;
        }else {
            sleep 3;
            next;
        }
    }
    if ($moduleccess == 1) {
        print_notice("[Notice][BLOCK]","$cmd\n Block tools show hostname $host_name $cmd_output", __FILE__, __LINE__); 
        return 0;
    }else {
        print_error("[Fail][BLOCK]","$cmd\n $cmd_output", __FILE__, __LINE__); 
        print STDOUT "[Fail]Error Code: NDEC-993001\n";
        return -1;
    }
}

sub unblock_alarm_blacklist{
    print_notice("[Notice][UNBLOCK]","Not support now. ", __FILE__, __LINE__);
    return -1;

    my ($conf, $is_block_alarm) = @_;
    if(defined($is_block_alarm) && $is_block_alarm ne "TRUE"){
        print_notice("[Notice][UNBLOCK]","No need to unblock alarm\n BLOCK_ALARM conf is $is_block_alarm", __FILE__, __LINE__);
        return 0;
    }

#获取基本配置信息
    my $single_internal_conf = get_basic_config();
    if(!defined($single_internal_conf->{'UNBLOCK_HOST_URL'}) || trim
            ($single_internal_conf->{'UNBLOCK_HOST_URL'}) eq ''){
        print_error("[Fail][UNBLOCK]","undefined UNBLOCK_HOST_URL in the config.pm",__FILE__,__LINE__);
        return -1;
    }
    my $unblock_url = $single_internal_conf->{'UNBLOCK_HOST_URL'};

#获取本机机器名信息
    my $host_name = "";
    if(get_local_host_name(\$host_name) != 0){
        return -1;
    }

    my $i = 0;
    my $moduleccess = 0;
    my $cmd = "curl -s -d \"hostName=$host_name\" -d \"notice=0\" $unblock_url";
    my $cmd_output;
    for(; $i<3; $i++){
        $cmd_output = `$cmd`;
        if ("OK!" eq trim($cmd_output)) {
            $moduleccess = 1;
            print_notice("[Notice][UNBLOCK]","$cmd\n Block tools show unblock ok:$cmd_output", __FILE__, __LINE__);
            last;
        }else {
            print_notice("[Notice][UNBLOCK]","$cmd\n Block tools show unblock Fail:$cmd_output", __FILE__, __LINE__);
            sleep 3;
            next;
        }
    }
    if($moduleccess == 0){
        print_error("[Fail][UNBLOCK]","unblock Fail", __FILE__, __LINE__);
        print STDOUT "[Fail]Error Code: NDEC-993001 \n";
        return -1;
    }else{
        print_notice("[Notice][UNBLOCK]","unblock ok", __FILE__, __LINE__);
        return 0;
    }
}
