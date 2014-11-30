#! /usr/bin/perl -w
my $g_tmp_basedir;
BEGIN {
    use Cwd 'realpath';
    $g_tmp_basedir = __FILE__;
    $g_tmp_basedir = realpath($g_tmp_basedir);

    $g_tmp_basedir =~ s/[^\/]+$//;
    unshift( @INC, $g_tmp_basedir );
}

#perl的内置函式库
use strict;
use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

#程序自定义函式库
use lib_aos_utility;
use file_operations;
use lib::check_file;
use get_operations;
use lib::Tiny;
use alert;
use config;
use check_deploy_diff;

my $version;
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
my $limit_rate = '3m';
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

#常量定义
#use constant CN_ZONE_DOMAIN => "ftp://cn.scm.noah.baidu.com:/home/work/data/archer/";
#use constant JP_ZONE_DOMAIN => "ftp://jp.scm.noah.baidu.com:/home/work/data/archer/";
#use constant HK_ZONE_DOMAIN => "ftp://hk.scm.noah.baidu.com:/home/work/data/archer/";
#use constant UPLOAD_CN_ZONE_DOMAIN =>"ftp://upload.cn.scm.noah.baidu.com:/home/work/data/archer/";
#use constant SCM_NOT_AOS_PATH => 'getprod@product.scm.baidu.com:/data/';

#获取输入参数信息
Getopt::Long::GetOptions
(
    'version' => \$version,
    'serverlist=s' => \$serverlist,
    'control=s' => \$control_path,
    'sourcehost=s' => \$sourceHost,
    'ssh' => \$ssh,
    'remote' => \$ifremote,
    'test' => \$test,
    'fullamount' => \$fullamount,
    'rollback' => \$if_rollback,
    'listid:s' => \$listid,
    'f:s' => \$temfile,
    's:s' => \$sourcepath,
    'sourcelist=s' => \$sourceList,
    'g:s' => \$group,
    'd:s' => \$topdir,
    'scm' => \$if_use_scm,
    'module=s' => \$module_name,
    'buildplatform=s' => \$module_build_platform,
    'tarswitch=s' => \$module_tarswitch,
    'limit_rate=s' => \$limit_rate,
    'machineinterval=i' => \$machine_interval,
    'emailalertlist=s' => \$email_alertlist,
    'telealertlist=s' => \$tele_alertlist,
    'action=s' => \$action,
    'highVersion' => \$is_yaml,
    'diff' => \$is_diff,
    'p2p' => \$is_p2p,
    'value=s' =>\$cg_value,
    'machinetimeout=i'=>\$machine_timeout,
    'deploylist=s'=>\$deploylist,
    'diffpath=s'=>\$deploylist,
);

#根据部署的服务节点，获取中转机数据存放地址
sub set_sourcepath{
    my($single_conf,$module_conf) = @_;
    my $zone_address = '';
    my $sourcePath = '';
    my $groupname = '';

    #判断提供的服务节点名是否有效
    if(!defined($module_conf->{'group'}) || trim($module_conf->{'group'}) eq ""){
        print_error("[Fail][main]","no group name\n",__FILE__,__LINE__);
        return undef;
    }
    else{
        $groupname = $module_conf->{'group'};
    }

    #检验中转机信息的有效性
    if(!defined($single_conf->{'JP_ZONE_DOMAIN'}) || trim($single_conf->{'JP_ZONE_DOMAIN'}) eq "" ||
        !defined($single_conf->{'CN_ZONE_DOMAIN'}) || trim($single_conf->{'CN_ZONE_DOMAIN'}) eq "" ||
        !defined($single_conf->{'HK_ZONE_DOMAIN'}) || trim($single_conf->{'HK_ZONE_DOMAIN'}) eq "" || 
        !defined($single_conf->{'UPLOAD_CN_ZONE_DOMAIN'}) || trim($single_conf->{'UPLOAD_CN_ZONE_DOMAIN'}) eq ""){
        print_error("[Fail][main]","config in the config.pm missing",__FILE__,__LINE__);
        return undef;
    }
    #根据服务节点的地域，选择不同的中转机
    if(index($groupname,"BAIDU-JP") != -1){
        $zone_address = $single_conf->{'JP_ZONE_DOMAIN'};
    }
    elsif(index($groupname,"BAIDU-HK") != -1){
        $zone_address = $single_conf->{'HK_ZONE_DOMAIN'};
    }
    else{
        $zone_address = $single_conf->{'CN_ZONE_DOMAIN'};
        if(defined($is_p2p)){
            $zone_address = $single_conf->{'UPLOAD_CN_ZONE_DOMAIN'};
        }
    }

    #数据下载地址的拼接，需要根据是否是多模块来进行判断
    $sourcepath=$zone_address."/".$module_conf->{'listid'};
    if(defined($module_conf->{'is_mutil'}) && $module_conf->{'is_mutil'} == 1){
        $sourcepath =$zone_address."/".$module_conf->{'listid'}. "/".$module_conf->{'module'};
    }
    return $sourcepath;
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
    print "\n";
    print_log("main","if remote:$ifremote", __FILE__, __LINE__);
    print_log("main","list id:$listid", __FILE__, __LINE__);
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
    print_log("main","if remote:$ifremote", __FILE__, __LINE__);
    print_log("main","list id:$listid", __FILE__, __LINE__);
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

#清除临时生成的src目录
sub clean_all {
    if (0 != system("rm -rf single_code*/src")) {
        return -1;
    }
}

#对SSH部署方式进行检查
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
            print_error("[Fail][main]","host $sourceHost Failed!", __FILE__, __LINE__);
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
       # ssh方式必须指定源机器，一般不会进入，除非程序bug
        print_error("[Fail][main]","sourceHost is empty!",__FILE__,__LINE__);
        return -1;
    }
}

