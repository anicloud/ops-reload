#!/usr/bin/perl -w
package backup_and_deploy_diff;
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
    @EXPORT=qw(backup_and_deploy_diff);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;

#智能diff的备份和部署逻辑
sub backup_and_deploy_diff {
    my ( $conf, $if_backup, $if_fullamount ) = @_;

    my $current = `pwd`;
    print_notice("[Notice][BACKUP_AND_DEPLOY]","CURRENT Directory",__FILE__,__LINE__);
    print "$current\n";

    my $ret = 0;

    #增量部署逻辑
    if($conf->{'DEPLOY_METHOD'} eq 'INCREASE'){
        #增量部署逻辑
        $ret = backup_and_deploy_increase($conf,$if_backup);
        if($ret != 0){
            print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_increase failed!",__FILE__,__LINE__);
            return -1;
        }
    }
    elsif($conf->{'DEPLOY_METHOD'} eq 'ALL'){
        #全量部署逻辑
        $ret = backup_and_deploy_all($conf,$if_backup);
        if($ret != 0){
            print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_all failed!",__FILE__,__LINE__);
            return -1;
        }
    }
    else{
        print_error("[Fail][BACKUP_AND_DEPLOY]","Unknown deploy method".$conf->{'METHOD'}."\n", __FILE__, __LINE__);
        return -1;
    }

    #bin和data组件绝对路径部署
    if(-e "src_abs"){
        $ret = backup_and_deploy_absolute_path($conf,"./src_abs",$if_backup);
        if($ret != 0){
            print_error("[Fail][BACKUP_AND_DEPLOY]","bin or data absolute backup and deploy failed!",__FILE__,__LINE__);
            return -1;
        }
    }
    return 0;
}

#bin和data组件绝对路径文件的备份和部署逻辑
sub backup_and_deploy_absolute_path{
    my ( $conf,$dir,$if_backup ) = @_;
    my $destpath ;
    my $backup_path;

    if( exists( $conf->{'TOP_DIR'} ) ){
        $destpath = $conf -> {'TOP_DIR'};
    }else {
        print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_absolute_path Failed because no top dir", __FILE__, __LINE__);
        return -1;
    }
    if ( exists( $conf->{'BACKUP_DIR'} ) ) {
        $backup_path = $conf->{'BACKUP_DIR'};
    }else {
        print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_all Failed because no backup path", __FILE__, __LINE__);
        return -1;
    }
    
    my @file_list = ();
    if(! -e $dir){
        print_error("[Fail][BACKUP_AND_DEPLOY]","$dir not exist!",__FILE__,__LINE__);
        return -1;
    }
    
    #遍历目录，获取目录下的所有的文件
    my $ret = get_filelist_of_dir( $dir,\@file_list );
    if( $ret != 0 ){
        print_error( "[Fail][BACKUP_AND_DEPLOY]","get_filelist_of_dir src_abs failed!",__FILE__,__LINE__ );
        return -1;
    }

    #对每个文件，进行备份和部署过程
    foreach my $file ( @file_list ){
        if(defined($if_backup) && $if_backup eq 'TRUE'){
            if( exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'}  eq 'TRUE' ){
                print "[Check][BACKUP_AND_DEPLOT][cp src_abs/$file ./$file]\n";
            }
            else{
                if(-e "/./$file"){
                    $ret = cp_file_leaf("/./$file","$backup_path/$file");
                    if( $ret != 0 ){
                        print_error("[Fail][BACKUP_AND_DEPLOY]","cp $file to $backup_path/$file failed!",__FILE__,__LINE__);
                        return -1;
                    }
                }
            }
        }
        if( exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq 'TRUE' ){
            print "[Check][BACKUP_AND_DEPLOY][mv src_abs/$file to ./$file]\n";
        }
        else{
            #文件与目录同名，先删除
            $ret = rm_file_directory_same_name("/./$file");
            if($ret != 0){
                print_error("[Fail][BACKUP_AND_DEPLOY]","rm $file failed!",__FILE__,__LINE__);
                return -1;
            }
            $ret = mv_file_leaf('src_abs/'.$file,'/./'.$file);
            if($ret != 0){
                 print_error("[Fail][BACKUP_AND_DEPLOY]","mv_file_leaf src_abs/$file to /./$file failed!",__FILE__,__LINE__);
                return -1;
            }
        }
    }
    return 0;
}

