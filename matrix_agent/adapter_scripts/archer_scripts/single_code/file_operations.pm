#!/usr/bin/perl -w

package file_operations;
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
    @EXPORT=qw(move_file_to_file move_file_to_dir copy_file copy_dir copy_content_to_dir
    get_cut_dir_num mk_path rm_file rm_dir rm_empty_dir clear_dir_without_metaquote clear_dir move_dir_to_dir
    move_content_to_dir print_log print_notice print_error print_start get_now_tag get_filelist_of_dir mv_file_leaf cp_file_leaf
    get_filename_from_fullpath);
}

use strict;

use FileHandle;
use File::Path;
use File::Copy;
use lib_aos_utility;

use constant DEFAULT_COMMENT    => '#';
use constant DEFAULT_CONFIG_SEP    => ':';
use constant DEFAULT_FILE_OPEN_MODE    => '0700';

sub get_cut_dir_num{
    $_ = shift;

    s/\/+/\//g;
    m/ftp:\/([^:\/]*):?(\/.*)/;

    my @arr =split('/',$2);
    return @arr-2;
}
#用于将文件拷为另一个文件，目标是否存在都可，但如果存在，不可是文件夹
sub move_file_to_file {
    my ($src, $dest) = @_;
    if ((not -e $src) && (not -l $src)) {
        print "[Fail][mv的源$src不存在!]\n";
        return -1;
    }
    if (-e $dest && -d $dest && (not -l $dest)) {
        print "[Fail][$dest是一个文件夹，$dest移动的目标应为一个文件!]\n";
        return -1;
    }
    my @params = ("mv", "-f", $src, $dest);
    if (0 != system(@params)) {
        print "[Fail][移动 file $src 到 $dest失败，原因是:$!]\n";
        return -1;
    }else {
        return 0;
    }
}

#用于将文件夹拷为另一个文件夹，但如果另一个文件夹已经存在了，就先删除之
sub move_dir_to_dir {
    my ($src, $dest) = @_;
    if (not -e $src) {
        print "[Fail][mv的源$src不存在!]\n";
        return -1;
    }
    if (-e $dest && not -d $dest) {
        print "[Fail][$dest是一个文件夹，$dest移动的目标应为一个文件!]\n";
        return -1;
    }
    my $parent = dirname($dest);
    #print "[TRACE][$dest 's parent :$parent]\n";
    if (-e $parent && not -d $parent) {
        print "[Fail][$dest的父路径$parent已经存在并且不是目录:$!]\n";
        return -1;
    }
    if (not -e $parent) {
        if (0 != mk_path($parent)) {
            print "[Fail][$dest的父路径$parent不存在且创建失败:$!]\n";
            return -1;
        }
    }
    if (-e $dest && -d $dest) {
        rm_dir($dest);
    }
    my @params = ("mv", $src, $dest);
    if (0 != system(@params)) {
        print "[Fail][移动 dir $src 到 $dest失败，原因是:$!]\n";
        return -1;
    }else {
        return 0;
    }
}

#用于将文件拷入一个文件夹，目标是否存在都可，但如果存在，必须是文件夹
sub move_file_to_dir {
    my ($src, $dest) = @_;
    if ((not -e $src) && (not -l $src)) {
        print "[Fail][mv的源$src不存在!]\n";
        return -1;
    }
    if (-e $dest && not -d $dest) {
        print "[Fail][$dest移动的目标应为一个文件夹!]\n";
        return -1;
    }
    my @params = ("mv", "-f", $src, $dest);
    if (1 != system(@params)) {
        print "[Fail][移动 file $src 到 $dest失败，原因是:$!]\n";
        return -1;
    }else {
        return 0;
    }
}

#用于将文件夹下的所有内容拷入另一个文件夹，源必须存在且为文件夹
#目标是否存在都可，但如果存在，必须是文件夹
sub move_content_to_dir {
    my ($src, $dest) = @_;
    if (not -e $src) {
        print "[Fail][mv的源$src不存在!]\n";
        return -1;
    }
    if (not -d $src) {
        print "[Fail][源$src应为一个文件夹!]\n";
        return -1;
    }
    if (-e $dest && not -d $dest) {
        print "[Fail][$dest移动的目标应为一个文件夹!]\n";
        return -1;
    }
    my @params = ("mv", "-f", $src.'/*', $dest);
    if (0 != system(@params)) {
        print "[Fail][移动$src目录下所有内容到 $dest失败，原因是:$!]\n";
        return -1;
    }else {
        return 0;
    }
}