#创建.archer目录及其子目录
sub mk_md5local{
    #在临时目录下，创建.archer/md5_local和.archer/operations目录

    my $archer_local = $g_tmp_basedir."/src/.archer";
    my $md5_local = $archer_local."/md5_local";
    my $ret = mk_path($md5_local);

    if($ret ne 0){
         print_error("[Fail][main]"," mk_path($md5_local) Failed!\n",__FILE__,__LINE__);
         return -1;
    }

    #将src目录下的source.md5、bin.md5、data.md5、directory.emp文件，分别移动至.archer/md5_local/下
    if(not -e "./src/source.md5"){
        print_error("[Fail][main]","Diff deploy method,but have no source.md5",__FILE__,__LINE__);
        return -1;
    }
    else{
        $ret = mv_file_leaf('./src/source.md5',$md5_local."/source.md5");
        if($ret != 0){
            print_error("[Fail][main]","mv file ./src/source.md5 to $md5_local/source.md5 failed!",__FILE__,__LINE__);
            return -1;
        }
    }
    if(-e "./src/bin.md5"){
        $ret = mv_file_leaf('./src/bin.md5',$md5_local."/bin.md5");
        if($ret != 0){
            print_error("[Fail][main]","mv file ./src/bin.md5 to $md5_local/bin.md5 failed!",__FILE__,__LINE__);
            return -1;
        }
    }
    if(-e "./src/data.md5"){
        $ret = mv_file_leaf('./src/data.md5',$md5_local."/data.md5");
        if($ret != 0){
            print_error("[Fail][main]","mv file ./src/data.md5 to $md5_local/data.md5 failed!",__FILE__,__LINE__);
            return -1;
        }
    }
    if(-e "./src/directory.emp"){
        $ret = mv_file_leaf('./src/directory.emp',$md5_local."/directory.emp");
        if($ret != 0){
            print_error("[Fail][main]","mv file ./src/data.md5 to $md5_local/directory.emp failed!",__FILE__,__LINE__);
            return -1;
        }
    }
    return 0;
}