#判断文件与目录同名，如果是的话，就删除同名文件或者目录
sub rm_file_directory_same_name{
    my ( $file ) = @_;
    my $dirname = dirname($file);
    my $ret = 0;
    #部署文件，但是目标存在同名目录，需要先删除
    if($dirname eq '.' && -d $file){
        print_notice("[Notice][BACKUP_AND_DEPLOY]","$file is a file,but dest have a same directory!",__FILE__,__LINE__);
        $ret = rm_dir($file);
        if($ret != 0){
            print_error("[Fail][BACKUP_AND_DEPLOY]","rm_dir $file Failed!",__FILE__,__LINE__);
            return -1;
        }
    }

    #部署目录，但是目标存在同名文件，需要先删除
    if($dirname ne '.' && -f $file){
        print_notice("[Notice][BACKUP_AND_DEPLOY]","$file is a directory,but dest have a file with the same name!",__FILE__,__LINE__);
        $ret = rm_file($file);
        if ($ret != 0){
            print_error("[Fail][BACKUP_AND_DEPLOY]","rm_file $file Failed!",__FILE__,__LINE__);
            return -1;
        }
    }
    return 0;
}

#全量备份和部署逻辑
sub backup_and_deploy_all{
    my ($conf,$if_backup) = @_;
    my $dest;
    my $backup_path;
    my $ret = 0;
    if( exists($conf->{'TOP_DIR'}) ){
        $dest = $conf->{'TOP_DIR'};
    }
    else{
        print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_all failed because no top dir\n", __FILE__, __LINE__);
        return -1;
    }
    if( exists($conf->{'BACKUP_DIR'})){
        $backup_path = $conf->{'BACKUP_DIR'};
    }
    else{
        print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_all failed because no backup path\n", __FILE__, __LINE__);
        return -1;
    }

    #备份逻辑
    if (defined($if_backup) && $if_backup eq "TRUE") {
        if( exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE" ){
            print "[Check][mv $dest $backup_path/$dest]\n";
        }
        else{
            if(-e $dest){
                $ret = move_dir_to_dir($dest, "$backup_path/$dest");
                if ($ret != 0) {
                    print_error("[Fail][BACKUP_AND_DEPLOY]","mv $dest to $backup_path/$dest  failed\n", __FILE__, __LINE__);
                    return -1;
                }
            }
        }
    }

    #部署逻辑
    if (-e $dest && not -d $dest) {
        print_error("[Fail][BACKUP_AND_DEPLOY]","$dest is not a dir when fullamount deploy\n", __FILE__, __LINE__);
        return -1;
    }
    if (not -e $dest) {
        if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
            print "[Check][mkdir $dest]\n";
        }
        else{
            $ret = mk_path("$dest");
            if( $ret != 0 ){
                print_error("[Fail][BACKUP_AND_DEPLOY]","mk_path $dest failed!",__FILE__,__LINE__);
                return -1;
            }
        }
    }

    if(-e "src/noahdes") {
        $ret = move_dir_to_dir("src/noahdes", "src_bak");
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","Cannot mv src/noahdes to src_bak\n",__FILE__, __LINE__);
            return -1;
        }
    }
    if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
        print "[Check][mv 'src' $dest]\n";
    }
    else{
        $ret = move_dir_to_dir('src', $dest); 
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","mv 'src' to $dest failed\n",__FILE__, __LINE__);
            return -1;
        }
    }
    if (-e "src_bak") {
        $ret = move_dir_to_dir("src_bak", "src/noahdes");
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","Cannot mv src_bak to src/noahdes\n",__FILE__, __LINE__);
            return -1;
        }
    }

    return 0;
}

