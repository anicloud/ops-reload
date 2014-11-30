#!/usr/bin/perl -w
#encode utf-8
package lib::download_construct;
my $g_tmp_basedir;
my $lib;
BEGIN {
    use Cwd 'realpath';
    $g_tmp_basedir = __FILE__;
    $g_tmp_basedir = realpath($g_tmp_basedir);

    $g_tmp_basedir =~ s/[^\/]+$//;
    $lib = $g_tmp_basedir."../";
    unshift(@INC,$lib);
    unshift( @INC, $g_tmp_basedir );
}

use strict;
use Getopt::Long;
use FileHandle;
use File::Basename;
use POSIX;
use lib::Tiny;
use lib::check_file;
use Data::Dumper;
use file_operations;

my $config_file = "";
my $deploy_path = "/home/users/xuyang04/work/bs";
my $limit_rate= '3m';
my $account = $ENV{'USER'};
my $md5dir = "/home/$account/backup/ci_backup";
my $mode;
my $retry_count = 3;
my $is_fullamount;

Getopt::Long::GetOptions
(
    'c:s' => \$config_file,
    'p:s' => \$deploy_path,
    'l:s' => \$limit_rate,
    'm:s' => \$mode,
    'f:s' => \$is_fullamount,
);

#ftp方式下载指定地址数据
sub download_ftp{
    my ($remote,$local,$limit_rate,$retry_count) = @_;
    
    if(defined($remote)){
        $remote =~ s/\/+/\//g;
        $remote =~ m/ftp:\/([^:\/]*):?(\/.*)/;

        $remote = "ftp://".$1.$2;
        print "$remote\n";
    }

    my $cur_file_name = basename($remote);
    my $cut_num = get_cut_dir_num($remote);

    my $ret =check_ftp_file($remote);
    my  $cmd = "wget -q --limit-rate=$limit_rate --level=40 -r -nH --cut-dir=$cut_num $remote";
    if($ret ne 0 and $retry_count ne 0 ){
        sleep(1);
        print "download_ftp failed,retry....\n";
        download_ftp($remote,$local,$limit_rate,$retry_count-1);
    }
    if($ret eq 0 ){
        $cmd = "wget -q --limit-rate=$limit_rate -nH --cut-dir=$cut_num $remote";
    }
    print "download_ftp CMD:$cmd\n";
    $ret = system($cmd);
    if($ret eq 0 ){
        #判断local目录是否存在，如果存在，则删除后进行mv操作
        if (-e $local){
            $cmd = "rm -rf $local";
            system($cmd);
        }
        if(-e $cur_file_name){
            $cmd = "mv $cur_file_name $local";
            system($cmd);
        }
        else{
            print "[Fail]Please check your ftp address!!\n";
        }
    }
    return $ret;
}

#scp方式下载指定地址数据
sub download_scp{
     my ($remote,$local,$passwd,$retry_count) = @_;

     my $cmd="$g_tmp_basedir/sshpass -p $passwd /usr/bin/scp -oStrictHostKeyChecking=no -r -q $remote $local";
     print "download_scp CMD:$cmd\n";

     my $ret = system($cmd);
     if($ret ne 0 and $retry_count ne 0 ){
        sleep(1);
        print "download_scp failed,retry...\n";
        $ret = download_scp($remote,$local,$passwd,$retry_count-1);
     }
     return $ret;
}

sub transfer_rate{
    $_ = shift;
    my $last_rate;
    m|(\d+)([A-Za-z])$|;

    if($2 eq 'm' || $2 eq 'M'){
        $last_rate = $1 * 1024;
    }
    elsif($2 eq 'G' || $2 eq 'g'){
        $last_rate = $1 * 1024 * 1024;
    }
    elsif($2 eq 'k' || $2 eq 'K'){
        $last_rate = $1;
    }
    else{
        $last_rate = $_/1024;
    }
    return $last_rate."k";
}
#bscp方式下载源数据
sub download_bscp{
    my ($remote,$local,$limit_rate,$retry_count) = @_;
    my $last_rate = transfer_rate($limit_rate);
    my $cmd ="bscp -l $last_rate $remote $local";
    
    my $proto = get_bscp_proto($remote);
    if($proto eq 'hdfs'){
        my $client_path = "/home/$account/opbin/hadoop-client-cc/hadoop/bin";
        $cmd="bscp -l $last_rate -P $client_path $remote $local";
    }
    print "download_bscp CMD:$cmd\n";
    my $ret = system($cmd);

    if($ret ne 0 and $retry_count ne 0){
        sleep(1);
        print "download_bscp failed,retry...\n";
        download_bscp($remote,$local,$limit_rate,$retry_count-1);
    }
    return $ret;
}