#生成operations操作序列文件
sub produce_operations{
    my ( $conf ) = @_;
    my $topdir;
    if(!defined($conf->{'deployPath'}) || trim($conf->{'deployPath'}) eq ''){
        print_error("[Fail][main]","deploy path undefined!",__FILE__,__LINE__);
        return -1;
    }
    else{
        $topdir = $conf->{'deployPath'};
    }

    #创建operations目录
    my $archer_local = $g_tmp_basedir."/src/.archer";
    my $operations = $archer_local."/operations";
    my $ret = mk_path( $operations );
    
    if( $ret != 0 ) {
        print_error( "[Fail][main]","mk_path $operations failed!",__FILE__,__LINE__ );
        return -1;
    }

    #初始化操作序列，并得到相关操作序列
    my ( %source_operations,%bin_operations,%data_operations ) = ();
#init_operations(\%source_operations);
#   init_operations(\%bin_operations);
#   init_operations(\%data_operations);

    #生成源包的操作序列
    my $source_local=$topdir."/.archer/md5_local/source.md5";
    my $source_remote = "./src/.archer/md5_local/source.md5";
    my $empty = '';
    if(-e "./src/.archer/md5_local/directory.emp"){
        $empty = "./src/.archer/md5_local/directory.emp";
    }
    $ret = get_source_operations($source_local,$source_remote,$empty,\%source_operations);
    if($ret != 0){
        print_error("[Fail][main]","get source operations failed!",__FILE__,__LINE__);
        return -1;
    }

    #生成bin组件的操作序列
    my $bin_local = $topdir."/.archer/md5_local/bin.md5";
    my $bin_remote = "./src/.archer/md5_local/bin.md5";
    if(-e "src/.archer/md5_local/bin.md5"){
        $ret = get_bin_and_data_operations($bin_local,$bin_remote,\%bin_operations);
        if($ret ne 0){
            print_error("[Fail][main]","get bin operations Failed!",__FILE__,__LINE__);
            return $ret;
        }
    }
    #生成data组件的操作序列
    my $data_local = $topdir."/.archer/md5_local/data.md5";
    my $data_remote = "./src/.archer/md5_local/data.md5";
    if(-e "src/.archer/md5_local/data.md5"){
        $ret = get_bin_and_data_operations($data_local,$data_remote,\%data_operations);
        if($ret ne 0){
            print_error("[Fail][main]","get data operations Failed!",__FILE__,__LINE__);
            return $ret;
        }
    }

    #将序列写入文件
    my $operationsFile = $operations."/operation";
    my $yaml = lib::Tiny -> new;
    if(%source_operations){
        $yaml->[0]->{'SOURCE'} = \%source_operations;
    }
    else{
        print_error("[Fail][main]","get_source_operations failed!",__FILE__,__LINE__);
        return -1;
    }
    $yaml->[0]->{'BIN'} = \%bin_operations;
    $yaml->[0]->{'DATA'} = \%data_operations;
    $yaml->write($operationsFile);

    return 0;
}

