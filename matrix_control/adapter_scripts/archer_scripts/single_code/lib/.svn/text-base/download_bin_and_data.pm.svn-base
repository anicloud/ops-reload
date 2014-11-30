#!/usr/bin/perl -w
#encode utf-8
package lib::download_bin_and_data;

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
    use Exporter();
    use vars qw($VERSION @ISA @EXPORT);
    @ISA=qw(Exporter);
    @EXPORT=qw(download_bin_and_data);
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

use constant BIN_AND_DATA_SEP => 'BIN_AND_DATA_SEP';
my $config_file = "";
my $deploy_path = "/home/users/xuyang04/work/bs";
my $limit_rate= '3m';
my $account = $ENV{'USER'};
my $md5dir = "/home/$account/backup/ci_backup";
my $mode;
my $retry_count = 3;
my $is_fullamount;

#标准化ginko的下载地址
sub format_kingo_url{
    my ($url) = @_; 
    my ($scheme, $machine, $path);
    if ($url =~ /^((ftp|http):\/\/)([^\/]+)(.*)/) {
        $scheme = $1; 
        $machine = $3; 
        $path = $4; 

    }
    else{
        return  undef;
    }
    return $machine.":".$path;
}

#去重多数据源下载地址
sub unique_source_path{
    my ($original_conf,$dest_conf) = @_;
    my %sources = ();
    my $i = 0;
    foreach my $address(@{$original_conf}){
        my $source= ${%{$address}}{'source'};
        $sources{$source} = undef;
    }
    my @keys = keys %sources;
    foreach my $key(@keys){
        my @files;
        foreach my $address(@{$original_conf}){
            my $file = ${%{$address}}{'file'};
            my $source= ${%{$address}}{'source'};
            my $dest = ${%{$address}}{'destpath'};
            my $md5 = ${%{$address}}{'md5'};   
            my $last = $file.BIN_AND_DATA_SEP.$dest.BIN_AND_DATA_SEP.$md5;
            if($key eq $source){
                push(@files,$last);
            }
        }
        $$dest_conf{$key} = \@files;
    }
}

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
            my ($cc,$keys) = split(':',$key);
            $$conftext{$keys}{'type'} = 'data';
            $$conftext{$keys}{'key'} = $item->{'src'};
            $$conftext{$keys}{'update_style'} = 1;
         }
     }
     return 0;
}

#ftp方式下载指定地址数据
sub download_ftp{
    my ($remote,$local,$limit_rate,$retry_count,$is_p2p) = @_;
    
    if(defined($remote)){
        $remote =~ s/\/+/\//g;
        $remote =~ m/ftp:\/([^:\/]*):?(\/.*)/;

        $remote = "ftp://".$1.$2;
        print "$remote\n";
    }
    else{
        print STDOUT "[Fail]Undefined $remote! Please contact with RDs!!!";
        return -1;
    }

    my $cur_file_name = basename($remote);
    my $cut_num = get_cut_dir_num($remote);
    my $cmd;
    my $ret;

##p2p方式下载数据
    if(defined($is_p2p) && $is_p2p == 1){
    #p2p方式，其地址的格式，必须为:"hostname:path"
        
        my $source_path = format_kingo_url($remote);
        if(not defined($source_path)){
            print_error("[Fail]","formate kingo source path failed!",__FILE__,__LINE__);
            return -1;
        }
        $cmd = "gkocp -d $limit_rate \"$source_path\" \"$cur_file_name\" >/dev/null";
    }
    else{
        $ret =check_ftp_file($remote);
        $cmd = "wget -q --limit-rate=$limit_rate --level=40 -r -nH --cut-dir=$cut_num \"$remote\"";
        if($ret eq 0){
            $cmd = "wget -q --limit-rate=$limit_rate --level=40 -nH --cut-dir=$cut_num \"$remote\"";
        }
    }
    $ret = system($cmd);
    if($ret ne 0 and $retry_count ne 0){
        sleep(1);
        print "download_ftp Failed,retry....\n";
        download_ftp($remote,$local,$limit_rate,$retry_count-1,$is_p2p);
    }
    
    if($ret eq 0 ){
        #判断下载后的文件是文件还是目录，如果是文件，则其目的路径，需要加上文件名，否则会被重命名
        if(-f $cur_file_name){
            my $tmp_local = $local."/".$cur_file_name;
            $local = $tmp_local;
        }
        if(-d $cur_file_name){
            my $tmp = $local."/".$cur_file_name;
            if(-e $tmp){
               $ret = mk_path($tmp);
           }
           $local=$tmp;
        }
        if (-e $local){
            $cmd = "rm -rf $local";
            system($cmd);
        }
        if(-e $cur_file_name){
            my @params= ("mv", "-f", $cur_file_name, $local);
            system(@params);
        }
        else{
            print STDOUT "[Fail]Please check your ftp address!!\n";
        }
    }
    return $ret;
}

