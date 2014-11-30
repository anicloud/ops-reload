#!/usr/bin/perl -w



package lib_aos_utility;
my $g_tmp_basedir;
BEGIN {
    use Cwd 'realpath';
    $g_tmp_basedir = __FILE__ ;
    $g_tmp_basedir = realpath($g_tmp_basedir);
    $g_tmp_basedir =~ s/[^\/]+$//;
    
    unshift(@INC, $g_tmp_basedir);
    use Exporter();
    use vars qw(@ISA @EXPORT);
    @ISA=qw(Exporter);
    @EXPORT=qw(trim help format_kingo_url get_local_host_name block_machine unblock_machine load_conf
    print_filelist_of_dir get_compressed_file_from_dir realurl format_path parse_params);
}

use strict;
use Getopt::Long;
use FileHandle;
use File::Path;
use File::Copy;
use file_operations;

use constant DEFAULT_SU_TAG    => '/';
use constant DEFAULT_CONFIG_SEP    => ':';
use constant DEFAULT_FILE_OPEN_MODE    => '0700';
use constant DEFAULT_COMMENT    => '#';

my $VERSION = 1.0.0;
my $BLOCK_TOOL_PATH = "./lib/block";
my $DEBUG_FLAG = 1;

sub trim
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}

sub help {
    printf STDOUT "noah-deploy-single version:$VERSION\n";
    printf STDOUT "-c conf file\n";
    printf STDOUT "-t whether only test\n";
    printf STDOUT "-f set the config file\n";
    printf STDOUT "-s source path\n";
    printf STDOUT "-d local deploy path\n";
    printf STDOUT "--serverlist servicelist\n";
    printf STDOUT "-g group name\n";
    printf STDOUT "--outline donnot affect noah\n";
    printf STDOUT "--rollback rollback the backup dir\n";
    printf STDOUT "\n";
}

sub check_envionments{
    return 0;
}
sub get_local_host_name{
    my ($host_name) = @_;
    my $tmp_machine_name = `hostname -f`;
    if ( $? != 0 ) {
        print_log( "[Fail][hostname -f][$!]", __FILE__, __LINE__ );
        return -1;
    }

    my $machine_name = trim($tmp_machine_name);
    $machine_name =~ s/.baidu.com$//i;
    if ( length($machine_name) <= 0 ) {
        print_log( "[Fail][Illegal machine name $tmp_machine_name]", __FILE__, __LINE__ );
        return -1;
    }
    $$host_name = $machine_name;
    return 0;
}
sub block_machine{
    my ($is_block_alarm, $conf) = @_;
    if($is_block_alarm ne "TRUE"){
        print_log("[Notice][No need to block alarm][BLOCK_ALARM conf is $is_block_alarm]", __FILE__, __LINE__);
        return 0;
    }

    my $host_name = "";
    if(get_local_host_name(\$host_name) != 0){
        return -1;
    }
    
    my $ifGot = 0;
    my $cmd = "$BLOCK_TOOL_PATH showRuleIdList -h $host_name" ;
    my $cmd_output = "";
    my @results;
    my $i = 0;
    $conf->{'BLOCK_LIST'} = "";
    for(; $i<3; $i++){
        $cmd_output = `$cmd`;
        @results = split("\n", $cmd_output);
        $ifGot = 0;
        foreach my $result(@results) {
            my @tmp_line = split(":", $result);
            if (trim($tmp_line[0]) eq "hostname") {
                $ifGot = 1;
            }
            if (trim($tmp_line[0]) eq "unblock") {
                if (defined($tmp_line[1])) {
                    $conf->{'BLOCK_LIST'} = $tmp_line[1];
                    print_log("[Notice][$cmd][Got unblock rules list:".$tmp_line[1]." ]", __FILE__, __LINE__);
                }else {
                    print_log("[Notice][$cmd][All rules are blocked]", __FILE__, __LINE__);
                }
            }
        }
        if($ifGot == 0){
            print_log("[Notice][$cmd][Block tools does not know the host: $host_name]", __FILE__, __LINE__);
            sleep 3;
            next;
        }else {
            print_log("[Ok][$cmd][Block tools show hostname ok]", __FILE__, __LINE__);
            last;
        }
    }

    $cmd = "$BLOCK_TOOL_PATH add -h $host_name" ;
    my $moduleccess = 0;
    for (; $i<3; $i++) {
        $cmd_output = `$cmd`;
        if (trim($cmd_output) eq "OK!") {
            $moduleccess = 1;
            last;
        }else {
            sleep 3;
            next;
        }
    }
    if ($moduleccess == 1) {
        print_log("[Ok][$cmd][$cmd_output]", __FILE__, __LINE__); 
        return 0;
    }else {
        print_log("[Fail][$cmd][$cmd_output]", __FILE__, __LINE__); 
        return -1;
    }
}

