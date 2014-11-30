#encode utf-8
my $g_tmp_basedir;
my %operation_list = ();

BEGIN {
    use Cwd 'realpath';
    $g_tmp_basedir = __FILE__;
    $g_tmp_basedir = realpath($g_tmp_basedir);

    $g_tmp_basedir =~ s/[^\/]+$//;
    unshift( @INC, $g_tmp_basedir );
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;
use checkout_md5;

my $file;
my $file_key;
my $tmp_md5_path;

use constant SCM_PATH => 'getprod@product.scm.baidu.com';

Getopt::Long::GetOptions
(
    'file=s' => \$file,
    'filekey=s' => \$file_key,
    'tmpmd5path=s' => \$tmp_md5_path,
) or die ShowHelp();

sub generate_scm_md5_path {
    my ($file, $file_key, $scm_md5_path, $passwd) = @_;

    my @scm_path_array = split(/:+/, $file_key);
    my $size = @scm_path_array;
    my $pos;
    if ($size == 3) {
        $$passwd = $scm_path_array[0];
        $$scm_md5_path = $scm_path_array[1].":".$scm_path_array[2];
    }
    else {
        $$passwd = "getprod";
        $$scm_md5_path = $file_key;
    }

    $pos = index($$scm_md5_path, "output");
    $$scm_md5_path = substr($$scm_md5_path, 0, $pos);
    $$scm_md5_path .= "*.md5";

    return 0;
}

sub main {
    my $ret = 0;
    my $scm_path = SCM_PATH.":/data/prod-\d\d/.*/.*_\d-\d-\d-\d.*_BL/output";
    if ( $file_key =~ m/$scm_path\/(*?)/) {
        $file = $1;
    }

    #校验数据文件路径，是否是产品库路径
    if (index($file_key, SCM_PATH) == -1) {
        print_error( "[Fail]not SCM path, can not download", __FILE__, __LINE__ );
        return -1;
    }

    #生成产品库md5地址
    my $scm_md5_path = "";
    my $passwd = "";
    $ret = generate_scm_md5_path($file, $file_key, \$scm_md5_path, \$passwd);
    if ($ret != 0) {
        print_error("[Fail]SCM path is not right", __FILE__, __LINE__ );
        return -1;
    }

    #下载总体md5文件到当前目录
    my $tmp_full_md5_path = "./scm_md5.md5";
    $ret = download_scm_md5($scm_md5_path, $tmp_full_md5_path, $passwd);
    if ($ret != 0) {
        print_error("[Fail]can not download scm md5", __FILE__, __LINE__ );
        #清除产品库md5文件
        my $cmd = "rm -rf $tmp_full_md5_path";
        if (0 != system($cmd)) {
            print_error("$cmd failed", __FILE__, __LINE__);
        }
        return -1;
    }

    #下载成功，获取文件md5值，获取成功，则存储到临时数据文件夹
    my $check_file = "./output/".$file;
    $ret = checkout_and_save_md5($check_file, $tmp_full_md5_path, $tmp_md5_path);
    if ($ret != 0) {
        print_error("checkout_and_save_md5 failed", __FILE__, __LINE__);
    }
    #清除产品库md5文件
    my $cmd = "rm -rf $tmp_full_md5_path";
    if (0 != system($cmd)) {
        print_error("$cmd failed", __FILE__, __LINE__);
        return -1;
    }

    return $ret;
}

sub checkout_and_save_md5 {
    my ($file, $tmp_full_md5_path, $tmp_md5_path) = @_;
    my $ret = 0;
    my $md5 = "";

    $ret = find_md5_text($tmp_full_md5_path, $file, \$md5);

    if ($ret != 0) {
        print_error("find md5 failed", __FILE__, __LINE__);
        return -1;
    }
    
    open(FILE, ">$tmp_md5_path");
    syswrite(FILE, $md5);
    close(FILE);

    return 0;
}

$| = 1;
my $ret = main();
exit($ret);