#解析多模块部署方式的参数信息
sub get_module_conf {
    my ($sourceList, $modulesInfo) = @_;
    $sourceList = trim($sourceList);

    my @modules = split(",", $sourceList);
    my $i = 0;
    foreach my $moduleInfo (@modules) {
        my ($module, $sourceType, $version, $sourceDetail,$action, $deployPath,$control_path) = split ("#",$moduleInfo);

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

sub get_single_source{
    my($internal_conf,$version_file_path,$i) = @_;
    if(!defined($internal_conf) || trim($internal_conf) eq ''){
        print_error("[Fail][main]","deploy info in the config.pm is missing",__FILE__,__LINE__);
        return -1;
    }

    my $single_path = $internal_conf->{'SCM_NOT_AOS_PATH'}."/prod-64/op/oped/noah/tools/noah-deploy-single/noah-deploy-single_";
    my $module_single_path = "";
    my $fh;
    my $line;
    my $tmpline;
    my $ret = 0;
    if(!defined($i)){
        $i = '';
    }
    if(-e $version_file_path){
        if(open($fh,$version_file_path)){
            while($line = <$fh>) {
               chomp($line);
               my $tmpline = trim($line);
               my ($component, $version) = split(":", $tmpline);
               $component = trim($component);
               $version = trim($version);
               if ($component eq "" || $version eq "") {
                   print_error("[FAIL][main]","version.des format error, wrong line is: $tmpline", __FILE__, __LINE__);
                   return -1;
               }
               $version =~ s/\./-/g;
               $version = $version."_PD_BL";
               print "[Source][component]:$component | [version]:$version\n";
               if($component eq "single"){
                     $module_single_path = $single_path."$version"."/output";
               }
               else{
                    $module_single_path = $single_path."$version"."/output/$component";
               }
               if (-d 'tmp') {
                   if($component eq 'single'){
                        $ret = rm_dir('tmp');
                        print_error("[Notice][main]","single rm tmp $ret",__FILE__,__LINE__);
                   }       
               }
               else{
                    if ($component ne "single") {
                        $ret = mk_path('tmp', 0755);
                        print "[not single] mkdir tmp $ret\n";
                    }
               }
               $ret = system("single_code/data/sshpass -p getprod scp -oStrictHostKeyChecking=no -r $module_single_path tmp");
               if($ret != 0){
                    print_error("[Fail][main]","download from srcm failed!",__FILE__,__LINE__);
                    return -1;
               }
               if($component eq 'single'){
                   if(not -e 'tmp/single_code'){
                       $ret = move_dir_to_dir("tmp","single_code$i");
                   }
                   else{
                       $ret = move_dir_to_dir("tmp/single_code","single_code$i");
                       $ret = rm_dir('tmp');
                   }
               }
               else{
                   $ret = move_dir_to_dir("tmp/$component","single_code$i/$component");
               }
               if($ret != 0){
                    print_error("[Fail][main]","move $component from tmp dir to single_code$i failed", __FILE__, __LINE__);
                    return -1;
               }
           }
        }
    }
    return 0;
}

#通过wget方式下载指定文件
sub download_file{
    my ($sourcepath,$destfile) = @_;
    if(!defined($sourcepath) || trim($sourcepath) eq ''){
        print_error("[Fail][main]","no source path!",__FILE__,__LINE__);
        return -1;
    }
    if(!defined($destfile) || trim($destfile) eq ''){
        print_error("[Fail][main]","no dest file",__FILE__,__LINE__);
    }
    my $ret = 0;
    my $retryInterval = 1;
    my $retryTimes = 3;
    my $cmd = '';

    for (my $i = 0; $i < $retryTimes; $i++) {
        $ret = 0;
        if(-e $destfile) {
            $cmd = "rm -rf $destfile";
            if(0 != system($cmd)){
                print_error("[Fail][main]","$cmd failed!",__FILE__,__LINE__);
                $ret = -1;
                next;
            }
        }
        $cmd = "wget -q -O '$destfile' $sourcepath";
        if(defined($is_p2p)){
            my $source_path = format_kingo_url($sourcepath);
            if(!defined($source_path)){
                print_error("[Fail][main]","set kingo sourcepath failed!",__FILE__,__LINE__);
                return -1;
            }
            $cmd = "gkocp -d $limit_rate $source_path $destfile >$kingo_logfile";
        }
        
        $ret = system($cmd);
        if($ret != 0){
            sleep($retryInterval);
            print_error("[notice][main]","$cmd failed", __FILE__, __LINE__);
        }
        else{
            last;
        }
    }
    if(not -e $destfile){
        print_error("[Fail][main]","download $destfile failed.", __FILE__, __LINE__);
        print STDOUT "[Fail]Error Code: NDEC-094002 \n";
        return -1;
    }
    return 0;
}

#根据source地址，下载、校验并解压
sub download_and_validate_source{
    my ($sourcepath,$destpath) = @_;
    if(!defined($sourcepath) || trim($sourcepath) eq ''){
        print_error("[Fail][main]","no source path!",__FILE__,__LINE__);
        return -1;
    }
    if(!defined($destpath) || trim($destpath) eq ''){
        $destpath = "./src";
    }
    
    #下载tar包对应的md5文件
    print "[procedure] get the tar's md5 file\n";
    my $ret = download_file($sourcepath."/source_file.md5","source_file.md5");
    if($ret != 0){
        print_error("[Fail][main]","download $sourcepath./source_file.md5 failed!",__FILE__,__LINE__);
        return -1;
    }

    #下载tar包
    print "[procedure] get the source\n";
    $ret = download_file($sourcepath."/source_file.tar.gz","source_file.tar.gz");
    if($ret != 0){
        print_error("[Fail][main]","download $sourcepath./source_file.tar.gz failed!",__FILE__,__LINE__);
        return -1;
    }

    #校验下载的tar包的正确性
    my $cmd = "md5sum --status -c source_file.md5";
    $ret = system($cmd);
    if($ret != 0){
        print "md5 check FailED!! Return code $ret\n";
        print STDOUT "[Fail]Error Code: NDEC-995005 \n";
        return -1;
    }
    
    #解包，并将其放在./src目录下
    if(not -d $destpath){
        if(0 != mk_path($destpath)){
            print_error("[Fail][main]","Cannot mkdir $destpath:$!", __FILE__, __LINE__);
            return -1;
        }
    }
    
    $cmd = "tar zxf source_file.tar.gz -C $destpath";
    $ret = system($cmd);
    if ($ret != 0) {
        print_error("[Fail][main]","$cmd failed", __FILE__, __LINE__);
        return -1;
    }

    #archer-web支持多层解压
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
            else{
                @param = ("rm", "-f", "$file");
                $ret = system(@param);
                if ($ret != 0){
                    print_error("[Warning][main]","rm -f $file failed", __FILE__, __LINE__);
                }
            }
        }
    }
    return $ret;
}

