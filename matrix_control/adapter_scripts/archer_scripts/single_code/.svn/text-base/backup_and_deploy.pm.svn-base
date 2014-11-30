#!/usr/bin/perl -w

package backup_and_deploy;
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
    @EXPORT=qw(backup_and_deploy_all);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;
use backup_and_deploy_diff;

sub backup_and_deploy_data {
    my ($conf, $if_backup) = @_;

    my $ret;
    my $dest;
    my $backup_path;
    if (exists($conf->{'TOP_DIR'})) {
        $dest = $conf->{'TOP_DIR'};
    }else {
        print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_all failed because no top dir\n", __FILE__, __LINE__);
        return -1;
    }

    if (exists($conf->{'BACKUP_DIR'})) {
        $backup_path = $conf->{'BACKUP_DIR'};
    }else {
        print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_all failed because no backup path\n", __FILE__, __LINE__);
        return -1;
    }

    if (not -e 'src_data') {
        return 0;
    }

    #注意：无论整体是增量还是全量，数据文件用增量上线
    if (not -e $dest) {
        if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
            print "[Check][mkdir -p $dest]\n";
        }else {
            $ret = system("mkdir -p $dest");
        }
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","mkdir -p $dest failed\n", __FILE__, __LINE__);
            return -1;
        }
    }

    my @file_list = ();
    $ret = get_filelist_of_dir('src_data', \@file_list);
    if ($ret != 0) {
        print_error("[Fail][BACKUP_AND_DEPLOY]","get_filelist_of_dir\n", __FILE__, __LINE__);
        return -1;
    }

    foreach my $file(@file_list) {
        if (defined($if_backup) && $if_backup eq "TRUE") {
            if (-e "/$file") {
                if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
                    #do nothing now
                }else {
                    $ret = system("mkdir -p $dest");
                }
                if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
                    #do nothing now
                }else {
                    $ret = cp_file_leaf("/$file", "$backup_path/$file");
                    if ($ret != 0) {
                        print_error("[Fail[BACKUP_AND_DEPLOY]","]cp_file_leaf from /$file to $backup_path/$file failed\n", __FILE__, __LINE__);
                        return -1;
                    }
                }
            }
        }
        if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
            #do nothing now
        }else {
            $ret = mv_file_leaf('src_data/'.$file, "/$file");
            if ($ret != 0) {
                print_error("[Fail][BACKUP_AND_DEPLOY]","mv_file_leaf from src_data/$file to /$file failed\n", __FILE__, __LINE__);
                return -1;
            }
        }
    }
    return 0;
}

sub backup_and_deploy_all_dir_replace {
    my ($conf, $if_backup) = @_;
    return backup_and_deploy_all($conf, $if_backup, 1);
}