#scp方式下载指定地址数据
sub download_scp{
     my ($remote,$local,$passwd,$retry_count,$is_p2p) = @_;

     my $cmd="$g_tmp_basedir/sshpass -p $passwd /usr/bin/scp -oStrictHostKeyChecking=no -r -q $remote $local";;
     if(defined($is_p2p) && $is_p2p == 1){
        my ($username,$address) = split('@',$remote);
        if($address eq '' || index($address,':') == -1){
            print STDOUT "[Fail] address $remote not correct!\n";
        }
        $cmd = "gkocp -d $limit_rate \"$address\" \"$local\" >/dev/null";
     }
     print "download_scp CMD:$cmd\n";

     my $ret = system($cmd);
     if($ret ne 0 and $retry_count ne 0 ){
        sleep(1);
        print "download_scp failed,retry...\n";
        $ret = download_scp($remote,$local,$passwd,$retry_count-1,$is_p2p);
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

sub download_dynimac_data{
    my($deploy_path,$limit_rate) = @_;
    my %configtext = ();
    my $cmd;
    my $config_file = '';
    if(-e "./src/noahdes/data.des"){
        $config_file = "./src/noahdes/data.des";
    }
    else{
        print_error("[Notice][DOWNLOAD_DATA]","no data.des!",__FILE__,__LINE__);
        return 0;
    }

    my $ret = parse_destfile($config_file,\%configtext);
    if($ret){
        print "[Fail]download_construct::main failed,because of parse_destfile failed\n";
        return -1;
    }
    $deploy_path =~ s/\/+$//;

    foreach my $key(sort keys %configtext){
        my $cmd ;
        my $confarr = $configtext{$key};
        my $type = $confarr->{'type'};
        my $value = $confarr->{'key'};
        my $passwd = $confarr->{'passwd'};
        $key =~ s/\/+$//;
        $value =~ s/\/+$//;

        my $tmp_deploy_path = $deploy_path;
        my $data_basename = basename($key);
        my $data_dirname = dirname($key);
        my $data_remote_basename = basename($value);
        my $data_remote_dirname = dirname($value);
        my $remote_md5 = "$value.md5";
        # 数据文件临时目录，固定为 ../src/

        my $local_dir = $g_tmp_basedir."/../src/".$data_dirname;
        #临时数据文件
        #最终数据文件
        my $local_file = $local_dir."/".$data_basename;

        #最后部署路径
        my $full_data_path = $tmp_deploy_path."/".$key;
        my $tmp_local_file = $local_dir."/".$data_remote_basename;

        my $tmp_remote_md5_file = "$tmp_local_file.md5";
        #备份MD5文件路径
        my $backup_md5_dir = $md5dir."/".$tmp_deploy_path."/".$data_dirname;
        my $backup_md5_file = $backup_md5_dir."/".$data_basename.".md5";

        if($key =~ m/^\//  ne ""){
            $tmp_deploy_path = $data_dirname;
            $tmp_deploy_path =~ s/\/+$//;
            $local_dir = $g_tmp_basedir."/../src_abs/".$data_dirname;
            $local_file = $local_dir."/".$data_basename;
            $tmp_local_file = $local_dir."/".$data_remote_basename;
            $tmp_remote_md5_file = "$tmp_local_file.md5";
            $backup_md5_dir = $md5dir."/".$tmp_deploy_path;
            $full_data_path = $backup_md5_dir;

            if(-e $tmp_deploy_path){
                $cmd = "rm -rf $tmp_deploy_path";
                $ret = system($cmd);

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
        if($type eq 'data'){
            my $style = $confarr->{'update_style'};
            if($style == 1){
                $ret = download_bscp($value,$local_file,$limit_rate,$retry_count);
            }
            elsif($style == 2){
                $ret = upload_bscp($value,$limit_rate,$retry_count);
            }
            if($ret ne 0 ){
                print "[Fail]download_construct::main::download_bscp()failed\n ";
                return -1;
            }
        }
    }
    return 0;
}

#获取源数据下载方式
sub get_download_info{
    my ($tmp_src,$source,$type,$passwd) = @_;
    
    if(index($tmp_src,'ftp://') != -1){
        $$type = 'ftp';
        $$source = $tmp_src;
    }
    elsif(index($tmp_src,'@') !=-1){
         $$type = 'scp';
         my @tmp_arr = split(":",$tmp_src);
         if(scalar(@tmp_arr eq 3)){
             $$source= $tmp_arr[1].":".$tmp_arr[2];
             $$passwd= $tmp_arr[0];
         }   
         else{
             $$passwd='getprod';
             $$source = $tmp_src;
        }   
    }
    elsif(index($tmp_src,'data://') != -1){
        $$type='data';
        $$source = $tmp_src;
    }
    else{
        return -1;
    }
    return 0;
}

#ftp方式源数据过程
sub download_source_by_ftp{
    my ($conf,$is_p2p) = @_;
    my $value = $conf->{'VALUE'};
    my $local_file = $conf->{'LOCAL_FILE'};

    my $ret;
    my $cmd;

    #ftp下载
    $ret = download_ftp($value,$local_file,$limit_rate,3,$is_p2p);
    if($ret != 0 ){
        print_error("[Fail]","DOWNLOAD $value by ftp  failed!",__FILE__,__LINE__);
        return -1;
    }
    else{
        print "DOWNLOAD $value succeed!\n";
    }
    return 0;
}
#scp方式下载源数据
sub download_source_by_scp{
    my ($conf,$passwd,$is_p2p) = @_; 
    my $value = $conf->{'VALUE'};
    my $local_file = $conf->{'LOCAL_FILE'};
    my $local_dir =$conf->{'LOCAL_DIR'};

    my $ret;
    my $cmd;

    if(download_scp($value,$local_file,$passwd,$retry_count,$is_p2p) != 0){
        print " DOWNLOAD $value failed!\n";                
        return -1;  
    }
    else{
        print "DOWNLOAD $value succeed!\n";
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
    my ($conf,$style) = @_;
    my $value = $conf->{'VALUE'};
    my $local_file = $conf->{'LOCAL_FILE'};
    my $local_dir =$conf->{'LOCAL_DIR'};

    my $ret;
    
    if($style eq 1){
        $ret = download_bscp($value,$local_dir,$limit_rate,$retry_count);
#bscp 的hudoop方式，需要删除指定目录下的.list文件
    }
    elsif($style == 2){
        $ret = upload_bscp($value,$limit_rate,$retry_count);
    }
    
    if($ret ne 0){
        print STDOUT "[Fail]download_construct::main::download_bscp()failed\n ";
        return -1;
    }
    return 0;
}


sub set_single_deploy_relevant_path{
    my ($value,$passwd,$type,$conf) = @_;
    my $ret;
    my $cmd;
    
    if(!defined($value) || $value eq ''){
        return -1;
    }
    my $data_remote_basename = basename($value);
    my $data_remote_dirname = dirname($value);
    my $remote_md5 = "$value.md5";
    
    # 数据文件临时目录，固定为 ../src/
    my $local_dir = $g_tmp_basedir."/../src_tmp/";
                        
    # 最终数据文件
    my $local_file = $local_dir;
                                      
    if(! -e $local_dir){
        $ret = mk_path($local_dir);
        if($ret){
            print_error("[Fail][$mode]","download_bin_and_data:mk_path $local_dir failed!",__FILE__,__LINE__);
            return -1;
        }
    }

    $conf->{'LOCAL_FILE'} = $local_file;
    $conf->{'VALUE'} = $value;
    $conf->{'LOCAL_DIR'} = $local_dir;
}

sub deploy_to_dest{
    my ($files,$src) = @_;
    my ($file,$dest,$md5) = split(BIN_AND_DATA_SEP,$files);
    if(!defined($file) || !defined($dest)){
        print_error("[Fail]","Internal error.Please contact with rds",__FILE__,__LINE__);
        return -1;
    }
    my $tmp_deploy_path = $g_tmp_basedir."../src/".$file;
    if($dest =~ m/^[\/]/ ne ''){
        $tmp_deploy_path= $g_tmp_basedir."../src_abs/".$file;
    }
    
    my $ret ;
    my $local_dir=$g_tmp_basedir."../src_tmp/";
    my $len = length $dest;
    my $next = substr $file,$len;
    my $local_file= $local_dir."/".$next;

    my $dirname = dirname($tmp_deploy_path);
    if(! -e $dirname){
        $ret = mk_path($dirname);
    }
    my $tmp_md5_file = `md5sum "$local_file"`;
    my ($new_md5,$new_file) = split(/\s+/,"$tmp_md5_file");
    if($new_md5 ne $md5){
        print_error("[Fail][main]","md5 check FAILED!!return code $ret\n",__FILE__,__LINE__);
        print "Error Code: NDEC-995005\n";
        return -1;
    }
    my @params = ("mv","-f",$local_file,$tmp_deploy_path);
    $ret = system(@params);
    if($ret != 0){
        print_error("[Fail]","mv file [$local_file] to [$tmp_deploy_path] failed!",__FILE__,__LINE__);
        return -1;
    }
    return 0;
}
sub download_sources{
    my ($mode,$value,$passwd,$type,$is_p2p) =@_;
    my %conf = ();
    my $cmd;
    my $ret;
     
    $ret = set_single_deploy_relevant_path($value,$passwd,$type,\%conf);
    if($type eq 'data'){
        $ret = download_source_by_bscp(\%conf,1);
    }
    elsif($type eq 'ftp'){
        $ret = download_source_by_ftp(\%conf,$is_p2p);
    }
    elsif($type eq 'scp'){
        $ret = download_source_by_scp(\%conf,$passwd,$is_p2p);
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

sub download_bin_and_data{
    my ($conf,$mode) = @_;
    my $full_amount = 0;
    my $conf_operations = ();
    my $is_p2p =0;
    if(defined($mode) && $mode ne 'bin' && $mode ne 'data'){
        print_error("[Fail][$mode]","Undefined $mode components!\n",__FILE__,__LINE__);
        return 1;
    }
    if(defined($conf->{'P2P'}) && $conf->{'P2P'} eq 'TRUE'){
        $is_p2p = 1;
    }
    if($mode eq "bin" && !defined($conf->{'BIN_OPERATIONS'})){
        print_error("[Fail][$mode]","Bin components,but no BIN_OPERATIONS\n",__FILE__,__LINE__);
        return 1;
    }
    if($mode eq 'bin' && defined($conf->{'BIN_OPERATIONS'})){
        $conf_operations = $conf->{'BIN_OPERATIONS'};
    }
    if($mode eq "data" && !defined($conf->{'DATA_OPERATIONS'})){
        print_error("[Fail][$mode]","Data components,but no DATA_OPERATIONS\n",__FILE__,__LINE__);
        return 1;
    }
    if($mode eq 'data' && defined($conf->{'DATA_OPERATIONS'})){
        $conf_operations = $conf->{'DATA_OPERATIONS'};
    }
    if(!defined($conf->{'SOURCE_PATH'}) || $conf->{'SOURCE_PATH'} eq ""){
        print_error("[Fail][$mode]","No source path!\n",__FILE__,__LINE__);
        return 1;
    }
    if(defined($conf->{'DEPLOY_METHOD'}) && $conf->{'DEPLOY_METHOD'} eq 'ALL'){
        $full_amount = 1;
    }

    my $sourcepath = $conf->{'SOURCE_PATH'};
    if(!defined($conf->{'TOP_DIR'}) || $conf->{'TOP_DIR'} eq ""){
        print_error("[Fail][$mode]","No top dir!\n",__FILE__,__LINE__);
        return 1;
    }
    if(defined($conf->{'LIMIT_RATE'}) && $conf->{'LIMIT_RATE'} ne ""){
        $limit_rate = $conf->{'LIMIT_RATE'};
    }
    my $deploypath = $conf->{'TOP_DIR'};
    if($mode eq 'data' && -e "./src/noahdes/data.des"){
        my $ret = download_dynimac_data($deploypath,$limit_rate);
        if($ret != 0){
            print_error("[Fail][DOWNLOAD_DATA]","download dynimic data failed!",__FILE__,__LINE__);
            return -1;
        }
    }
    foreach my $operations(sort keys %{$conf_operations}){
        my $value = $conf_operations->{$operations};
        
        #空数组，不处理
        if(!defined($value)){
            next;
        }
        my $local_file;
        my $tmp_deploy_path;
        if($operations eq "ADD" || $operations eq "UPDATE"){
            my %download_conf = ();

            #将相同的源地址去重，避免重复下载
            unique_source_path($value,\%download_conf);
          
            foreach my $source(sort keys %download_conf){
                #分2个步骤进行，首先下载数据
                my $passwd ;
                my $key;
                my $type ;
                my $src;
                my $ret = get_download_info($source,\$src,\$type,\$passwd);
                $ret = download_sources($mode,$src,$passwd,$type,$is_p2p);
                if($ret != 0){
                    print_error("[Fail][$mode]","download $src failed!\n",__FILE__,__LINE__);
                    return -1;
                }
                foreach my $file(@{$download_conf{$source}}){
                    $ret = deploy_to_dest($file,$src);
                    if($ret != 0){
                        print_error("[Fail][$mode]","download $file failed!\n",__FILE__,__LINE__);
                        return -1;
                    }
                }
                if(-e $g_tmp_basedir."../src_tmp/"){
                    $ret = rm_dir($g_tmp_basedir."../src_tmp/");
                }
            }
        }
        if($operations eq "SKIP" && $full_amount == 1){
            foreach my $file(@{$value}){
                my $key = ${%{$file}}{'file'};
                my $value = $sourcepath."/".$key;
                print "SKIP operation  and full deploy,now Deal $key\n";

                my $data_basename = basename($key);
                my $data_dirname = dirname($key);
                $tmp_deploy_path = $deploypath;
                
                #数据文件临时目录，固定为../src/
                my $local_dir = $g_tmp_basedir."/../src/".$data_dirname;
                $local_file = $local_dir."/".$data_basename;
                my $full_data_path = $tmp_deploy_path."/".$key;
                
                if($key =~ m/^\// ne ""){ 
                    $tmp_deploy_path = $data_dirname;
                    $tmp_deploy_path =~ s/\/+$//;
                    $local_dir = $g_tmp_basedir."/../src_abs/".$data_dirname;
                    $local_file = $local_dir."/".$data_basename;
                    $full_data_path = $key;
                    
                    if(! -e $tmp_deploy_path){
                        my $ret = system("mkdir -p $tmp_deploy_path");
                        if($ret){
                            print "[Fail] mkdir -p $tmp_deploy_path Failed!\n";
                            return 1;
                        }
                    }
                }
                my $ret = cp_file_leaf($full_data_path,$local_file);
                if($ret ne 0){
                    print "[Fail] cp_file_leaf $full_data_path to $local_file Failed!\n";
                    return 1;
                }
                if(-e $full_data_path.".md5"){
                    my $source_md5 = $full_data_path.".md5";
                    my $dest_md5 = $local_file.".md5";
                    $ret = cp_file_leaf($source_md5,$dest_md5);
                    if($ret ne 0){
                        print "[Fail]cp_file_leaf from $full_data_path.md5 to $local_file.md5 Failed!\n";
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}
$| = 1;