#单个模块部署逻辑
sub deploy_one_module{
    my ($internal_conf,$module_conf) = @_;
    my $sourcepath = '';
    my $moduleName = '';
    my $ret = 0;

    my $i = 0;
    my $is_yaml = 1;
    my $is_mutil = 0;
    my $topdir = '';

    if(defined($module_conf->{'order'})){
        $i = $module_conf->{'order'};
    }
    if(defined($module_conf->{'is_yaml'})){
        $is_yaml = $module_conf->{'is_yaml'};
    }
    if(defined($internal_conf->{'is_mutil'})){
        $is_mutil = $module_conf->{'is_mutil'};
    }
    if(defined($module_conf->{'deployPath'}) && trim($module_conf->{'deployPath'}) ne ''){
        $topdir = $module_conf->{'deployPath'};
    }
    if(defined($module_conf->{'module'}) && trim($module_conf->{'module'}) ne ''){
        $moduleName = $module_conf->{'module'};
    }

    print_start("main");

    if(0 != system("cp single_code single_code$i -rf")){
        print_error("[Fail][main]","Create single_code$i Failed!",__FILE__,__LINE__);
        return -1;
    }

    #获取模块中转机地址
    $sourcepath = set_sourcepath($internal_conf,$module_conf);
    if(!defined($sourcepath)){
        print_error("[Fail][main]","Set source path Failed!",__FILE__,__LINE__);
        return -1;
    }

    #下载源包并校验
    $ret = download_and_validate_source($sourcepath);
    if($ret != 0){
        print_error("[Fail][main]","data parepare failed!",__FILE__,__LINE__);
        return -1;
    }

#生成源包、bin、data等文件

    $ret = mk_md5local();
    if($ret ne 0){
        print_error("[Fail][main]","initialize Failed!",__FILE__,__LINE__);
        return $ret;
    }

#生成操作序列文件
    $ret = produce_operations($module_conf);
    if($ret != 0){
        print_error("[Fail][main]","produce operations  file failed!",__FILE__,__LINE__);
        return -1;
    }

    #判断是否是依赖解耦过程
    $ret = get_single_source($internal_conf,'src/noahdes/version.des',$i);
    if($ret != 0){
        return $ret;
    }
    
    if(-e "./src"){
        $ret = move_dir_to_dir("./src", "./single_code$i/src");
        if($ret != 0){
            return $ret;
        }
    }
    
    #生成部署信息文件
    my $final_cmd;
    my $conf_text = "";
    if($ret != 0){
        return $ret;
    }
    else {
        if (-e "single_code$i/src/noahdes/operations_map.pm") {
            if (0 != system("mv single_code$i/src/noahdes/operations_map.pm single_code$i/operations_map.pm")) {
                print_error("[Fail][main]","mv single_code$i/src/noahdes/operations_map.pm single_code$i/operations_map.pm Failed:$!", __FILE__, __LINE__);
                return 1;
            }
        }
        if (-e "single_code$i/src/noah-control.pl") {
            if (0 != system("mv single_code$i/src/noah-control.pl single_code$i/noah-control.pl")) {
                print_error("[Fail][main]","mv single_code/src/noah-control.pl single_code/noah-control.pl Failed:$!", __FILE__, __LINE__);
                return 1;
            }
        }
        if(defined($is_p2p)){
            $conf_text .="P2P:TRUE\n";
        }
        if(defined($is_yaml)  && $is_yaml == 1){
            $conf_text .="YAML:TRUE\n";
        }
        if(defined($is_diff)){
            $conf_text .="DIFF:TRUE\n";
        }
        if(defined($sourcepath)){
             $conf_text .= "SOURCE_PATH:$sourcepath\n";
        }
        if (defined($test)) {
            $conf_text .= "ONLY_TEST:TRUE\n";
        }
        if (defined($fullamount)) {
            $conf_text .= "DEPLOY_METHOD:ALL\n";
        }else {
            $conf_text .= "DEPLOY_METHOD:INCREASE\n";
        }
        if (defined( $group ) && $group ne "") {
            $conf_text .= "GROUP_NAME:$group\n";
        }else {
            print_error("[Fail][main]","cannot get the group name", __FILE__, __LINE__);
            return 1;
        }
        if (defined($module_conf->{'controlPath'}) && $module_conf->{'controlPath'} ne "") {
            $conf_text .= "CONTROL_PATH:$module_conf->{'controlPath'}\n";
        }
        if (defined($if_rollback)) {
            $conf_text .= "OPERATION:ROLLBACK\n";
        }else {
            $conf_text .= "OPERATION:DEPLOY\n";
        }
        if (defined($module_conf->{'action'}) && $module_conf->{'action'} ne "") {
            $conf_text .= "ACTION:$module_conf->{'action'}\n";
        }else {
            $conf_text .= "ACTION:install\n";
        }
        if (defined($limit_rate)) {
            $conf_text .= "LIMIT_RATE:$limit_rate\n";
        }
        if (defined($machine_interval)) {
            $conf_text .= "MACHINE_INTERVAL:$machine_interval\n";
        }
           
        my $backup_dir = ""; 
        $ret = get_backup_dir($listid, \$backup_dir);
        if ($ret == 0 && $backup_dir ne "") {
            $conf_text .= "BACKUP_DIR:$backup_dir\n";
        }else {
            print_error("[Fail][main]","get_backup_dir Failed", __FILE__, __LINE__);
            return 1;
        }   
            
        my $backup_log_dir = "";
        $ret = get_backup_log_dir($listid, \$backup_log_dir);
        if ($ret == 0 && $backup_log_dir ne "") {
            $conf_text .= "BACKUP_LOG_DIR:$backup_log_dir\n";
        }else {
            print_error("[Fail][main]","get_backup_log_dir Failed", __FILE__, __LINE__);
            return 1;
        }
        if(defined($deploylist)){
            $ret = check_deploy_diff($internal_conf,$listid,\$topdir);
            if($ret != 0){
                 print_error("[Fail][main]","check_deploy_diff failed!\n",__FILE__,__LINE__);
                 exit 1;
            }
            $module_conf->{'deployPath'} = $topdir;
        }
        if (defined($module_conf->{'deployPath'}) && $module_conf->{'deployPath'} ne "") {
            $conf_text .= "TOP_DIR:$module_conf->{'deployPath'}\n";
        }else {
            print_error("[Fail][main]","no destination of top dir of deploy files", __FILE__, __LINE__);
            return 2;
        }
        if (defined($module_conf->{'module'}) && $module_conf->{'module'} ne "") {
            $conf_text .= "SCM_MODULE:$module_conf->{'module'}\n";
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
            print_error("[Fail][main]","部署失败，请检查.Deploy Failed, please check!!!",__FILE__,__LINE__);
            if ($email_alertlist ne "" || $tele_alertlist ne "") {
                fail_alert($email_alertlist, $tele_alertlist, $listid);
            }
            return 1;
        }
        print_log("main","End main",__FILE__,__LINE__);
        return 0;
    }
    return 0;
}