sub backup_and_deploy_all {
    my ($conf, $if_backup, $if_fullamount) = @_;
    $if_backup = "\U$if_backup";

    #do not support backup. by wangfenglei
    $if_backup = "FALSE";

    my $current = `pwd`;
    print "CURRENT:$current\n";

    my $ret = 0;
    my $dest;
    my $backup_path;
    if (exists($conf->{'TOP_DIR'})) {
        $dest = $conf->{'TOP_DIR'};
    }else {
        print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_all failed because no top dir\n", __FILE__, __LINE__);
        return -1;
    }
    
    if (exists($conf->{'BACKUP_DIR'})) {
        $backup_path = $conf->{'BACKUP_DIR'};
    }else {
        print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_all failed because no backup path\n", __FILE__, __LINE__);
        return -1;
    }
    #支持diff增量部署
    if($conf->{'DIFF'} eq 'TRUE'){
        $ret = backup_and_deploy_diff($conf, $if_backup, $if_fullamount);
        if($ret != 0){
            print_error("[Fail][BACKUP_AND_DEPLOY]","backup_and_deploy_diff failed!",__FILE__,__LINE__);
        }
        return $ret;
    }

    #注意：增量上线不支持上空目录，目录只在上文件的过程中顺便创建
    if ($conf->{'DEPLOY_METHOD'} eq 'INCREASE') {
        if (not -e $dest) {
            if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
                print "[Check][mkdir -p $dest]\n";
            }else {
                $ret = system("mkdir -p $dest");
            }
            if ($ret != 0) {
                print_error("[Fail][BACKUP_AND_DEPLOY]","mkdir -p $dest failed\n", __FILE__, __LINE__);
                return -1;
            }
        }
    
        my @file_list = ();
        my @empty_dir_list = ();
        if (-e "src/noahdes") {
            $ret = move_dir_to_dir("src/noahdes", "src_bak");
        }
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","Cannot mv src/noahdes to src_bak\n", __FILE__, __LINE__);
            return -1;
        }
        $ret = get_filelist_of_dir('src', \@file_list, \@empty_dir_list);
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","get_filelist_of_dir\n", __FILE__, __LINE__);
            return -1;
        }
        foreach my $file(@file_list) {
            if (defined($if_backup) && $if_backup eq "TRUE") {
                if (-e "$dest/$file") {
                    if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
                        #print "[Check][cp $dest/$file $backup_path/$dest/$file]\n";
                    }else {
                        $ret = cp_file_leaf("$dest/$file", "$backup_path/$dest/$file");
                    }
                    if ($ret != 0) {
                        print_error("[Fail][BACKUP_AND_DEPLOY]","cp_file_leaf from $dest/$file to $backup_path/$dest/$file failed\n", __FILE__, __LINE__);
                        return -1;
                    }
                }
            }
            if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
                #print "[Check][mv src/$file $dest/$file]\n";
            }else {
                $ret = mv_file_leaf('src/'.$file, "$dest/$file");
            }
            if ($ret != 0) {
                print_error("[Fail][BACKUP_AND_DEPLOY]","mv_file_leaf from src/$file to $dest/$file failed\n", __FILE__, __LINE__);
                return -1;
            }
        }
        if (-e "src_bak") {
            $ret = move_dir_to_dir("src_bak", "src/noahdes");
        }
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","Cannot mv src_bak to src/noahdes\n", __FILE__, __LINE__);
            return -1;
        }
        foreach my $dir(@empty_dir_list) {
            if (not -e "$dest/$dir") {
                if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
                    print "[Check][mkdir $dest/$dir]\n";
                }else {
                    $ret = mk_path("$dest/$dir");
                }
                if ($ret != 0) {
                    print_error("[Fail][BACKUP_AND_DEPLOY]","mk_path $dest/$dir failed:$!\n", __FILE__, __LINE__);
                    return -1;
                }
            }
        }
    }elsif ($conf->{'DEPLOY_METHOD'} eq 'ALL') {
        if (defined($if_backup) && $if_backup eq "TRUE") {
            if (-e $dest && not -d $dest) {
                print_error("[Fail][BACKUP_AND_DEPLOY]","$dest is not a dir when fullamount deploy\n", __FILE__, __LINE__);
                return -1;
            }
            if (not -e $dest) {
                if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
                    print "[Check][mkdir $dest]\n";
                }else {
                    $ret = mk_path("$dest");
                }
            }
            if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
                print "[Check][mv $dest $backup_path/$dest]\n";
            }else {
                if(-e $dest){
                    $ret = move_dir_to_dir($dest, "$backup_path/$dest");
                    if ($ret != 0) {
                        print_error("[Fail][BACKUP_AND_DEPLOY]","mv $dest to "."$backup_path/$dest"." failed\n", __FILE__, __LINE__);
                        return -1;
                    }
                }
            }
        }
        if (-e "src/noahdes") {
            $ret = move_dir_to_dir("src/noahdes", "src_bak");
        }
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","Cannot mv src/noahdes to src_bak\n", __FILE__, __LINE__);
            return -1;
        }
        if (exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'} eq "TRUE") {
            print "[Check][mv 'src' $dest]\n";
        }else {
            $ret = move_dir_to_dir('src', $dest);
        }
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","mv 'src' to $dest failed\n", __FILE__, __LINE__);
            return -1;
        }
        if (-e "src_bak") {
            $ret = move_dir_to_dir("src_bak", "src/noahdes");
        }
        if ($ret != 0) {
            print_error("[Fail][BACKUP_AND_DEPLOY]","Cannot mv src_bak to src/noahdes\n", __FILE__, __LINE__);
            return -1;
        }
    }else {
        print_error("[Fail][BACKUP_AND_DEPLOY]","Unknown deploy method ".$conf->{'METHOD'}."\n", __FILE__, __LINE__);
        return -1;
    }
    #增加对bin和data绝对路径数据的处理过程
    if(-e 'src_abs'){
        my @file_list = ();
        my $ret = get_filelist_of_dir('src_abs',\@file_list);
        if($ret != 0){
            print_error("[Fail][BACKUP_AND_DEPLOY]","get_filelist_of_dir src_abs failed!",__FILE__,__LINE__);
            return -1;
        }
        foreach my $file(@file_list){
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
            if( exists($conf->{'ONLY_TEST'}) && $conf->{'ONLY_TEST'}  eq 'TRUE' ){
                print "[Check][BACKUP_AND_DEPLOT][mv src_abs/$file /./$file]\n";
            }
            else{
                $ret = mv_file_leaf('src_abs/'.$file,'/./'.$file);
                if($ret != 0){
                    print_error("[Fail][BACKUP_AND_DEPLOY]","mv_file_leaf src_abs/$file to /./$file failed!",__FILE__,__LINE__);
                    return -1;
                }
            }
        }
    }
    return backup_and_deploy_data($conf, $if_backup);
}
$| = 1;
