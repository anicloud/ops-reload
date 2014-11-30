#! /usr/bin/perl -w
my @TMP_ARGV;
BEGIN {
    use Cwd 'realpath';
    $g_tmp_basedir = __FILE__;
    $g_tmp_basedir = realpath($g_tmp_basedir);

    $g_tmp_basedir =~ s/[^\/]+$//;
    unshift( @INC, $g_tmp_basedir );
    @TMP_ARGV = @ARGV;
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;
use alert;
use config;
use check_deploy_diff;

#use constant SCM_BASIC_PATH => 'ftp://getprod:getprod@getprod.scm.baidu.com/data/prod-aos/';
#use constant SCM_NOT_AOS_PATH => 'getprod@product.scm.baidu.com:/data/';

my $help;
my $test;
my $temfile;
my $sourcepath;
my $group;
my $serverlist;
my $topdir;
my $listid;
my $ifremote;
my $account = trim(`echo \$USER`);
my $backup_top_dir = "/home/$account/backup/";
my $kingo_logfile = "/dev/null";
my $home = trim(`echo \$HOME`);
my $ssh_key_files = "$home/.ssh/authorized_keys";
my $offline;
my $if_rollback;
my $fullamount;
my $control_path;
my $sourceHost;
my $ssh;
my $action;
my $if_use_scm;
my $module_name;
my $module_version;
my $module_build_platform;
my $module_tarswitch = 0;
my $limit_rate='3m';
my $machine_interval;
my $machine_timeout;
my $email_alertlist = '';
my $tele_alertlist = '';
my $sourceList;
my $is_yaml;
my $is_diff;
my $is_p2p;
my $cg_value;
my $deploylist;

#use constant CN_ZONE_DOMAIN => "ftp://cn.scm.noah.baidu.com:/home/work/data/archer/";
#use constant JP_ZONE_DOMAIN => "ftp://jp.scm.noah.baidu.com:/home/work/data/archer/";
#use constant HK_ZONE_DOMAIN => "ftp://hk.scm.noah.baidu.com:/home/work/data/archer/";
#use constant UPLOAD_CN_ZONE_DOMAIN =>"ftp://upload.cn.scm.noah.baidu.com:/home/work/data/archer"

Getopt::Long::GetOptions
(
    'help|h' => \$help,
    'serverlist=s' => \$serverlist,
    'control=s' => \$control_path,
    'sourcehost=s' => \$sourceHost,
    'ssh' => \$ssh,
    'remote' => \$ifremote,
    'test' => \$test,
    'offline' => \$offline,
    'fullamount' => \$fullamount,
    'rollback' => \$if_rollback,
    'listid=s' => \$listid,
    'f:s' => \$temfile,
    's:s' => \$sourcepath,
    'sourcelist=s' => \$sourceList,
    'g:s' => \$group,
    'd:s' => \$topdir,
    'scm' => \$if_use_scm,
    'module=s' => \$module_name,
    'version=s' => \$module_version,
    'buildplatform=s' => \$module_build_platform,
    'tarswitch=s' => \$module_tarswitch,
    'limit_rate=s' => \$limit_rate,
    'machineinterval=i' => \$machine_interval,
    'emailalertlist=s' => \$email_alertlist,
    'telealertlist=s' => \$tele_alertlist,
    'action=s' => \$action,
    'highVersion' => \$is_yaml,
    'diff' => \$is_diff,
    'p2p'=>\$is_p2p,
    'value=s'=>\$cg_value,
    'machinetimeout'=>\$machine_timeout,
    'deploylist=s'=>\$deploylist,
    'diffpath=s'=>\$deploylist,
);
sub set_diff_argv{
    my $argv = '';
    foreach my $param(@TMP_ARGV){
        $argv .= " $param";
    }
    return $argv;
}

sub set_sourcepath{
    my ($conf,$groupname,$listid,$moduleName,$is_multi,$sourcepath) = @_;
    my $tmp_zone;
    if(!defined($groupname) || $groupname eq ""){
        print_error("[Fail][main]","no group name\n",__FILE__,__LINE__);
        return -1;
    }
    if(not defined($conf->{'JP_ZONE_DOMAIN'}) ||
       not defined($conf->{'HK_ZONE_DOMAIN'}) ||
       not defined($conf->{'CN_ZONE_DOMAIN'}) ||
       not defined($conf->{'UPLOAD_CN_ZONE_DOMAIN'})){
        print_error("[Fail][main]","config in the config.pm is missing",__FILE__,__LINE__);
        return -1;
    }
    if(index($groupname,"BAIDU-JP") != -1){
        $tmp_zone = $conf->{'JP_ZONE_DOMAIN'};
    }
    elsif(index($groupname,"BAIDU-HK") != -1){
        $tmp_zone = $conf->{'HK_ZONE_DOMAIN'};
    }
    else{
        $tmp_zone = $conf->{'CN_ZONE_DOMAIN'};
        if(defined($is_p2p)){
            $tmp_zone = $conf->{'UPLOAD_CN_ZONE_DOMAIN'};
        }
    }
    $$sourcepath = $tmp_zone."/$listid/source_file.tar.gz";
    if(defined($is_multi) && $is_multi == 1){
        $$sourcepath = $tmp_zone."/$listid/$moduleName/source_file.tar.gz";
    }
    return 0;
}

sub get_group_name {
    my ($serverlist, $group_name) = @_;
    if (defined($group_name) && trim($group_name) ne "") {
        return 0;
    }
    if (!defined($serverlist) || trim($serverlist) eq "") {
        return -1;
    }
    my @lines = ( split "\n", $serverlist );
    my $host_name;
    my $ret = 0;
    $ret = get_local_host_name(\$host_name);
    if ($ret != 0) {
        return -1;
    }
    my $now_group;
    foreach my $one(@lines) {
        if ($one =~ m/[(.*?)]/) {
            $now_group = trim($1);
        }
        if ($one == $host_name && $now_group ne "") {
            $$group_name = $now_group;
            last;
        }
    }
    if ($$group_name eq "") {
        $$group_name = 'DEFAULT';
    }
    return 0;
}

sub get_backup_dir {
    my ($listid, $backup_dir) = @_;
    print_log("[Notice][main]","if remote:$ifremote", __FILE__, __LINE__);
    print_log("[Notice][main]","list id:$listid", __FILE__, __LINE__);
    if (defined($ifremote) && defined($listid) && $listid ne "") {
        my (undef, undef, undef, $mday, $mon, $year, undef, undef, undef) = localtime(time);
        my $today = sprintf("%d%d%d", $year + 1900, $mon + 1, $mday);
        $$backup_dir = trim($backup_top_dir)."/ci_backup/$today/$listid";
    }else {
        if (!defined($ifremote)) {
            my $time = get_now_tag();
            $$backup_dir = trim($backup_top_dir)."/ci_backup/$$".$time;
        }else {
            print_error("[Fail][main]","It is called by remote but no listid", __FILE__, __LINE__);
            return -1;
        }
    }

    return 0;
}

sub get_backup_log_dir {
    my ($listid, $backup_log_dir) = @_;
    print_log("[Notice][main]","if remote:$ifremote", __FILE__, __LINE__);
    print_log("[Notice][main]","list id:$listid", __FILE__, __LINE__);
    if (defined($ifremote) && defined($listid) && $listid ne "") {
        $$backup_log_dir = trim($backup_top_dir)."/ci_log/$listid";
    }else {
        if (!defined($ifremote)) {
            my $time = get_now_tag();
            $$backup_log_dir = trim($backup_top_dir)."/ci_log/$$".$time;
        }else {
            print_error("[Fail][main]","It is called by remote but no listid", __FILE__, __LINE__);
            return -1;
        }
    }

    return 0;
}

sub get_source {
    my ($sourcepath, $destpath) = @_;
    if (!defined($sourcepath) || trim($sourcepath) eq "") {
        print_error("[Fail][main]","no source path", __FILE__, __LINE__);
        return -1;
    }
    if (!defined($destpath) || trim($destpath) eq "") {
        $destpath = "./src";
    }
    print "[procedure][get the source file]\n";

    my $ret = 0;
    my $retryTimes = 3;
    my $retryInterval = 1;
    my $cmd;
    for (my $i = 0; $i < $retryTimes; $i++) {
        $ret = 0;
        if (-e 'source_file.tar.gz')    {
           if (0 != system("rm -rf source_file.tar.gz")) {
               print_error("[Fail][main]","$cmd failed", __FILE__, __LINE__);
               $ret = -1;
               next;
           }
        }
        $cmd = "wget --limit-rate=$limit_rate -q -O 'source_file.tar.gz' $sourcepath";
        if(defined($is_p2p)){
            my $source_path = format_kingo_url($sourcepath);
            if(not defined($source_path)){
                print_error("[Fail][main]","set kingo sourcepath failed!",__FILE__,__LINE__);
                return -1;
            }
            $cmd = "gkocp -d $limit_rate $source_path source_file.tar.gz >$kingo_logfile";
        }
        print_log("[Notice][main]","Start to download package...", __FILE__, __LINE__);
        print "$cmd\n\n";
        $ret = system($cmd);
        if ($ret != 0) {
            sleep($retryInterval);
            print_error("[Fail][main]","$cmd failed", __FILE__, __LINE__);
        }else {
            last;
        }
    }
    if (not -e 'source_file.tar.gz') {
        print_error("[Fail][main]","download source_file.tar.gz failed.", __FILE__, __LINE__);
        print STDOUT "[Fail]Error Code: NDEC-094002 \n";
        return -1;
    }
    if ($ret == 0) {
        if (not -d $destpath) {
            if (0 != system("mkdir $destpath")) {
                print_error("[Fail][main]","Cannot mkdir $destpath:$!", __FILE__, __LINE__);
                return -1;
            }
        }
        $cmd = "tar zxf source_file.tar.gz -C $destpath";
        $ret = system($cmd);
        if ($ret != 0) {
            print_error("[Fail][main]","$cmd failed", __FILE__, __LINE__);
        }
        if ($module_tarswitch){
            my @file_list;
            get_compressed_file_from_dir($destpath, \@file_list);
            for my $file (@file_list){
                my @param = ("tar", "zxf", "$file", "-C", $destpath);
                print_error("[Notice][main]","try to uncompress: tar zxf $file -C $destpath", __FILE__, __LINE__);
                my $ret = system(@param);
                if ($ret != 0){
                    print_error("[Warning][main]","tar zxf $file -C $destpath failed", __FILE__, __LINE__);
                }
                else {
                    @param = ("rm", "-f", "$file");
                    $ret = system(@param);
                    if ($ret != 0){
                        print_error("[Warning][main]","rm -f $file failed", __FILE__, __LINE__);
                    }
                }
            }
        }
    }
    return $ret;
}

sub get_direct_source {
    my ($sourcepath, $destpath) = @_;
    if (!defined($sourcepath) || trim($sourcepath) eq "") {
        print_error("[Fail][main]","no source path", __FILE__, __LINE__);
        return -1;
    }
    if (!defined($destpath) || trim($destpath) eq "") {
        $destpath = "./src";
    }
    print "[procedure][get the source file]\n";

    my $ret = 0;
    my $retryTimes = 3;
    my $retryInterval = 1;
    my $cmd;
    for (my $i = 0; $i < $retryTimes; $i++) {
        $ret = 0;
        if (-e 'source_file.tar.gz')    {
           if (0 != system("rm -rf source_file.tar.gz")) {
               print_error("[Fail][main]","$cmd failed", __FILE__, __LINE__);
               $ret = -1;
               next;
           }
        }
        #support scp.
        $sourcepath = trim($sourcepath);
        if ($sourcepath =~ /^ftp:\/\/|http:\/\//) {
            $cmd = "wget --limit-rate=$limit_rate -q -O source_file.tar.gz $sourcepath";
        }
        else {
            $cmd = "single_code/data/sshpass -p getprod scp -oStrictHostKeyChecking=no $sourcepath source_file.tar.gz";
        }

        if(defined($is_p2p)){
            my $source_path = format_kingo_url($sourcepath);
            if(not defined($source_path)){
                print_error("[Fail][main]","set kingo sourcepath failed!",__FILE__,__LINE__);
                return -1;
            }
            $cmd = "gkocp -d $limit_rate $source_path source_file.tar.gz >$kingo_logfile";
        }
        print_log("[Notice][main]","Start to download package...", __FILE__, __LINE__);
        print "$cmd\n\n";
        $ret = system($cmd);
        if ($ret != 0) {
            sleep($retryInterval);
            print_error("[Fail][main]","$cmd failed", __FILE__, __LINE__);
        }else {
            last;
        }
    }
    if (not -e 'source_file.tar.gz') {
        print_error("[Fail][main]","download source_file.tar.gz failed.", __FILE__, __LINE__);
        print STDOUT "[Fail]Error Code: NDEC-094002 \n";
        return -1;
    }
    if ($ret == 0) {
        if (not -d $destpath) {
            if (0 != system("mkdir $destpath")) {
                print_error("[Fail][main]","Cannot mkdir $destpath:$!", __FILE__, __LINE__);
                return -1;
            }
        }
        $cmd = "tar zxf source_file.tar.gz -C $destpath";
        $ret = system($cmd);
        if ($ret != 0) {
            print_error("[Fail][main]","$cmd failed", __FILE__, __LINE__);
        }

        # add by wfl
        `mkdir -p $destpath/matrix`;
        `mv instance.conf $destpath/matrix`;

        if ($module_tarswitch){
            my @file_list;
            get_compressed_file_from_dir($destpath, \@file_list);
            for my $file (@file_list){
                my @param = ("tar", "zxf", "$file", "-C", $destpath);
                print_error("[Notice][main]","try to uncompress: tar zxf $file -C $destpath", __FILE__, __LINE__);
                my $ret = system(@param);
                if ($ret != 0){
                    print_error("[Warning][main]","tar zxf $file -C $destpath failed", __FILE__, __LINE__);
                }
                else {
                    @param = ("rm", "-f", "$file");
                    $ret = system(@param);
                    if ($ret != 0){
                        print_error("[Warning][main]","rm -f $file failed", __FILE__, __LINE__);
                    }
                }
            }
        }
    }
    return $ret;
}

sub get_multi_module_conf {
    my ($sourceList, $modulesInfo) = @_;
    $sourceList = trim($sourceList);
    my @modules = split(",", $sourceList);
    my $i = 0;
    foreach my $moduleInfo (@modules) {
        my ($module, $sourceType, $version, $sourceDetail, $action, $deployPath, $control_path) = split ("#", $moduleInfo);
        my %moduleInfoHash = ();
        $i++;
        $modulesInfo->{$i} = \%moduleInfoHash;
        $modulesInfo->{$i}{'module'} = $module;
        $modulesInfo->{$i}{'sourceType'} = $sourceType;
        $modulesInfo->{$i}{'version'} = $version;
        $modulesInfo->{$i}{'sourceDetail'} = $sourceDetail;
        $modulesInfo->{$i}{'action'} = $action;
        $modulesInfo->{$i}{'deployPath'} = $deployPath;
        $modulesInfo->{$i}{'controlPath'} = $control_path;
    }
    return 0;
}

sub get_single_source {
    my ($conf,$version_file_path, $i) = @_;

    if(not defined($conf->{'SCM_NOT_AOS_PATH'})){
        print_error("[Fail][main]","SCM_NOT_AOS_PATH not defined in the config.pm",__FILE__,__LINE__);
        return -1;
    }
    my $single_path = $conf->{'SCM_NOT_AOS_PATH'}."/prod-64/op/oped/noah/tools/noah-deploy-single/noah-deploy-single_";
    my $module_single_path = "";
    my $fh;
    my $line;
    my $tmpline;
    my $ret = 0;
    if (!defined($i)){
        $i = '';
    }
    if (-e $version_file_path) {
        if (open($fh, $version_file_path)) {
            while($line = <$fh>) {
                chomp($line);
                my $tmpline = trim($line);
                my ($component, $version) = split(":", $tmpline);
                $component = trim($component);
                $version = trim($version);
                if ($component eq "" || $version eq "") {
                    print_error("[Fail][main]","version.des format error, wrong line is: $tmpline", __FILE__, __LINE__);
                    return -1;
                }
                $version =~ s/\./-/g;
                $version = $version."_PD_BL";
                print "[Source][component]:$component | [version]:$version\n";
                if ($component eq "single") {
                    $module_single_path = $single_path."$version"."/output";
                }else {
                    $module_single_path = $single_path."$version"."/output/$component";
                }

                if (-d 'tmp') {
                    if ($component eq "single") {
                        $ret = rm_dir('tmp');
                        print "[single] rm tmp $ret\n";
                    }
                }else {
                    if ($component ne "single") {
                        $ret = mk_path('tmp', 0755);
                        print "[not single] mkdir tmp $ret\n";
                    }
                }
                $ret = system("single_code/data/sshpass -p getprod scp -oStrictHostKeyChecking=no -r $module_single_path tmp");
                if ($ret != 0) {
                    print_error("[Fail][main]","download from scm failed", __FILE__, __LINE__);
                    return -1;
                }

                if ($component eq "single") {
                    if (not -e 'tmp/single_code') {
                        $ret = move_dir_to_dir("tmp","single_code$i");
                    }else {
                        $ret = move_dir_to_dir("tmp/single_code","single_code$i");
                        $ret = rm_dir('tmp');
                    }
                }else {
                    $ret = move_dir_to_dir("tmp/$component","single_code$i/$component");
                }
                if ($ret != 0) {
                    print_error("[Fail][main]","move $component from tmp dir to single_code$i failed", __FILE__, __LINE__);
                    return -1;
                }
            }
        }
    }
    return 0;
}

#后续可以删除
sub get_scm_source{
    my ($single_internal_conf,$scm, $module_name, $module_version, $module_build_platform, $module_tarswitch) = @_;
    if (!defined($scm) || !defined($module_name) || $module_name eq "" ||
        !defined($module_version) || $module_version eq "" ||
        !defined($module_build_platform) || $module_build_platform eq "")
    {
        print_error("[Fail][main]","no --scm or module_version or module_build_platform is set", __FILE__, __LINE__);
        return -1;
    }
    if(!defined($single_internal_conf->{'SCM_BASIC_PATH'}) && trim($single_internal_conf->{'SCM_BASIC_PATH'}) eq ''){
        print_error("[Fail][main]","SCM_BASIC_PATH info in the config.pm is missing!",__FILE__,__LINE__);
        return -1;
    }
    if ($module_version =~ m/^\d+-\d+-\d+-\d+$/) {
        print_error("[Fail][main]","Illegal version of module,must be like x.x.x.x", __FILE__, __LINE__);
        return -1;
    }
    if (!$module_build_platform eq '32' && !$module_build_platform eq '64') {
        print_error("[Fail][main]","Illegal build platform", __FILE__, __LINE__);
        return -1;
    }
    my $module_tail = get_filename_from_fullpath($module_name);

    my $tmp_PRODUCT = $module_tail . "_" . $module_version . ".tar.gz";

    my $product_dir;
    $module_version =~ s/\./-/g;
    $product_dir = $module_tail."_".$module_version."_PD_BL";

    my $real_source_path = realurl($single_internal_conf->{'SCM_BASIC_PATH'}."/prod-$module_build_platform/".$module_name."/".$product_dir ."/". $tmp_PRODUCT);
    print_log("[Notice][main]","Scm source path:$real_source_path", __FILE__, __LINE__);

    my $scm_file_tmp_path = "./scm_src";
    my $ret = system("rm -rf $scm_file_tmp_path");
    if ($ret != 0) {
        print_error("[Fail][main]","cannot delete $scm_file_tmp_path", __FILE__, __LINE__);
        return -1;
    }
    $ret = get_source($real_source_path, $scm_file_tmp_path);
    if ($ret != 0) {
        print_error("[Fail][main]","cannot download scm source file", __FILE__, __LINE__);
        return -1;
    }
    if ((not defined($module_tarswitch)) || $module_tarswitch eq "" || $module_tarswitch == 0) {
        print_log("[Notice][main]","Need not to depress the source", __FILE__, __LINE__);
    }else {
        my $path = "";
        my $extent_type = "";
        my @decompress_info = ();
        my $cmd;
        get_compressed_file_from_dir("$scm_file_tmp_path/output", \@decompress_info );
        foreach my $de_info (@decompress_info) {
            $path = $$de_info{'path'};
            $extent_type = $$de_info{'type'};
             # 因为需要解压缩，进入$scm_file_tmp_path/output目录，将tar.gz或 bz2解压缩
            if ($extent_type eq "tar.gz") {
                $cmd = "tar zxvfp $path 1>/dev/null 2>/dev/null";
            }elsif ($extent_type eq "tar.bz2" || $extent_type eq "bz2") {
                $cmd = "tar jxvf $path 1>/dev/null 2>/dev/null";
            }elsif ($extent_type eq "war") {
                $cmd = "unzip $path 1>/dev/null 2>/dev/null";
            }else {
                print "[Fail][格式解析错误,解析出类型为$extent_type,不支持]\n";
            }
    
            if (0 != system($cmd) ) {
                print "[Fail][解压缩产品失败 ][ $! ]\n";
                return -1;
            }else {
                $cmd = rm_file($path);
                if (0 != $cmd ) {
                    print "[Fail][解压缩产品失败 ][ $! ]\n";
                }
            }
        }
    }
    
    $ret = system("mv $scm_file_tmp_path/output ./src && rm -rf $scm_file_tmp_path");
    return $ret;
}

sub clean_all {
    if (0 != system("rm -rf single_code*/src")) {
        return -1;
    }
}

sub deploy_one_module {
    my ($conf,$module, $moduleInfo, $i) = @_;
    my $ret = 0;

    if (0 != system("cp single_code single_code$i -rf")){
        print_error("[Fail][main]","create single_code$i failed:$!");
        exit 1;
    }
    my $sourcepath;
    $ret = set_sourcepath($conf,$group,$listid,$module,1,\$sourcepath);
    if($ret != 0){
        print_error("[Fail][main]","Set source path Failed!",__FILE__,__LINE__);
        return -1;
    }
    $ret = get_source($sourcepath);
    if ($ret == 0) {
        $ret = get_single_source($conf,"src/noahdes/version.des", $i);
    }
    if ($ret != 0) {
        return $ret;
    }
    $ret = move_dir_to_dir("./src", "./single_code$i/src");
    if ($ret != 0) {
        return $ret;
    }
    my $final_cmd;
    my $conf_text = "";
    #$ret = get_group_name($serverlist, \$group);
    print_log("[Notice][main]","The group name:$group" , __FILE__, __LINE__);
    if ($ret != 0) {
        return $ret;
    }else {
        if (-e "single_code$i/src/noahdes/operations_map.pm") {
            if (0 != system("mv single_code$i/src/noahdes/operations_map.pm single_code$i/operations_map.pm")) {
                print_error("[Fail][main]","mv single_code$i/src/noahdes/operations_map.pm single_code$i/operations_map.pm failed:$!");
                exit 1;
            }
        }
        if (-e "single_code$i/src/noah-control.pl") {
            if (0 != system("mv single_code$i/src/noah-control.pl single_code$i/noah-control.pl")) {
                print_error("[Fail][main]","mv single_code$i/src/noah-control.pl single_code$i/noah-control.pl failed:$!");
                exit 1;
            }
        }
        $conf_text .= "YAML:TRUE\n";
        if (defined($test)) {
            $conf_text .= "ONLY_TEST:TRUE\n";
        }
        if(defined($is_p2p)){
            $conf_text .= "P2P:TRUE\n";
        }
        if (defined($group) && $group ne "") {
            $conf_text .= "GROUP_NAME:$group\n";
        }else {
            print_error("[Fail][main]","cannot get the group name", __FILE__, __LINE__);
            exit 1;
        }
        if (defined($moduleInfo->{'controlPath'}) && $moduleInfo->{'controlPath'} ne "") {
            $conf_text .= "CONTROL_PATH:".$moduleInfo->{'controlPath'}."\n";
        }
        if (defined($limit_rate)) {
            $conf_text .= "LIMIT_RATE:$limit_rate\n";
        }
        if (defined($machine_interval)) {
            $conf_text .= "MACHINE_INTERVAL:$machine_interval\n";
        }
        if (defined($moduleInfo->{'action'})) {
            $conf_text .= "ACTION:" . $moduleInfo->{'action'} . "\n";
        }else {
            $conf_text .= "ACTION:install\n";
        }
        if (defined($if_rollback)) {
            $conf_text .= "OPERATION:ROLLBACK\n";
        }else {
            $conf_text .= "OPERATION:DEPLOY\n";
        }
        if (defined($fullamount)) {
            $conf_text .= "DEPLOY_METHOD:ALL\n";
        }else {
            $conf_text .= "DEPLOY_METHOD:INCREASE\n";
        }

        my $backup_dir = "";
        $ret = get_backup_dir($listid, \$backup_dir);
        if ($ret == 0 && $backup_dir ne "") {
            $conf_text .= "BACKUP_DIR:$backup_dir\n";
        }else {
            print_error("[Fail][main]","get_backup_dir failed", __FILE__, __LINE__);
            exit 1;
        }
        my $backup_log_dir = "";
        $ret = get_backup_log_dir($listid, \$backup_log_dir);
        if ($ret == 0 && $backup_log_dir ne "") {
            $conf_text .= "BACKUP_LOG_DIR:$backup_log_dir\n";
        }else {
            print_error("[Fail][main]","get_backup_log_dir failed", __FILE__, __LINE__);
            exit 1;
        }
        if (defined($moduleInfo->{'deployPath'}) && $moduleInfo->{'deployPath'} ne "") {
            $conf_text .= "TOP_DIR:".$moduleInfo->{'deployPath'}."\n";
        }else {
            print_error("[Fail][main]","no destination of top dir of deploy files", __FILE__, __LINE__);
            exit 2;
        }
    
        if (defined($module) && $module ne "") {
            $conf_text .= "SCM_MODULE:$module\n";
        }else {
            $conf_text .= "SCM_MODULE:\n";
        }
    
        $conf_text .= 'EXTENSION_KEY:'.'<$'."\n";
        my $file_name = "noah-control.conf.$$";
        print "conf_text_before:\n$conf_text";
        open(FILE, ">$file_name");
        syswrite(FILE, $conf_text);
        close(FILE);
    
        $final_cmd = "cd single_code$i && perl noah-control.pl -c ../$file_name 2>&1";
        $ret = system($final_cmd);
        system("rm -f $file_name");
        system("rm -rf ./src");
    
        if ($ret != 0) {
            print_error("[Fail][main]","部署失败，请检查.Deploy failed, please check!!!\n",__FILE__,__LINE__);
            if ($email_alertlist ne "" || $tele_alertlist ne "") {
                fail_alert($email_alertlist, $tele_alertlist, $listid);
            }
            return 1;
        }
        return 0;
    }
}

sub checkSsh() {
    if (defined($sourceHost) && $sourceHost ne "") {
        my $localHost = "";
        if(get_local_host_name(\$localHost) != 0){
            return -1;
        }
        if ($sourceHost eq $localHost) {
             #本机
            return 0;
        }

        my @hostname = `host $sourceHost`;
        if ($#hostname == -1) {
            print_error("[Fail][main]","host $sourceHost failed!", __FILE__, __LINE__);
            return -1;
        }

        foreach (@hostname){
            m/.*\s([^\s]*\.baidu\.com).*/;
            if (open(FH, $ssh_key_files)) {
                my $accountAndHost = $account."@".$1;
                while (<FH>) {
                    if (index($_, $accountAndHost) != -1) {
                        #找到
                        return 0;
                    }
                }
            }
            else {
                print_error("[Fail][main]","can NOT open ssh key file [$ssh_key_files]!", __FILE__, __LINE__);
                return -1;
            }
        }
        print_error("[Fail][main]","can NOT find [$sourceHost] in ssh key file [$ssh_key_files]!", __FILE__, __LINE__);
        return -1;
    }
    else {
        #ssh方式必须指定源机器，一般不会进入，除非程序bug
        print_error("[Fail][main]","[Fail]sourceHost is empty!",__FILE__,__LINE__);
        return -1;
    }
}

#entrance, do authorization check first
$| = 1;
my $ret = 0;
if (defined($ssh)) {
    $ret = checkSsh();
}
if ($ret != 0) {
    exit $ret;
}

#获取本地的配置文件信息
my $single_internal_conf = get_basic_config();

$| = 1;
clean_all();
#智能增量下载逻辑
if(defined($is_diff)){
    my $argv = set_diff_argv();
    my $cmd = "perl noah-deploy-single-diff.pl $argv 2>&1";
    $ret = system($cmd);
    if($ret != 0){
        print_error("[Fail][main]","noah-deploy-single-diff.pl failed!",__FILE__,__LINE__);
        exit 1;
    }
    exit 0;
}

#非智能增量多模块下载逻辑
if (defined($sourceList) && trim($sourceList) ne '') {
    my %modulesInfo= ();
    my $i = 1;
    $ret = get_multi_module_conf($sourceList, \%modulesInfo);
    if ($ret != 0) {
        exit $ret;
    }
    foreach my $module_key(sort {$a<=>$b} keys %modulesInfo) {
        print_start("main");
        $ret = deploy_one_module($single_internal_conf,$modulesInfo{$module_key}{'module'}, $modulesInfo{$module_key}, $i);
        if ($ret != 0) {
            exit 1;
        }
        $i ++;
        print_start("end");
    }
    exit 0;
}

print_start("main");

## not use proxy to download source package.
#$ret = set_sourcepath($single_internal_conf,$group,$listid,$module_name,0,\$sourcepath);
#if($ret != 0){
#    print_error("[Fail][main]","set sourcepath failed!",__FILE__,__LINE__);
#    exit -1;
#}

$ret = get_direct_source($sourcepath);
if ($ret != 0) {
    $ret = get_scm_source($single_internal_conf,$if_use_scm, $module_name, $module_version, $module_build_platform, $module_tarswitch);
}

if ($ret == 0) {
    $ret = get_single_source($single_internal_conf,"src/noahdes/version.des");
}

if ($ret != 0) {
    exit $ret;
}

$ret = move_dir_to_dir("./src", "./single_code/src");
if ($ret != 0) {
    exit $ret;
}

my $final_cmd;
my $conf_text = "";
#$ret = get_group_name($serverlist, \$group);
print_log("[Notice][main]","The group name:$group" , __FILE__, __LINE__);
if ($ret != 0) {
    exit $ret;
}else {
    if (-e "single_code/src/noahdes/operations_map.pm") {
        if (0 != system("mv single_code/src/noahdes/operations_map.pm single_code/operations_map.pm")) {
            print_error("[Fail][main]","mv single_code/src/noahdes/operations_map.pm single_code/operations_map.pm failed:$!", __FILE__, __LINE__);
            exit 1;
        }
    }
    if (-e "single_code/src/noah-control.pl") {
        if (0 != system("mv single_code/src/noah-control.pl single_code/noah-control.pl")) {
            print_error("[Fail][main]","mv single_code/src/noah-control.pl single_code/noah-control.pl failed:$!", __FILE__, __LINE__);
            exit 1;
        }
    }
    if(defined($is_yaml)){
        $conf_text .="YAML:TRUE\n";
    }
    if (defined($test)) {
        $conf_text .= "ONLY_TEST:TRUE\n";
    }
    if (defined($fullamount)) {
        $conf_text .= "DEPLOY_METHOD:ALL\n";
    }else {
        $conf_text .= "DEPLOY_METHOD:INCREASE\n";
    }
    if (defined($group) && $group ne "") {
        $conf_text .= "GROUP_NAME:$group\n";
    }else {
        print_error("[Fail][main]","cannot get the group name", __FILE__, __LINE__);
        exit 1;
    }
    if (defined($control_path) && $control_path ne "") {
        $conf_text .= "CONTROL_PATH:$control_path\n";
    }
    if (defined($if_rollback)) {
        $conf_text .= "OPERATION:ROLLBACK\n";
    }else {
        $conf_text .= "OPERATION:DEPLOY\n";
    }
    if (defined($action)) {
        $conf_text .= "ACTION:$action\n";
    }else {
        $conf_text .= "ACTION:install\n";
    }
    if (defined($limit_rate)) {
        $conf_text .= "LIMIT_RATE:$limit_rate\n";
    }
    if (defined($machine_interval)) {
        $conf_text .= "MACHINE_INTERVAL:$machine_interval\n";
    }

    $conf_text .= "BACKUP_DIR:$topdir/matrix/archer_backup\n";

    #my $backup_dir = "";
    #$ret = get_backup_dir(123, \$backup_dir);
    #if ($ret == 0 && $backup_dir ne "") {
    #    $conf_text .= "BACKUP_DIR:$backup_dir\n";
    #}else {
    #    print_error("[Fail][main]","get_backup_dir failed", __FILE__, __LINE__);
    #    exit 1;
    #}

    #seems it's not used
    #my $backup_log_dir = "";
    #$ret = get_backup_log_dir($listid, \$backup_log_dir);
    #if ($ret == 0 && $backup_log_dir ne "") {
    #    $conf_text .= "BACKUP_LOG_DIR:$backup_log_dir\n";
    #}else {
    #    print_error("[Fail][main]","get_backup_log_dir failed", __FILE__, __LINE__);
    #    exit 1;
    #}

    if(defined($deploylist)){
        $ret = check_deploy_diff($single_internal_conf,$listid,\$topdir);
        if($ret != 0){
            print_error("[Fail][main]","check_deploy_diff failed!\n",__FILE__,__LINE__);
            exit 1;
        }
    }
    if (defined($topdir) && $topdir ne "") {
        $conf_text .= "TOP_DIR:$topdir\n";
    }else {
        print_error("[Fail][main]","no destination of top dir of deploy files", __FILE__, __LINE__);
        exit 2;
    }

    if (defined($module_name) && $module_name ne "") {
        $conf_text .= "SCM_MODULE:$module_name\n";
    }else {
        $conf_text .= "SCM_MODULE:\n";
    }

    $conf_text .= 'EXTENSION_KEY:'.'<$'."\n";
    my $file_name = "noah-control.conf.$$";
    print "conf_text_before:\n$conf_text";
    open(FILE, ">$file_name");
    syswrite(FILE, $conf_text);
    close(FILE);

    $final_cmd = "cd single_code && perl noah-control.pl -c ../$file_name 2>&1";
    $ret = system($final_cmd);
    system("rm -f $file_name");

    if ($ret != 0) {
        print_error("[Fail][main]","部署失败，请检查.Deploy failed, please check!!!\n",__FILE__,__LINE__);
        if ($email_alertlist ne "" || $tele_alertlist ne "") {
            fail_alert($email_alertlist, $tele_alertlist, $listid);
        }
        print_start("end");
        exit 1;
    }
    print_start("end"); 
    exit 0;
}
print_start("end"); 
exit 0;