#diff增量备份和部署逻辑
sub backup_and_deploy_increase{
    my ( $conf,$if_backup ) = @_;
    my $dest;
    my $backup_path;
    my $if_test;
    my $ret = 0;
    if(exists($conf->{'TOP_DIR'})){
        $dest = $conf->{'TOP_DIR'};
    }
    else{
        print_error("[Fail][BACKUP_AND_DEPLOY]","no top dir",__FILE__,__LINE__);
        return -1;
    }
    if(exists($conf->{'BACKUP_DIR'})){
        $backup_path = $conf->{'BACKUP_DIR'};
    }
    else{
        print_error("[Fail][BACKUP_AND_DEPLOY]","no backup dir",__FILE__,__LINE__);
        return -1;
    }
    if($conf->{'DEPLOY_METHOD'} ne 'INCREASE'){
        print_error("[Fail][BACKUP_AND_DEPLOY]","Internal error!",__FILE__,__LINE__);
        return -1;
    }

#创建部署路径
    if (not -e $dest) {
        if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
            print "[Check][mkdir -p $dest]\n";
        }
        else{
            $ret = mk_path($dest);
            if ($ret != 0) {
                print_error("[Fail][BACKUP_AND_DEPLOY]","mkdir -p $dest failed\n",__FILE__, __LINE__);
                return -1;
            }
        }
    }

#获取src临时目录下的所有文件
    my @file_list = ();
    my @empty_dir_list = ();
    if (-e "src/noahdes") {
        $ret = move_dir_to_dir("src/noahdes", "src_bak");
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","Cannot mv src/noahdes to src_bak\n",__FILE__, __LINE__);
            return -1;
        }
    }
    $ret = get_filelist_of_dir('src', \@file_list, \@empty_dir_list);
    if ($ret != 0) {
        print_error("[Fail][BACKUP_AND_DEPLOY]","get_filelist_of_dir\n", __FILE__,__LINE__);
        return -1;
    }
# 依次部署每个文件
    foreach my $file(@file_list) {
        if (defined($if_backup) && $if_backup eq "TRUE") {
            if (-e "$dest/$file") {
                if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
                    print "[Check][cp $dest/$file $backup_path/$dest/$file]\n";
                }
                else{
                    $ret = cp_file_leaf("$dest/$file", "$backup_path/$dest/$file");
                    if ($ret != 0) {
                        print_error("[Fail][BACKUP_AND_DEPLOY]","cp_file_leaf from $dest/$file to $backup_path/$dest/$file failed",__FILE__,__LINE__);
                        return -1;
                    }
                }
            }
        }
        if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
            print "[Check][mv src/$file $dest/$file]\n";
        }
        else{
            $ret = mv_file_leaf('src/'.$file, "$dest/$file");
            if ($ret != 0) {
                print_error("[Fail][BACKUP_AND_DEPLOY]","mv_file_leaf from src/$file to $dest/$file failed\n", __FILE__, __LINE__);
                return -1;
            }
        }
    }
    if (-e "src_bak") {
        $ret = move_dir_to_dir("src_bak", "src/noahdes");
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","Cannot mv src_bak to src/noahdes\n",__FILE__, __LINE__);
            return -1;
        }
    }
#依次创建空目录
    foreach my $dir(@empty_dir_list) {
        if (not -e "$dest/$dir") {
            if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
                print "[Check][mkdir $dest/$dir]\n";
            }
            else{
                $ret = mk_path("$dest/$dir");
                if ($ret != 0) {
                    print_error("[Fail][BACKUP_AND_DEPLOY]","mk_path $dest/$dir failed:$!\n", __FILE__, __LINE__);
                    return -1;
                }
            }
        }
    }
    return 0;
