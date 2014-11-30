#encode utf-8
my $g_tmp_basedir;
my %operation_list = ();
my @argv;
BEGIN {
    use Cwd 'realpath';
    $g_tmp_basedir = __FILE__;
    $g_tmp_basedir = realpath($g_tmp_basedir);

    $g_tmp_basedir =~ s/[^\/]+$//;
    unshift( @INC, $g_tmp_basedir );
    @argv = @ARGV;
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;

use execute_work_flow;
use operations_map;

use check_all;

#@brief 创建任务开始的标记
#@param $ll_id 操作单id
#@note 在备份日志目录下创建start.tag文件
sub tag_start {
    my ($backupdir, $account) = @_;
    my $log_dir = $backupdir."log";
    my $startTag = "$log_dir/start\.tag";
    my $finishTag = "$log_dir/finish\.tag";
    if (not -e $log_dir) {
        mk_path($log_dir);
        print_log("[main]","mkdir $log_dir",__FILE__,__LINE__);
    }
    rm_file($finishTag);
    print_log("main","rm $finishTag",__FILE__,__LINE__);
    `touch $startTag`;
    print_log("main","touch $startTag",__FILE__,__LINE__);
    return 0;
}

#@brief 创建任务结束的标记
#@param $ll_id 操作单id
#@note 在备份日志目录下创建finish.tag文件
sub tag_finish {
    my ($backupdir, $account) = @_;
    my $log_dir = $backupdir."log";
    my $finishTag = "$log_dir/finish\.tag";
    if (not -e $log_dir) {
        print_error("[Fail][main]","没找到备份日志文件夹",__FILE__,__LINE__);
        return -1;
    }
    `touch $finishTag`;
    print_log("main","touch $finishTag",__FILE__,__LINE__);
    return 0;
}

sub parse_cmdline {
    my ($conf_text) = @_;

    my $args = {};
    @ARGV =@argv;

    my $ret = GetOptions( $args, "c:s");
    if ( !$ret ) {
        print_errpr("[Fail][main]","GetOptions parse error",__FILE__,__LINE__);
        return -1;
    }
    if (   ( not defined $args->{c} )
        || ( defined( $args->{c} ) && ( $args->{c} eq "" ) ) )
    {
        help();
        return -1;
    }

    if ( defined( $args->{c} ) && $args->{c} ne "" ) {
        $$conf_text = $args->{c};
    }
    return 0;
}

sub print_all{
    my $conf = shift;
    my $dest = 'src';
    
    print_filelist_of_dir("$dest");
}

sub main {
    my $ret       = 0;
    my $conf_text = "";
    my %global_params = ();
        
    $ret = parse_cmdline( \$conf_text );
    if ( $ret < 0 ) {
        print_error( "[Fail][parse cmdline error.]", __FILE__, __LINE__ );
        return -1;
    }

    $ret = chdir $g_tmp_basedir;
    if ( not $ret ) {
        print_error( "[Fail][chdir $g_tmp_basedir]", __FILE__, __LINE__ );
        return -1;
    }
    
    my %conf = ();
    my $operations_file = "src/.archer/operations/operation";
    if ( 0 != load_conf( $conf_text, \%conf ) ) {
        return -1;
    }
    if(-e $operations_file){
        if(0 !=load_operations($operations_file,\%conf)){
            return -1;
        }
    }
    if ( 0 != check_conf( \%conf ) ) {
        return -1;
    }
    my $backup_dir;
    if (exists($conf{'BACKUP_DIR'})) {
        $backup_dir = $conf{'BACKUP_DIR'};
    }
    my $account = trim(`echo \$USER`);
    #标记本次上线开始
    if (0 != check_all(\%conf)) {
        return -1;
    }
    #tag_start($backup_dir, $account);
    
    if ( 0 != init_operations_map(\%conf)) {
        return -1;
    }
    if ( 0 != init_operations(\%conf)) {
        return -1;
    }
    if ( 0 != execute_operations(\%conf)) {
        #tag_finish($backup_dir, $account);
        return -1;
    }
    if (exists($conf{'ONLY_TEST'}) && $conf{'ONLY_TEST'} eq "TRUE") {
        print STDOUT "所有操作检查成功SUCCESS\n";
    }else {
        #tag_finish($backup_dir, $account);
        #标记本次上线结束
        print STDOUT "所有操作成功SUCCESS\n";
    }
    return 0;
}
$| = 1;
my $ret = main();
if($ret == 0){
    exit 0;
}
else{
    exit 1;
}