sub load_conf {
    my ($conf_file, $config_item) = @_;
    my $line;
    my @data;
    my $fh;
    my $err_msg;
    my $pos = 0;

    if (!defined($conf_file) || $conf_file eq "") {
        $err_msg = "[load_conf] conf_file is NULL";

        return -1;
    }

    $fh = fopen($conf_file, "r");
    if ($fh < 0) {
        print "[load_conf] Cannot open conf file [$conf_file] to read";

        return -1;
    } else {
        while($line = <$fh>) {
            chomp($line);

            $line = trim($line);
            if ($line eq "") {
                next;
            }
            #
            if (substr($line, 0, 1) eq  DEFAULT_COMMENT) {
                next;
            }
            #
            $pos = index($line, DEFAULT_CONFIG_SEP, 0);
            if ($pos == -1) {
                print "[load_conf] format of conf item error, not exist separate char. [$conf_file] [$line]";
                    

                next;
            }

            #
            $data[0] = trim(substr($line, 0, $pos));
            $data[1] = trim(substr($line, $pos + 1));
            
            if ($data[0] eq "") {
                print "[load_conf] conf item [$conf_file] [$line] is NULL.";

                next;
            }

            if ($data[1] eq "") {
                $$config_item{$data[0]} = "";
            } else {
                $$config_item{$data[0]} = $data[1];
            }

            #print "[load_conf] name [$data[0]] value [$data[1]].\n";
        }
        #
        close($fh);

        return 0;
    }
}

sub fopen {
    my ($file, $mode) = @_;
    my $fh;

    my $err_msg;
    if (!defined($file) || $file eq "")
    {
        return -1;
    }       

    if (!defined($mode) || $mode eq "")
    {       
        $mode = DEFAULT_FILE_OPEN_MODE;
    }       

    if ($mode ne "r" && $mode ne "r+" 
            && $mode ne "w" && $mode ne "w+" 
            && $mode ne "a" && $mode ne "a+" ) {

        print "[fopen] open file [$file] ERROR: [$mode] is invalid";

        return -1;
    }       

    $fh = new FileHandle "$file", "$mode";
    if (defined $fh) {

        return $fh;
    } else {
        print "[fopen] open file [$file] [$mode] ERROR: [$@]";

        return -1;
    }
}

#@brief 用于将文件夹展成文件列表
#@param $dir_path 文件夹路径
#@param $files 文件列表数组的引用
#@param $tag 是否统计空文件夹,1为是，0为否
sub print_filelist_of_dir {
    my ($dir_path, $tag)  = @_;
    my @dirs = ( $dir_path . '/' );

    my ($dir, $file);
    print "Start to list the files\n";
    while ($dir = pop(@dirs)) {
        local *DH;
        if (!opendir (DH, $dir)) {
            print "[Fail][无法打开文件夹 $dir: $!] \n";
            next;
        }
        foreach (readdir(DH)) {
            if ($_ eq '.' || $_ eq '..') {
                next;
            }
            $file = $dir . $_;
            if (!-l $file && -d _) {
                $file .= '/';
                push (@dirs, $file);
                if (defined $tag && $tag) {
                    print "$file\n";
                }
            }else {
                print "$file\n";
            }
        }
        closedir(DH);
    }
    print "End of list the files\n";
}