#my ( $source_operations,$bin_operations,$data_operations ) = @_;
#   if(exists($conf->{'SOURCE_OPERATIONS'})){
#       $source_operations = $conf->{'SOURCE_OPERATIONS'};
#   }
#   else{
#       print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_all Failed because no source operation",__FILE__,__LINE__);
#       return -1;
#   }
#   if( exists($conf->{'BIN_OPERATIONS'}) ){
#       $bin_operations = $conf->{'BIN_OPERATIONS'};
#   }
#   else{
#       print_notice("[NOTICE][BACKUP_AND_DEPLOY]","no bin operations",__FILE__,__LINE__)
#   }
#   if( exists($conf->{'DATA_OPERATIONS'}) ){
#       $data_operations = $conf->{'DATA_OPERATIONS'};
#   }
#   else{
#       print_notice("[NOTICE][BACKUP_AND_DEPLOY]","no data operations",__FILE__,__LINE__);
#   }

#    #判断是否含有DIFF变量
#   if(! exists($conf -> {'DIFF'}) || $conf->{'DIFF'} ne 'TRUE'){
#       print_error("[Fail][BACKUP_AND_DEPLOY]","Internal error!",__FILE__,__LINE__);
#       return -1;
#   }

#    #判断是否为dry run
#   if(exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq 'TRUE'){
#       $if_test = 1;
#   }
#   #备份和部署.archer目录
#   $ret = backup_and_deploy_directory($dest,$backup_path,"./src/.archer",$if_backup,$if_test);
#   if($ret != 0){
#       print_error("[Fail][BACKUP_AND_DEPLOY]","backup and deploy .archer failed!",__FILE__,__LINE__);
#       return -1;
#   }
#
#   #bin组件的备份和部署逻辑
#   if(%{$bin_operations}){
#       $ret = backup_and_deploy_by_operations($dest,$backup_path,$bin_operations,$if_backup,$if_test);
#       if( $ret != 0 ){
#           print_error("[Fail][main]","backup_and_deploy bin operations failed!",__FILE__,__LINE__);
#           return -1;
#       }
#   }
#   #data组件的备份和部署逻辑
#   if(%{$data_operations}){
#       $ret = backup_and_deploy_by_operations($dest,$backup_path,$data_operations,$if_backup,$if_test);
#       if( $ret != 0 ){
#           print_error("[Fail][main]","backup_and_deploy data operations failed!",__FILE__,__LINE__);
#           return -1;
#       }
#   }
#   #source组件的备份和部署逻辑
#   if($source_operations){
#       $ret = backup_and_deploy_by_operations($dest,$backup_path,$source_operations,$if_backup,$if_test);
#       if( $ret != 0 ){
#           print_error("[Fail][main]","backup_and_deploy source operations failed!",__FILE__,__LINE__);
#           return -1;
#       }
#   }
#    return 0;
}

