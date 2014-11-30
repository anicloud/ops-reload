#!/usr/bin/perl -w

package checkout_md5;
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
    @EXPORT=qw(checkout_file_md5 checkout_folder_md5 find_file_md5
        find_folder_md5 download_scm_md5 find_md5_text);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;

use constant SCM_MD5_BASIC_PATH => 'ftp://getprod:getprod@product.scm.baidu.com/data/';
use constant DEFAULT_MD5_SEP    => '  ';
use constant DEFAULT_COMMENT    => '#';

sub checkout_file_md5 {
    my ($module_name, $module_version, $module_build_platform, $check_file_path, $md5) = @_;

    #校验参数
    if (!defined($module_name) || $module_name eq "" ||
        !defined($module_version) || $module_version eq "" ||
        !defined($module_build_platform) || $module_build_platform eq "") {
        print_error("no --scm or module_version or module_build_platform is set", __FILE__, __LINE__);
        return -1;
    }
    if ($module_version =~ m/^\d+-\d+-\d+-\d+$/) {
        print_error("Illegal version of module,must be like x.x.x.x", __FILE__, __LINE__);
        return -1;
    }
    if (!$module_build_platform eq '32' && !$module_build_platform eq '64') {
        print_error("Illegal build platform", __FILE__, __LINE__);
        return -1;
    }
    if (!defined($check_file_path) || $check_file_path eq "") {
        print_error("no file path is set",  __FILE__, __LINE__);
        return -1;
    }

    #获取产品库md5文件
    my $source_path;
    my $tmp_path;
    generate_md5_source_des_path($module_name, $module_version, $module_build_platform, \$source_path, \$tmp_path);
    my $ret = 0;
    $ret = download_scm_md5($source_path, $tmp_path, "getprod");

    #获取md5文件失败，报错返回
    if ($ret != 0) {
        print_error("can not get md5", __FILE__, __LINE__);
    }else {
        #获取成功，在md5文件中查找待检文件的md5值
        $check_file_path = "./output/".$check_file_path;
        $ret = find_file_md5($tmp_path, $check_file_path, \$$md5);
        if ($ret != 0) {
                print_error("checkout md5 failed", __FILE__, __LINE__);
        }
    }

    #处理完成，清除产品库md5文件
    my $cmd = "rm -rf $tmp_path";
    if (0 != system($cmd)) {
        print_error("$cmd failed", __FILE__, __LINE__);
        return -1;
    }
    return $ret;
}

sub checkout_folder_md5 {
    my ($module_name, $module_version, $module_build_platform, $file_folder_path, $md5) = @_;

    #校验参数
    if (!defined($module_name) || $module_name eq "" ||
        !defined($module_version) || $module_version eq "" ||
        !defined($module_build_platform) || $module_build_platform eq "") {
        print_error("no --scm or module_version or module_build_platform is set", __FILE__, __LINE__);
        return -1;
    }
    if ($module_version =~ m/^\d+-\d+-\d+-\d+$/) {
        print_error("Illegal version of module,must be like x.x.x.x", __FILE__, __LINE__);
        return -1;
    }
    if (!$module_build_platform eq '32' && !$module_build_platform eq '64') {
        print_error("Illegal build platform", __FILE__, __LINE__);
        return -1;
    }
    if (!defined($file_folder_path) || $file_folder_path eq "") {
        print_error("no file folder path is set",  __FILE__, __LINE__);
        return -1;
    }

    #获取产品库md5文件
    my $source_path;
    my $tmp_path;
    generate_md5_source_des_path($module_name, $module_version, $module_build_platform,
                                  \$source_path, \$tmp_path);
    my $ret = 0;
    $ret = download_scm_md5($source_path, $tmp_path, "getprod");
    #获取md5文件失败，报错返回
    if ($ret != 0) {
        print_error("can not get md5", __FILE__, __LINE__);
    }else {
        #获取成功，在md5文件中查找待检文件夹md5的md5映射值
        my $file_folder_path_all = "./output/".$file_folder_path."/";
        $ret = find_folder_md5($tmp_path, $file_folder_path_all, $md5);
        if ($ret != 0) {
            print_error("checkout folder md5 failed", __FILE__, __LINE__);
        }
    }
    #处理完成，清除产品库md5文件
    my $cmd = "rm -rf $tmp_path";
    if (0 != system($cmd)) {
        print_error("$cmd failed", __FILE__, __LINE__);
        return -1;
    }
    return $ret;
}

