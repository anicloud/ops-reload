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
    @EXPORT=qw(checkout_file_md5 checkout_folder_md5);
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
    my ($scm, $module_name, $module_version, $module_build_platform, $file_path) = @_;

    if (!defined($scm) || !defined($module_name) || $module_name eq "" ||
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
    if (!defined($file_path) || $file_path eq "") {
        print_error("no file path is set",  __FILE__, __LINE__);
        return -1;
    }

    my $real_md5_source_path = generate_md5_source_path($module_name, $module_version, $module_build_platform);

    my $ret = 0;
    my $cmd;

    $cmd = "wget $real_md5_source_path";
    $ret = system($cmd);
    if ($ret != 0) {
        print_error("$cmd failed", __FILE__, __LINE__);
        return -1;
    }

    #处理md5文件
    my $md5_file = generate_md5_file_name($module_name, $module_version, $module_build_platform);

    my $file = "./" . $md5_file;
    $file_path = "./output/".$file_path;
    my $md5 = "";
    my $line;
    my $pos = 0;
    my @data;

    open(FH, $file);
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
                $md5 = $data[0];
                print_log("find $file_path md5 item: $md5.", __FILE__, __LINE__);
                last;
            }
        }
    }
    close(FH);

    $cmd = "rm -rf $file";
    if (0 != system($cmd)) {
        print_error("$cmd failed", __FILE__, __LINE__);
        return -1;
    }
    return $md5;
}

sub generate_md5_source_path {
    my ($module_name, $module_version, $module_build_platform) = @_;

    my $module_tail = get_filename_from_fullpath($module_name);

    my $product_dir;
    $module_version =~ s/\./-/g;
    $product_dir = $module_tail."_".$module_version."_PD_BL";

    my $md5_file = $product_dir . ".md5";

    my $real_md5_source_path = realurl(SCM_MD5_BASIC_PATH."/prod-$module_build_platform/".$module_name."/".$product_dir ."/". $md5_file);
    print_log("Scm md5 source path:$real_md5_source_path", __FILE__, __LINE__);

    return $real_md5_source_path;
}

sub generate_md5_file_name {
    my ($module_name, $module_version, $module_build_platform) = @_;

    my $module_tail = get_filename_from_fullpath($module_name);

    my $product_dir;
    $module_version =~ s/\./-/g;
    $product_dir = $module_tail."_".$module_version."_PD_BL";

    my $md5_file = $product_dir . ".md5";
    print_log("Scm md5 file name:$md5_file", __FILE__, __LINE__);

    return $md5_file;
}

sub checkout_folder_md5 {
    my ($scm, $module_name, $module_version, $module_build_platform, $file_folder_path) = @_;

    if (!defined($scm) || !defined($module_name) || $module_name eq "" ||
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

    my $real_md5_source_path = generate_md5_source_path($module_name, $module_version, $module_build_platform);

    my $ret = 0;
    my $cmd;

    $cmd = "wget $real_md5_source_path";
    $ret = system($cmd);
    if ($ret != 0) {
        print_error("$cmd failed", __FILE__, __LINE__);
        return -1;
    }

    #处理md5文件
    my $md5_file = generate_md5_file_name($module_name, $module_version, $module_build_platform);

    my $file = "./" . $md5_file;
    my $file_folder_path_all = "./output/".$file_folder_path."/";
    my %md5 = ();
    my $line;
    my $pos = 0;
    my @data;
    my $findFile;

    print_log("file folder path: $file_folder_path_all", __FILE__, __LINE__);
    
    open(FH, $file);
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
        } elsif (index($line, $file_folder_path_all, 0) != -1) {
            if ($data[0] eq "") {
                print_error("this $file_folder_path_all has no right md5.", __FILE__, __LINE__);
                return -1;
            }
            else {
                $findFile = $file_folder_path."/".trim(substr($data[1], length($file_folder_path_all)));
                $md5{$findFile} = $data[0];
                #print_log("find $file_folder_path_all md5 item: $data[0] $data[1]", __FILE__, __LINE__);
            }
        }
    }
    close(FH);

    $cmd = "rm -rf $file";
    if (0 != system($cmd)) {
        print_error("$cmd failed", __FILE__, __LINE__);
        return -1;
    }

    return %md5;
}