#用于将一个文件拷为另一个文件，目标是否存在均可，但存在时不可是文件夹
sub copy_file {
    my ($source, $destination) = @_;
    if ((not -e $source) && (not -l $source)) {
        print "[Fail][cp的源$source不存在!]\n";
        return -1;
    }
    if (-e $destination && -d $destination && (not -l $destination)) {
        print "[Fail][$destination是一个文件夹，$source复制的目标应为一个文件!]\n";
        return -1;
    }
    my $src = format_path($source);
    my $dest = format_path($destination);
    my $options = '-d';
    if (-p $src){
        $options = '-dr';
    }
    my @params = ("cp", $options, $src, $dest);
    if (0 != system(@params)) {
        print "[Fail][cp -d $src $dest失败，原因是$!]\n";
        return -1;
    }
    return 0;
}

#用于将一个文件夹拷入另一个文件夹，目标是否存在均可，但存在时必须是文件夹
sub copy_dir {
    my ($src, $dest) = @_;
    if (-e $dest && (not -d $dest)) {
        print "[Fail][$dest存在且不是文件夹!]\n";
        return -1;
    }
    my @params = ("cp", "-r", "-d", $src, $dest);
    if (0 != system(@params)) {
        print "[Fail][cp -rd $src $dest失败，原因是$!]\n";
        return -1;
    }
    return 0;
}

#用于将一个文件夹的内容拷入另一个文件夹，目标是否存在均可，但存在时必须是文件夹
sub copy_content_to_dir {
    my ($src, $dest) = @_;
    if (not -e $src) {
        print "[Fail][copy的源$src不存在!]\n";
        return -1;
    }
    if (-e $dest && (not -d $dest)) {
        print "[Fail][$dest存在且不是文件夹!]\n";
        return -1;
    }
    my @params = ("cp", "-rd", $src.'/*', $dest);
    if (0 != system(@params)) {
        print "[Fail][cp -rd '$src'/* '$dest'失败，原因是$!]\n";
        return -1;
    }
    return 0;
}

#创建文件夹，如果目标已经存在，直接返回成功，如果不指定mode，默认权限为0755
sub mk_path {
    my ($dir,$mode) = @_;
    my $ret = -1;
    if (defined $mode) {
        $ret = mkpath($dir, 0, $mode);
    }else {
        $ret = mkpath($dir, 0, 0775);
    }
    if (-e $dir && -d $dir) {
        return 0;
    }elsif (-e $dir && (not -d $dir)) {
        print "[Fail][要创建文件夹的对应路径下存在一个同名且非文件夹的物体] \n";
        print STDOUT "[Fail]Error Code:NDEC-990001\n";
        return -1;
    }
    unless ($ret =~ m/[0-9]+/) {
        print "[Fail][创建路径$dir失败,$ret,原因是:$!] \n";
        print STDOUT "[Fail]Error Code:NDEC-990001\n";
        return -1;
    }else {
        return 0;
    }
}

#删除文件，如果文件不存在直接返回成功
sub rm_file {
    my ($file) = @_;
    my @params = ("rm", "-f", $file);
    if (-e $file && 0 != system(@params)) {
        print "[Fail][删除文件$file失败，原因是:$!]\n";
        return -1;
    }else {
        return 0;
    }
}

#删除文件，如果文件不存在直接返回成功
sub rm_empty_dir {
    my ($dir) = @_;
    if (-e $dir && 1 != rmdir($dir)) {
         print "[Fail][删除空文件夹$dir失败，原因是:$!]\n";
        return -1;
    }else {
        return 0;
    }
}
#清空文件夹，不支持元字符
sub clear_dir_without_metaquote {
    my ($dir) = @_;
    if (not -e $dir) {
        return 0;
    }
    my @params = ("rm", "-rf", $dir.'/*');
    if (0 != system(@params)) {
        print "[Fail][使用rm -rf $dir/*清空文件夹$dir失败，原因是:$!]\n";
    }
    return 0;
}

#清空文件夹，支持元字符
sub clear_dir {
    my ($dir) = @_;
    if (not -e $dir) {
        return 0;
    }
    if (1 != rmtree($dir)) {
        print "[Fail][删除文件夹$dir失败，原因是:$!]\n";
        return -1;
    }
    if (1 != mkpath($dir)) {
        print "[Fail][创建文件夹$dir失败，原因是:$!]\n";
        return -1;
    }
    return 0;
}