#bscp方式上传数据
sub upload_bscp{
     my ($local,$limit_rate) = @_;
     my $cmd ="bscp -l $limit_rate  $local";

     print "upload_bscp CMD:$cmd\n";
     my $ret = system($cmd);
     return $ret;
}
#解析yaml格式源数据文件
sub parse_destfile{
    my ($file_name,$conftext) = @_;
    my $yaml = lib::Tiny -> new;
    $yaml = lib::Tiny -> read($file_name);
    if(!defined($yaml)){
        print "[Fail]YAML parse failed!! Error Line is:$YAML::Tiny::errstr!\n";
        print STDOUT "[Fail]Error Code:NDEC-091001\n";
        return -1;
    }
    my $sub_item;
    my $hash = $yaml->[0];
    foreach my $key(keys %{$hash}){
        my $item = $hash->{$key};
        if(index($key,'d:') != -1){

            print "[Fail]Data dist not support now!\n";
            return -1;

            my ($cc,$keys) = split(':',$key);
            $$conftext{$keys}{'type'} = 'data';
            $$conftext{$keys}{'key'} = $item->{'src'};
            $$conftext{$keys}{'update_style'} = 1;
        }
        else{
            $sub_item =  $item->{'src'};
            $sub_item=~ s/\/\/\/+/\/\//;
            $sub_item=~ s/\/+$//;

            if(index($sub_item,'ftp://') != -1 ){
                $$conftext{$key}{'type'}='ftp';
                $$conftext{$key}{'key'} = $sub_item;
            }
            elsif(index($sub_item,'@') !=-1){
                $$conftext{$key}{'type'}='scp';
                my @tmp_arr = split(':',$sub_item);
                if(scalar(@tmp_arr eq 3)){
                      $$conftext{$key}{'key'} = $tmp_arr[1].":".$tmp_arr[2];
                      $$conftext{$key}{'passwd'} = $tmp_arr[0];
                }
                else{
                      $$conftext{$key}{'passwd'}='getprod';
                      $$conftext{$key}{'key'}= $sub_item;
                }
            }
            elsif(index($sub_item,'data://') != -1 ){
                 $$conftext{$key}{'type'}='data';
                 $$conftext{$key}{'key'} = $sub_item;
                 $$conftext{$key}{'update_style'} = 1;
            }
            else{
                  $$conftext{$key}{'type'}='ftp';
                  $$conftext{$key}{'key'} =$sub_item;
             }
        }
    }
    return 0;
}

#获取源数据下载方式
sub get_download_mode{
    my ($sourcePath) = @_;
    my $type = 'ftp';

    if(index($sourcePath,'ftp://') != -1){
        $type = 'ftp';
    }
    elsif(index($sourcePath,'@') !=-1){
         $type = 'scp';
    }
    elsif(index($sourcePath,'data://') != -1){
        $type='data';
    }
    return $type;
}