sub download_scm_md5 {
    my ($scm_md5_path, $tmp_md5_path, $passwd) = @_;

    my $ret = 0;
    my $cmd = "./sshpass -p $passwd scp -oStrictHostKeyChecking=no $scm_md5_path $tmp_md5_path";

    $ret = system($cmd);
    if ($ret != 0) {
        print_error("$cmd failed", __FILE__, __LINE__);
        return -1;
    }
    return 0;
}

sub find_file_md5 {
    my ($md5_file, $file_path, $md5) = @_;

    my $line;
    my $pos = 0;
    my @data;

    open(FH, $md5_file);
    while(<FH>) {
        $line = trim($_);
        if ($line eq "") {
            next;
        }

        if (substr($line, 0, 1) eq  DEFAULT_COMMENT) {
            next;
        }

        $pos = index($line, DEFAULT_MD5_SEP, 0);
        if ($pos == -1) {
            print_log("format of md5 item error, not exist separate space.", __FILE__, __LINE__);
            next;
        }

        $data[0] = trim(substr($line, 0, $pos));
        $data[1] = trim(substr($line, $pos + 1));

        #print_log("now $data[0] : $data[1].", __FILE__, __LINE__);

        if ($data[1] eq "") {
            print_log("file item is NULL.", __FILE__, __LINE__);
            next;
        } elsif ($data[1] eq $file_path) {
            if ($data[0] eq "") {
                print_error("this $file_path has no right md5.", __FILE__, __LINE__);
                return -1;
            }
            else {
                $$md5 = $data[0];
                print_log("find $file_path md5 item: $$md5.", __FILE__, __LINE__);
                last;
            }
        }
    }
    close(FH);

    return 0;
}

sub find_md5_text {
    my ($md5_file, $file_path, $md5_text) = @_;

    my $line;
    my $pos = 0;
    my @data;
    my $find_file;

    #处理前缀
    my $start_pos = length($file_path);
    my $pre;
    generate_md5_pre($file_path, \$pre);

    open(FH, $md5_file);
    while(<FH>) {
        $line = trim($_);
        if ($line eq "") {
            next;
        }

        if (substr($line, 0, 1) eq  DEFAULT_COMMENT) {
            next;
        }

        $pos = index($line, DEFAULT_MD5_SEP, 0);
        if ($pos == -1) {
            print_log("format of md5 item error, not exist separate space.", __FILE__, __LINE__);
            next;
        }

        $data[0] = trim(substr($line, 0, $pos));
        $data[1] = trim(substr($line, $pos + 1));

        if ($data[1] eq "") {
            print_log("file item is NULL.", __FILE__, __LINE__);
            next;
        }elsif (index($data[1], $file_path) == 0) {
            if ($data[0] eq "") {
                print_error("this $file_path has no right md5.", __FILE__, __LINE__);
            }
            else {
                $find_file = trim(substr($data[1], $start_pos));
                if (length($find_file) == 0) {
                    $$md5_text .= "$data[0]  $pre\n";
                    last;
                }
                $$md5_text .= "$data[0]  $pre/$find_file\n";
            }
        }
    }
    close(FH);

    if ($$md5_text eq "") {
        return -1;
    }
    return 0;
}

sub generate_md5_pre {
    my ($file_path, $pre) = @_;
    my @dir_name = split(/\/+/, $file_path);
    my $size = @dir_name;
    $$pre = "./".$dir_name[$size - 1];
}

sub generate_md5_source_des_path {
    my ($module_name, $module_version, $module_build_platform,
        $source_path, $des_path) = @_;

    my $module_tail = get_filename_from_fullpath($module_name);

    my $product_dir;
    $module_version =~ s/\./-/g;
    $product_dir = $module_tail."_".$module_version."_PD_BL";

    my $md5_file = $product_dir . ".md5";

    $$source_path = realurl(SCM_MD5_BASIC_PATH."/prod-$module_build_platform/".$module_name."/".$product_dir ."/". $md5_file);
    $$des_path = "./" . $md5_file;
}