#entrance, do authorization check first
$| = 1;
my $ret = 0;

#整理部署环境
clean_all();

#获取single插件的系统配置
my $single_internal_conf = get_basic_config();

#ssh方式部署，权限检验
if (defined($ssh)) {
    $ret = checkSsh();
    if ($ret != 0) {
        exit $ret;
    }
}

print_log("main","Enter main",__FILE__,__LINE__);

if(defined($sourceList)){
    #多模块部署逻辑
    my %modulesInfo = ();
    my $i = 0;

    #获取多模块部署的配置文件
    $ret = get_module_conf($sourceList, \%modulesInfo);
    if ($ret != 0) {
        exit -1;
    }
    foreach my $module_key(sort {$a<=>$b} keys %modulesInfo){
        $modulesInfo{$module_key}->{'is_mutil'} = 1;
        $modulesInfo{$module_key}->{'is_yaml'} = 1;
        $modulesInfo{$module_key}->{'order'} = $i;
        $modulesInfo{$module_key}->{'listid'} = $listid;
        $modulesInfo{$module_key}->{'group'} = $group;

        #部署逻辑
        $ret = deploy_one_module($single_internal_conf,$modulesInfo{$module_key});
        if($ret != 0){
            exit 1;
        }
        $i++;
    }
}
else{
    my %modulesInfo = ();
    $modulesInfo{'listid'} = $listid;
    $modulesInfo{'group'} = $group;
    $modulesInfo{'deployPath'} = $topdir;
    $modulesInfo{'is_yaml'} = $is_yaml;
    $modulesInfo{'is_mutil'} = 0;
    $modulesInfo{'action'} = $action;
    $modulesInfo{'controlPath'} = $control_path;
    $modulesInfo{'module'} = $module_name;
    $modulesInfo{'order'} = 0;

    $ret = deploy_one_module($single_internal_conf,\%modulesInfo);
}
exit $ret;