#ftp方式源数据过程
sub download_source_by_ftp{
    my ($conf,$is_multi) = @_;
    my $remote_md5 = $conf->{'REMOTE_MD5'};
    my $tmp_remote_md5_file = $conf->{'TMP_REMOTE_MD5_FILE'};
    my $value = $conf->{'VALUE'};
    my $local_file = $conf->{'LOCAL_FILE'};
    my $backup_md5_file = $conf->{'BACKUP_MD5_FILE'};
    my $tmp_local_file = $conf->{'TMP_LOCAL_FILE'};
    my $local_dir =$conf->{'LOCAL_DIR'};
    my $full_data_path = $conf->{FULL_DATA_PATH};

    my $ret;
    my $cmd;

    chdir "$g_tmp_basedir";
    $ret = download_ftp($remote_md5,$tmp_remote_md5_file,$limit_rate,3);
    if($ret != 0){ 
        #print "Get Md5 failed!DOWNLOAD!\n";
        if( -e $tmp_remote_md5_file){
            $cmd = "rm -r $tmp_remote_md5_file";
            system($cmd);
        }

        #print "Get Md5 failed! md5 is essential to check the file.\n";
        #return -1;

        #ftp 下载文
        $ret = download_ftp($value,$local_file,$limit_rate,3);
        if($ret != 0){ 
            print "Get $value failed!!!\n";
            if(-e $local_file){
                system("rm -rf $local_file");
            }   
            return -1; 
        }   
        else{
            print "DOWNLOAD $value succeed!!!\n";
        }   
    }   
    else{
        #下载md5文件成功
        print "Get Md5 succeed!\n";
        #对比两边的md5
        $cmd = "diff $tmp_remote_md5_file $backup_md5_file &>/dev/null";
        $ret = system($cmd);

        #md5 不相同
        if($ret != 0){
            my $msg;
            if($ret == 1){
                $msg = 'md5 NOT SAME';
            }
            else{
                $msg = 'md5 not FOUND in cache';
            }
            print "$msg\n";

            #ftp下载
            $ret = download_ftp($value,$tmp_local_file,$limit_rate,3);
            if($ret != 0 ){
                print "DOWNLOAD $value failed!\n";
                return -1;
            }
            else{
                print "DOWNLOAD $value succeed!\n";

                #md5 校验
                $cmd = "cd $local_dir && md5sum --status -c $tmp_remote_md5_file";
                $ret = system($cmd);
                #md5校验失败
                if($ret != 0){
                    print "md5 check FailED!! Return code $ret\n";
                    print STDOUT "[Fail]Error Code: NDEC-995005 \n";
                    return -1;
                }
                else{
                    #md5校验成功
                    print "====>md5 check succeed!\n";

                    if(-e $local_file && !($tmp_local_file eq $local_file)){
                        $cmd = "rm -rf $local_file";
                        if(system($cmd)){
                            print "bin_construct.pl::main:rm -rf $local_file failed!\n";
                        }
                    }
                    if(!($tmp_local_file eq $local_file)){
                        if($is_multi eq 1){
                            print "222222";
                            my $basename = basename($tmp_local_file);
                            my $local = $local_file."/".$basename;

                            # $cmd="mv $tmp_local_file $local";
                        }
                        else{
                            $cmd = "mv $tmp_local_file $local_file";
                        }
                        print "1111111111$cmd\n";
                        if(system($cmd)){
                                print "bin_construct.pl::main:  mv $tmp_local_file $local_file failed!\n";
                        }
                    }

                    #更新md5
                    $cmd = "mv $tmp_remote_md5_file $backup_md5_file";
                    $cmd = "mv $tmp_remote_md5_file $backup_md5_file";
                    system($cmd);
                }
            }
        }
        # md5相同
        else{
            #清楚下载的md5文件
            my @dir_file = <$full_data_path/*>;
            if (-e $tmp_remote_md5_file){
                $cmd = "rm -rf $tmp_remote_md5_file";
                system($cmd);
            }
            print "MD5 same!!!\n";
            if(@dir_file || -e $full_data_path){
                print "no need to re-download\n";
            }
            else{
                print "Dest directory is empty,need to download \n";
                $ret = download_ftp($value,$local_file,$limit_rate,$retry_count);
                if($ret!= 0){
                    if(-e $local_file){
                        system("rm -rf $local_file");
                    }
                    print "Get $value failed!\n";
                    return -1;
                }
                else{
                    print "DOWNLOAD succeed\n";
                }
            }
        }
    }
    return 0;
}
#scp方式下载源数据
sub download_source_by_scp{
    my ($conf,$passwd,$is_multi) = @_; 
    my $remote_md5 = $conf->{'REMOTE_MD5'};
    my $tmp_remote_md5_file = $conf->{'TMP_REMOTE_MD5_FILE'};
    my $value = $conf->{'VALUE'};
    my $local_file = $conf->{'LOCAL_FILE'};
    my $backup_md5_file = $conf->{'BACKUP_MD5_FILE'};
    my $tmp_local_file = $conf->{'TMP_LOCAL_FILE'};
    my $local_dir =$conf->{'LOCAL_DIR'};
    my $full_data_path = $conf->{FULL_DATA_PATH};

    my $ret;
    my $cmd;

    chdir "$g_tmp_basedir";
    if(download_scp($remote_md5,$tmp_remote_md5_file,$passwd,$retry_count) != 0){
        ##下载md5失败，清楚md5
        #print "Get Md5 failed! md5 is essential to check the file.\n";
        #return -1;
 
        if (-e $tmp_remote_md5_file){
            $cmd = "rm -rf $tmp_remote_md5_file";
            system($cmd);
        }
        #ftp 下载文件-
        if(download_scp($value,$local_file,$passwd,$retry_count) != 0){
            print "Get $value failed!\n";
            if ( -e $local_file){
                system("rm -rf $local_file");
            }
            return -1;
        }
        else{
            print "DOWNLOAD $value succeed!\n";
        }
    }
    else{
        #MD5 download success
        print "Get Md5 succeed!\n";
        #对比两边的md5-
        $cmd = "diff $tmp_remote_md5_file $backup_md5_file &>/dev/null";
        $ret = system($cmd);
                
        #md5 not same
        if($ret != 0){
            print "MD5 not the same,DOWNLOAD!\n";
            if(download_scp($value,$tmp_local_file,$passwd,$retry_count) != 0){
                print " DOWNLOAD $value failed!\n";
                return -1;
            }
            else{
                print "DOWNLOAD $value succeed!\n";
                #md5 检验
                   
                $cmd = "cd $local_dir && md5sum  --status -c $tmp_remote_md5_file";
                $ret = system($cmd);
                if($ret != 0){
                    print "MD5 check FailED\n";
                    print STDOUT "[Fail]Error Code: NDEC-905005\n";
                    return -1;
                }
                #md5 校验成功
                if( -e $local_file && !($tmp_local_file eq $local_file)){
                    $cmd = "rm -rf $local_file";
                    system($cmd);
                }
                if(!($tmp_local_file eq $local_file)){
                    if($is_multi == 1){
                        my $basename = basename($tmp_local_file);
                        my $local= $local_file."/".$basename;
                        $cmd = "mv $tmp_local_file $local";
                    }
                    else{
                        $cmd = "mv $tmp_local_file $local_file";
                    }
                    $ret = system($cmd);
                    if($ret ne 0){
                        print "[Fail] mv $tmp_local_file $local_file\n";
                        return -1;
                    }
                }
                print "mv $tmp_local_file $local_file\n";
                $cmd  = "mv $tmp_remote_md5_file $backup_md5_file";
                print "$cmd\n";
                system($cmd);
            }
        }
        else{
            my @dir_file = <$full_data_path/*>;
            if (-e $tmp_remote_md5_file){
                $cmd = "rm -rf $tmp_remote_md5_file";
                system($cmd);
            }
            if(@dir_file || -e $full_data_path){
                print "MD5 same ,and dest exist!NO NEED TO DOWNLOAD File\n";
            }
            else{
                print "MD5 same,but the dest is not exist!NEED TO DOWNLOAD!\n";
                $ret = download_scp($value,$local_file,$passwd,$retry_count);
                if($ret!= 0){
                    print "Get $value failed!\n";
                    if(-e $local_file){
                        system("rm -rf $local_file");
                    }
                    return -1;
                }
                else{
                    print "DOWNLOAD succeed\n";
                }
            }
        }
    }
    return 0;
}

sub get_bscp_proto{
    my ($datakey) = @_;
    my $proto = '';
    if(index($datakey,'data') == -1){
        return $proto;
    }
    my $log = $g_tmp_basedir."/bscp.log";
    my $cmd = "bscp --list $datakey >$log";

    my $listinfo = system($cmd);

    $cmd = `cat $log`;
    my @list = split(/\n/,$cmd);
    my @line = split(/,/,$list[2]);

    foreach my $keys (@line){
        my ($type,$proto)  = split(/:/,$keys);
        $type =~ s/\"//g;
        $proto =~ s/\"//g;
        if($type eq 'proto' && $proto eq 'hdfs'){
            return 'hdfs';
        }
    }
    return '';
}
#bscp方式下载指定地址数据
sub download_source_by_bscp{
    my ($conf,$style,$is_multi) = @_;
    my $remote_md5 = $conf->{'REMOTE_MD5'};
    my $tmp_remote_md5_file = $conf->{'TMP_REMOTE_MD5_FILE'};
    my $value = $conf->{'VALUE'};
    my $local_file = $conf->{'LOCAL_FILE'};
    my $backup_md5_file = $conf->{'BACKUP_MD5_FILE'};
    my $tmp_local_file = $conf->{'TMP_LOCAL_FILE'};
    my $local_dir =$conf->{'LOCAL_DIR'};
    my $full_data_path = $conf->{'FULL_DATA_PATH'};

    my $ret;
    my $proto = get_bscp_proto($value);

    if($style eq 1){
        if($is_multi == 1){
            $ret = download_bscp($value,$local_dir,$limit_rate,$retry_count);
            if($ret ne 0){
                print "[Fail]download_construct::main::download_bscp()failed\n ";
                return -1;
            }   
#bscp 的hudoop方式，需要删除指定目录下的.list文件
            my $list_file = $local_dir."/.list";
            if(-e $list_file  && -f $list_file && $proto eq 'hdfs'){
                $ret = system("rm -rf $list_file");
                if($ret != 0){
                    print STDOUT "[Fail] rm -rf $local_file failed!\n";
                    return -1;
                }
            }
        }
        else{
#bscp 的hudoop方式，目标必须是目录，不能是文件
           if($proto eq 'hdfs'){
               $ret = mk_path($local_file);
               if($ret){
                    print STDOUT "[Fail] mk_path ($local_file) failed!\n";
                    return -1;
               }
           }
           $ret = download_bscp($value,$local_file,$limit_rate,$retry_count);
        }
    }
    elsif($style == 2){
        $ret = upload_bscp($value,$limit_rate,$retry_count);
    }
    if($ret ne 0){
        print "[Fail]download_construct::main::download_bscp()failed\n ";
        return -1;
    }
    return 0;
}

#单目录多源数据下载方式
sub set_multi_deploy_relevant_path{
    my ($deploy_path,$key,$value,$passwd,$type,$conf) = @_;
    
    my $ret;
    my $cmd;

    $key =~ s/\/+$//;
    $value =~ s/\;+$//;
#获取文件的下载方式
    my $tmp_deploy_path = $deploy_path;
    my $data_remote_basename = basename($value);
    my $data_remote_dirname = dirname($value);
    my $remote_md5 = "$value.md5";

# 数据文件临时目录，固定为 ../src/
    my $local_dir = $g_tmp_basedir."/../src/".$key;
    my $local_file = $local_dir."/".$data_remote_basename;

#最后部署路径
    my $full_data_path = $tmp_deploy_path."/".$key;
    my $tmp_local_file = $local_dir."/".$data_remote_basename;
    my $tmp_remote_md5_file = "$tmp_local_file.md5";

#备份MD5文件路径
    my $backup_md5_dir = $md5dir."/".$tmp_deploy_path."/".$key;
    my $backup_md5_file = $backup_md5_dir."/".$data_remote_basename.".md5";
 
#绝对路径部署
    if(($key =~ m/^(bin|data)/) eq "" && $key =~ m/^\//  ne ""){
        if($type eq 'ftp'){
            $ret =check_ftp_file($value);
        }
        elsif($type eq 'scp'){
            $ret = check_scp_file($passwd, $value);
        }
        if($ret eq 1 && -e "/./".$key && !(-d "/./".$key)) {
            print "[Fail]Absolute path deploy ,and $key is a file";
            return -1;
        }
        $tmp_deploy_path = $key;
        $tmp_deploy_path =~ s/\/+$//;

        $local_dir = $g_tmp_basedir."/../src_abs/".$key;
        $local_file = $local_dir."/".$data_remote_basename;
        $tmp_local_file = $local_dir."/".$data_remote_basename;
        $tmp_remote_md5_file = $tmp_local_file."md5";

        $backup_md5_dir = $md5dir."/".$tmp_deploy_path;
        $full_data_path = $backup_md5_dir;
        if(! -e  $tmp_deploy_path){
            $cmd = "mkdir -p $tmp_deploy_path";
            $ret = system($cmd);
            if($ret){
                print "[Fail]download_construct.pl::main::mkdir -p $tmp_deploy_path failed!\n";
                return -1;
            }
        }
        $backup_md5_file = $backup_md5_dir."/".$data_remote_basename.".md5";
    }
        
#新建各种文件目录信息
    if(! -e $local_dir){
        $cmd = "mkdir -p $local_dir";
        $ret = system($cmd);
        if($ret){
            print "[Fail]download_construct.pl::main::mkdir -p $tmp_deploy_path failed!\n";
            return -1;
        }
    }
    if(! -e $backup_md5_dir){
        $cmd = "mkdir -p $backup_md5_dir";
        $ret = system($cmd);
        if($ret){
            print "[Fail]download_construct.pl::main::mkdir -p $tmp_deploy_path failed!\n";
            return -1;
        }
    }

    $conf->{'REMOTE_MD5'} = $remote_md5;
    $conf->{'LOCAL_FILE'} = $local_file;
    $conf->{'FULL_DATA_PATH'} = $full_data_path;
    $conf->{'TMP_LOCAL_FILE'} = $tmp_local_file;
    $conf->{'TMP_REMOTE_MD5_FILE'} = $tmp_remote_md5_file;
    $conf->{'VALUE'} = $value;
    $conf->{'LOCAL_DIR'} = $local_dir;
    $conf->{'BACKUP_MD5_FILE'} = $backup_md5_file;
}

sub set_single_deploy_relevant_path{
    my ($deploy_path,$key,$value,$passwd,$type,$conf) = @_;
    my $ret;
    my $cmd;

    $key =~ s/\/+$//;
    $value =~ s/\;+$//;
    
    my $tmp_deploy_path = $deploy_path;
    my $data_basename = basename($key);
    my $data_dirname = dirname($key);
    my $data_remote_basename = basename($value);
    my $data_remote_dirname = dirname($value);
    my $remote_md5 = "$value.md5";
    
    # 数据文件临时目录，固定为 ../src/
    my $local_dir = $g_tmp_basedir."/../src/".$data_dirname;
                        
    #临时数据文
    # 最终数据文件
    my $local_file = $local_dir."/".$data_basename;
                        
    #最后部署路径
    my $full_data_path = $tmp_deploy_path."/".$key;
    my $tmp_local_file = $local_dir."/".$data_remote_basename;

    my $tmp_remote_md5_file = "$tmp_local_file.md5";
                                        
    #备份MD5文件路径
    my $backup_md5_dir = $md5dir."/".$tmp_deploy_path."/".$data_dirname;
    my $backup_md5_file = $backup_md5_dir."/".$data_basename.".md5";

    #判断给出的路径是否为绝对路径
    if(($key =~ m/^(bin|data)/) eq "" && $key =~ m/^\//  ne ""){
#给出的路径是绝对路径，并且本地部署路径是文件，但是却是目录部署
        if($type eq 'ftp'){
            $ret =check_ftp_file($value);
        }
        elsif($type eq 'scp'){
            $ret = check_scp_file($passwd, $value);
        }

        if($ret eq 1 && -e "/./".$key && !(-d "/./".$key)) {
            print "[Fail]Absolute path deploy ,and $key is a file,but want to deploy a directory\n";
            return -1;
        }
        $tmp_deploy_path = $data_dirname;
        $tmp_deploy_path =~ s/\/+$//;
        $local_dir = $g_tmp_basedir."/../src_abs/".$data_dirname;
        $local_file = $local_dir."/".$data_basename;
        $tmp_local_file = $local_dir."/".$data_remote_basename;
        $tmp_remote_md5_file = "$tmp_local_file.md5";
        $backup_md5_dir = $md5dir."/".$tmp_deploy_path;
        $full_data_path = $backup_md5_dir;
        
        if(! -e $tmp_deploy_path){
            $cmd = "mkdir -p $tmp_deploy_path";
            $ret = system($cmd);
            if($ret){
                print "[Fail]download_construct.pl::main::mkdir -p $tmp_deploy_path\n";
                return -1;
            }
        }
        $backup_md5_file = $backup_md5_dir."/".$data_basename.".md5";
    }
    if(! -e $local_dir){
        $cmd = "mkdir -p $local_dir";
        if(system($cmd)){
            print "download_consruct.pl::main::mkdir -p $local_dir failed!\n";
            return -1;
        }
    }
    if(! -e $backup_md5_dir){
        $cmd = "mkdir -p $backup_md5_dir";
        if(system($cmd)){
            print "bin_construct.pl::main::mkdir -p $backup_md5_dir failed!\n";
            return -1;
        }
    }

    $conf->{'REMOTE_MD5'} = $remote_md5;
    $conf->{'LOCAL_FILE'} = $local_file;
    $conf->{'FULL_DATA_PATH'} = $full_data_path;
    $conf->{'TMP_LOCAL_FILE'} = $tmp_local_file;
    $conf->{'TMP_REMOTE_MD5_FILE'} = $tmp_remote_md5_file;
    $conf->{'VALUE'} = $value;
    $conf->{'LOCAL_DIR'} = $local_dir;
    $conf->{'BACKUP_MD5_FILE'} =$backup_md5_file;
}

sub download_sources{
    my ($deploy_path,$key,$value,$passwd,$update_type,$is_multi) =@_;
    my %conf = ();
    my $cmd;
    my $ret;
    my $type = get_download_mode($value);
    if(defined($is_multi) && $is_multi == 1){
        if($type eq 'scp'){
            my @tmp_arr = split(':',$value);
            if(scalar(@tmp_arr eq 3)){
                $value = $tmp_arr[1].":".$tmp_arr[2];
                $passwd = $tmp_arr[0];
            }
            else{
                $passwd = 'getprod';
            }
        }
        $ret = set_multi_deploy_relevant_path($deploy_path,$key,$value,$passwd,$type,\%conf);
    }
    else{
        $ret = set_single_deploy_relevant_path($deploy_path,$key,$value,$passwd,$type,\%conf);
    }
    if($type eq 'data'){
        $ret = download_source_by_bscp(\%conf,1,$is_multi);
    }
    elsif($type eq 'ftp'){
        $ret = download_source_by_ftp(\%conf,$is_multi);
    }
    elsif($type eq 'scp'){
        #if($is_multi == 1){
        #   my $value = $conf{'VALUE'};
        #   my @tmp_arr = split(':',$value);
        #   if(scalar(@tmp_arr eq 3)){
        #   $value = $tmp_arr[1].":".$tmp_arr[2];
        #        $passwd = $tmp_arr[0];
        #    }   
        #   else{
        #       $passwd = 'getprod';
        #   }
        #   $conf{'VALUE'} = $value;  
        #}
        $ret = download_source_by_scp(\%conf,$passwd,$is_multi);
    }
    if($ret != 0){
        print "download_sources failed!\n";
        return -1;
    }
    if($mode eq 'data'){
        print "mode:$mode\n";
    }
    elsif($mode eq 'bin'){
        print "mode:$mode\n";
        if(-e $conf{'LOCAL_FILE'}){
            $cmd = "chmod -R u+x $conf{'LOCAL_FILE'}";
            if(system($cmd)){
                print "$cmd failed\n";
            }
            print "$cmd success\n";
        }
    }
    return 0;
}

#主函数
sub main{
    my %configtext = ();
    my $cmd;

    #解析配置文件
    my $ret = parse_destfile($config_file,\%configtext);
    if($ret){
        print "[Fail]download_construct::main failed,because of parse_destfile failed\n";
        return -1; 
    }   


    #依次构建数据项
    $deploy_path =~ s/\/+$//;
    foreach my $key(sort keys %configtext){
        my $cmd ;
        my $confarr = $configtext{$key};
        my $type = $confarr->{'type'};
        my $value = $confarr->{'key'};
        my $passwd = $confarr->{'passwd'};
        my $update_style = $confarr->{'update_style'};

        $value =~ s/;+/;/g;
        $value =~ s/\s+//g;
        $value =~ s/^;+//;
        $value =~ s/;+$//;

        my @sources = split(';',$value);
        if($#sources != 0){
            foreach my $source(@sources){
                $ret=download_sources($deploy_path,$key,$source,$passwd,$update_style,1);
                if($ret){
                    print STDOUT "[Fail] Download multi sources failed!\n";
                    return -1;
                }
            }
        }
        else{
            $ret = download_sources($deploy_path,$key,$value,$passwd,$update_style,0);
            if($ret){
                print STDOUT "[Fail] Download single sources failed!\n";
                return -1;
            }
        }
    }
    return 0;
}

$| = 1;
my $ret = main();
if($ret ne 0){
    exit 1;
}
exit 0;
