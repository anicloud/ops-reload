package rollback;
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
    @EXPORT=qw(rollback_files);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;

sub rollback_files {
    print_error("[Fail][ROLLBACK]","Not support now.", __FILE__, __LINE__);
    return -1;

    my $conf = shift;

    my $dest;
    my $ret;
    if (exists($conf->{'TOP_DIR'})) {
        $dest = $conf->{'TOP_DIR'};
    }else {
        print_error("[Fail][ROLLBACK]","rollback_all FAILed because:$!", __FILE__, __LINE__);
        return -1;
    }

    my $backup_path;
    if (exists($conf->{'BACKUP_DIR'})) {
        $backup_path = $conf->{'BACKUP_DIR'};
    }else {
        print_error("[Fail][ROLLBACK]","rollback_all FAILed because:$!", __FILE__, __LINE__);
        return -1;
    }
    if (not -e $dest) {
        $ret = system("mkdir -p $dest");
        if ($ret != 0) {
            print_error("[Fail][ROLLBACK]","mkdir -p $dest FAILed", __FILE__, __LINE__);
            return -1;
        }
    }
    my @file_list = ();
    if (-e "$backup_path") {
        $ret = get_filelist_of_dir("$backup_path", \@file_list);
        if ($ret != 0) {
            print_error("[Fail][ROLLBACK]","get_filelist_of_backup_dir", __FILE__, __LINE__);
            return -1;
        }
    }else {
        return -1;
    }
    foreach my $file(@file_list) {
        $ret = cp_file_leaf("$backup_path/".$file, "$backup_path"."bak/".$file);
        if ($ret != 0) {
            print_error("[Fail][ROLLBACK]","mv_file_leaf from $backup_path/$file to $dest/$file FAILed\n", __FILE__, __LINE__);
            return -1;
        }
        $ret = mv_file_leaf("$backup_path"."bak/".$file, "$dest/$file");
        if ($ret != 0) {
            print_error("[Fail][ROLLBACK]","mv_file_leaf from $backup_path/$file to $dest/$file FAILed\n", __FILE__, __LINE__);
            return -1;
        }
    }
}