sub unblock_machine{
    my ($is_block_alarm, $conf) = @_;
    if($is_block_alarm ne "TRUE"){
        print_log("[Notice][No need to unblock alarm][BLOCK_ALARM conf is $is_block_alarm]", __FILE__, __LINE__);
        return 0;
    }
    my $host_name = "";
    if(get_local_host_name(\$host_name) != 0){
        return -1;
    }
    my $i = 0;
    my $moduleccess = 0;
    my $cmd;
    my $cmd_output;
    for(; $i<3; $i++){
        my $ifGot = 0;
        if (exists($conf->{'BLOCK_LIST'}) && trim($conf->{'BLOCK_LIST'}) ne "") {
            $cmd = "$BLOCK_TOOL_PATH recover -h $host_name -rid ".$conf->{'BLOCK_LIST'} ;
        }elsif (exists($conf->{'BLOCK_LIST'}) && trim($conf->{'BLOCK_LIST'}) eq "") {
            print_log("[Ok][No rules need to be unblocked]", __FILE__, __LINE__);
            return 0;
        }else {
            print_log("[Fail][$cmd][UNBLOCK_RULES got error]", __FILE__, __LINE__);
            return -1;
        }
        $cmd_output = `$cmd`;
        if ("OK!" eq trim($cmd_output)) {
            $moduleccess = 1;
            print_log("[Ok][$cmd][Block tools show unblock ok:$cmd_output]", __FILE__, __LINE__);
            last;
        }else {
            print_log("[Notice][$cmd][Block tools show unblock fail:$cmd_output]", __FILE__, __LINE__);
            sleep 3;
            next;
        }
    }
    if($moduleccess == 0){
        print_log("[Fail][unblock Fail]", __FILE__, __LINE__);
        return -1;
    }else{
        print_log("[Ok][unblock ok]", __FILE__, __LINE__);
        return 0;
    }
}

# 从一个文件夹地址里，获取其中的任意一个.tar.gz或.tar.bz2的文件路径及其类型
sub get_compressed_file_from_dir
{
    my ($dir, $file_list) = @_;
    if (not -e $dir) {
        print_error("[Fail][main]","$dir is not exists", __FILE__, __LINE__);
        return -1;
    }
    for my $file (glob($dir."/*")) {
        my %tmp_file = ();
        if ($file =~ m/(.*?)\.tar\.gz$/ ) {
            push(@$file_list, $file);
        }
    }
    return 0;
}

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
    $machine =~ s/(:)+$//;
    return $machine.":".$path;
}
sub realurl
{
    my ($url) = @_;

    my ($scheme, $machine, $path);
    if ($url =~ /^((ftp|http):\/\/)([^\/]+)(.*)/) {
    $scheme = $1;
    $machine = $3;
    $path = $4;
    }
    else {
    return undef;
    }

    $path = format_path($path);
    if (!$path) {
    return undef;
    }
    return $scheme.$machine.$path;
}

sub format_path 
{
    my ($path) = @_;

    my $scheme = "";
    my $realpath = "";
    my @part;
    my @newpart;

    $path =~ s/(^\s*)|(\s*$)//g; #trim
    if ($path !~ /^\//) {
    return undef;
    }
    
    @part = split('/', $path);
    foreach my $p (@part) {
    if ($p eq "..") {
        pop(@newpart);
        next;
    }
    if ($p eq ".") {
        next;
    }
    if (not defined($p) or ($p eq "")) {
        next;
    }
    push @newpart, $p;
    }
    $realpath = join("/", @newpart);
    if ($scheme) {
    return $scheme.$realpath;
    }
    return "/".$realpath;
}
$| = 1;