#根据操作序列，备份和部署
sub backup_and_deploy_by_operations{
    my ($dest,$backup_path,$operations,$if_backup,$if_test) = @_;
    my $ret = 0;
    if(!defined($operations) || !defined($backup_path) || !defined($dest)){
        print_error('[Fail][BACKUP_AND_DEPLOY]','backup data params not correct',__FILE__,__LINE__);
        return -1;
    }
    if(-e "src/noahdes"){
        $ret = move_dir_to_dir( "src/noahdes","src_bak");
        if($ret != 0){
            print_error("[Fail][BACKUP_AND_DEPLOY]","Cannot mv src/noahdes to src_bak\n", __FILE__, __LINE__);
            return -1;
        }
    }
    my $ret = 0;
    my $del = $operations->{'DEL'};

    #del操作序列不为空, 优先处理
    if($del){
        foreach my $file(@{$del}){
            my $path = ${%{$file}}{'file'};
            if (defined($if_backup) && $if_backup eq "TRUE") {
                if (-e "$dest/$path") {
                    if ($if_test == 1) {
                        print "[Check][cp $dest/$path $backup_path/$dest/$path]\n";
                    }
                    else{
                        $ret = cp_file_leaf("$dest/$path", "$backup_path/$dest/$path");
                        if ($ret != 0) {
                            print_error("[Fail][BACKUP_AND_DEPLOY]","cp_file_leaf from $dest/$path to $backup_path/$dest/$path Failed",__FILE__,__LINE__);
                            return -1;
                        }
                    }
                }
            }
        }
    }

    #对ADD和UPDATE操作序列进行备份和部署
    foreach my $ope(keys %{$operations}){
        my $value = $operations->{$ope};
        if($ope eq 'ADD' || $ope eq 'UPDATE'){
            foreach my $path(@{$value}){
                my $file = ${%{$path}}{'file'};
                if(defined($if_backup) && $if_backup eq 'TRUE'){
                    if (-e "$dest/$file") {
                        if (defined($if_test) && $if_test eq 1 ) {
                            print "[Check][cp $dest/$file $backup_path/$dest/$file]\n";
                        }
                        else{
                            $ret = cp_file_leaf("$dest/$file", "$backup_path/$dest/$file");
                            if ($ret != 0) {
                                print_error("[Fail][BACKUP_AND_DEPLOY]","cp_file_leaf from $dest/$file to $backup_path/$dest/$file failed",__FILE__,__LINE__);
                                return -1;
                            }
                        }
                    }
                }
                if(defined($if_test) && $if_test == 1){
                    print "[Check] mv src/$file to $dest/$file\n";
                }
                else{
                    if(-e "src/$file"){
                        $ret = mv_file_leaf('src/'.$file, "$dest/$file");
                        if ($ret != 0) {
                            print_error("[Fail][BACKUP_AND_DEPLOY]","mv_file_leaf from src/$file to $dest/$file failed\n", __FILE__, __LINE__);
                            return -1;
                        }
                    }
                }
            }
        }
        elsif($ope eq 'EMPTY'){
            foreach my $file(@{$value}){
                if(not -e "$dest/$file"){
                    if($if_test == 1){
                        print "[Check][mkdir $dest/$file]\n";
                    }
                    else{
                        $ret = mk_path("$dest/$file");
                        if($ret != 0){
                            print_error("[Fail][BACKUP_AND_DEPLOY]","mk_path $dest/$file Failed:$!", __FILE__, __LINE__);
                            return -1;
                        }
                    }
                }
            }
        }
    }
    if(-e "src_bak"){
        $ret = move_dir_to_dir("src_bak", "src/noahdes");
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","Cannot mv src_bak to src/noahdes\n", __FILE__, __LINE__);
            return -1;
        }
    }
    return 0;
}

#以目录为粒度，进行部署
sub backup_and_deploy_directory{
    my ($destpath,$backup_path,$dir,$if_backup,$if_test) = @_;
    my $ret = 0;
    my $basename = basename($dir);

    #备份
    if(defined($if_backup) && $if_backup eq 'TRUE'){
        if(-e "$destpath/$basename"){
            if(defined($if_test) && $if_test eq 1){
                print "[Check]mv $dir to $destpath/$basename \n";
            }
            else{
                $ret = move_dir_to_dir("$destpath/$basename","$backup_path/$destpath/$basename");
                if($ret != 0){
                    print_error("[Fail][BACKUP_AND_DEPLOY]","mv $dir to $backup_path/$destpath/$basename failed!",__FILE__,__LINE__);
                    return -1;
                }
            }
        }
    }
    #部署
    if(defined($if_test) && $if_test eq 1){
        print "[Check]mv .archer to $destpath/$basename \n";
    }
    else{
        $ret = move_dir_to_dir($dir,"$destpath/$basename");
        if($ret != 0){
            print_error("[Fail][BACKUP_AND_DEPLOY]","mv $dir to $destpath/$basename failed!",__FILE__,__LINE__);
            return -1;
        }
    }
    return 0;
}
$| = 1;