#删除文件夹，如不存在直接返回成功
sub rm_dir {
    my ($dir) = @_;
    $dir = trim($dir);
    if ($dir eq '') {
        print "[Fail][本操作将删除/*，很危险！]\n";
        return -1;
    }
    if (not -e $dir) {
        return 0;
    }
    my @params = ("rm", "-r", "-f", $dir);
    if (0 != system(@params)) {
        print "[Fail][删除文件夹$dir失败，原因是:$!]\n";
        return -1;
    }else {
        return 0;
    }
}

sub print_log
{
    my ($method,$string, $file, $line) = @_;
    use File::Basename;
    $file = basename($file);
    chomp $string;
    my $components = '[Trace][';
    if($method eq 'main'){
        $components .='main]'
    }
    else{
        $components .= $method."]";
    }
    print STDOUT get_now().$components . "[" . $file . ":" . $line ."]".$string."\n";
}

sub get_now
{
    my ($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = localtime(time);
    my $now_time = sprintf("%d.%d.%d %d:%d:%d", $year + 1900, $mon + 1, $mday, $hour, $min, $sec);
    
    return "[$now_time]";
}

sub get_now_tag
{
    my ($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = localtime(time);
    my $now_time = sprintf("%d_%d_%d_%d_%d_%d", $year + 1900, $mon + 1, $mday, $hour, $min, $sec);
    
    return "$now_time";
}

sub print_error
{
    my ($string,$string2, $file, $line) = @_;
    use File::Basename;
    $file = basename($file);
    chomp $string;
    
    print STDOUT get_now().$string . "[" . $file . ":" . $line ."]".$string2."\n";

}
sub print_start{
    my($string) = @_;
    my $len = length $string;
    my $left = (100-$len)/2;
    my $right = 100-$left-$len;
    
    my $leftString = '='x $left;
    my $rightString = '='x $right;
    
    print STDOUT $leftString.$string.$rightString."\n";
}
sub print_notice{
    my ($string,$string2, $file, $line) = @_;
    use File::Basename;
    $file = basename($file);
    chomp $string;

    print STDOUT get_now().$string . "[" . $file . ":" . $line ."]".$string2."\n";
}
#@brief 用于将文件夹展成文件列表
#@param $dir_path 文件夹路径
#@param $files 文件列表数组的引用
#@param $tag 是否统计空文件夹,1为是，0为否
sub get_filelist_of_dir {
    my ($dir_path, $files, $empty_dirs, $tag)  = @_;
    chdir "$dir_path";
    my @dirs = ( './' );

    my ($dir, $file);
    while ($dir = pop(@dirs)) {
        local *DH;
        if (!opendir (DH, $dir)) {
            print "[Fail][无法打开文件夹 $dir: $!] \n";
            next;
        }
        my $file_count = 0;
        foreach (readdir(DH)) {
            if ($_ eq '.' || $_ eq '..') {
                next;
            }
            $file = $dir . $_;
            if (!-l $file && -d $file) {
                $file .= '/';
                push (@dirs, $file);
                if (defined $tag && $tag) {
                    unshift (@$files, $file);
                }
            }else {
                unshift (@$files, $file);
            }
            $file_count ++;
        }
        if ($file_count == 0) {
            unshift (@$empty_dirs, $dir);
        }
        closedir(DH);
    }
    chdir '..';
    return 0;
}

sub mv_file_leaf{
    my ($src, $dest) = @_;
    my $parent_dir = dirname($dest);
    my $ret;

    if (not -e $parent_dir) {
        my @params = ("mkdir", "-p", $parent_dir);
        $ret = system(@params);
        if ($ret != 0) {
            print_error("[Fail] mkdir -p $parent_dir failed:$!\n", __FILE__, __LINE__);
        }
    }
    return move_file_to_file($src, $dest);
}

sub cp_file_leaf{
    my ($src, $dest) = @_;
    my $parent_dir = dirname($dest);
    my $ret;
    if (not -e $parent_dir) {
        my @params = ("mkdir", "-p", $parent_dir);
        $ret = system(@params);
        if ($ret != 0) {
            print_error("[Fail] mkdir -p $parent_dir failed:$!\n", __FILE__, __LINE__);
        }
    }
    return copy_file($src, $dest);
}

# @brief mv_dir_leaf, 将一个目录的内容移到另外一个目录。只有目录树末端的内容被移动。
# 可以指定如何对待软连接，可选的方式是忽略和跟踪。可以支持跨文件系统的移动，实际上
# 跨文件系统的支持来自mv(1)。
# @param src - 需要移动的源目录树根。
# @param dest - 移动的目的目录树根。
# @param follow_link - 指定是否跟踪软链，如果值是1，跟踪软链，否则忽略。\
# @return 成功0， 失败-1。
sub mv_dir_leaf{
    my ($src, $dest, $follow_link) = @_;
    my $cmd;
    my $err_msg;
    my $cmd_output;
    my $ret;
    my $src_mode;
    my $dest_mode;
    if(!defined $src || !defined $dest){
        print_error("[$$] Null parameter", __FILE__, __LINE__);
        return -1;
    }
    if(!defined $follow_link || $follow_link != 1){
        $follow_link = 0;
    }
    if($follow_link == 1){
=h
当前版本不支持源的软链，要获得源软链支持，解开下面的注释。并将后面软链源
分支的返回值改为正常0
        my $src_cp = $src;
        $src = realpath($src);
        if(!defined $src){
            print_error("[$$] realpath [ $src_cp ] failed. [ $! ]", __FILE__,  __LINE__);
            return -1;
        }
=cut
        my $dest_cp = $dest;
        $dest = realpath($dest);
        if(!defined $dest){
            print_error("[$$] realpath [ $dest_cp ] failed. [$! ]", __FILE__, __LINE__);
            return -1;
        }
    }
    if(!-e $dest){
#When moving files around, shell command 'mv' is called instead of some
#function because 'mv' works between different filesystems.
        $ret = move_file_to_file($src, $dest);
        if($ret != 0){
            $err_msg = "move $src to $dest failed. [ $! ]";
            print_error("[$$] $!", __FILE__, __LINE__);
            return -1;
        }
        return 0;
    }
    $src_mode = (stat($src))[2];
    $dest_mode = (stat($dest))[2];
    if(S_ISDIR($src_mode)){
        if(S_ISDIR($dest_mode)){
            $ret = opendir SRCDIR, $src;
            if(!defined $ret || !$ret){
                $err_msg = "Open dir [ $src ] failed. [ $! ]";
                print_error("[$$] $err_msg", __FILE__, __LINE__);
                return -1;
            }
            my @dirents = readdir SRCDIR;
            close SRCDIR;
            foreach my $dirent(@dirents){
                if($dirent eq '.' || $dirent eq '..'){
                    next;
                }
                my $sub_src = "$src/$dirent";
                my $sub_dest = "$dest/$dirent";
                $ret = mv_dir_leaf($sub_src, $sub_dest, $follow_link);
                if($ret < 0){
                    return -1;
                }
            }
            if(S_ISLNK($dest_mode)){
                $err_msg = "Symlink dest [ $dest ] ignored.";
                return -1;
            }
        }
        else{
            $err_msg = "Cannot overwrite non-dir [ $dest ] with dir [$src]";
            print_error("[$$] $err_msg", __FILE__, __LINE__);
            return -1;
        }
    }
    elsif(S_ISREG($src_mode)){
        if(S_ISREG($dest_mode)){
            $ret = move_file_to_file($src, $dest);
            if($ret != 0){
                $err_msg = "move $src to $dest failed. [ $cmd_output]";
                print_error("[$$] $err_msg", __FILE__, __LINE__);
                return -1;
            }
        }
        else{
            $err_msg = "Cannot overwrite non-regular file[ $dest ] with regular file [ $src ]";
            print_error("[$$] $err_msg", __FILE__, __LINE__);
        return -1;
        }
    }
    elsif(S_ISLNK($src_mode)){
        $err_msg = "Symlink src [ $src ] ignored";
    print_error("[$$] $err_msg", __FILE__, __LINE__);
        return -1;
    }
    else{
        $err_msg = "[ $src ] is not dir or regular file or symbolic link";
        print_error("[$$] $err_msg", __FILE__, __LINE__);
        return -1;
    }
    return 0;
}

# 从一个地址里面，获取文件名，可用于url
# 算法是，找到最后一个"/"，获取它后面的字符串
sub get_filename_from_fullpath
{
    my ($str) = @_;

    my $position = rindex $str, "/";
    if ($position <= 0)
    {
        print STDOUT "字符串$str不包含\/错误";
        return undef;
    }
    my $ret = substr $str, $position+1;  
    return $ret;  
}
$| = 1;
