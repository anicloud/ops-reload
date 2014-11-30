#!/usr/bin/perl -w

package execute_work_flow;
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
    @EXPORT=qw(check_conf load_operations execute_operations init_operations);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;
use lib::Tiny;
use lib_aos_utility;
use file_operations;

sub load_operations{
    my($filename,$config_item) = @_;
    if(not -e $filename){
        return -1;
    }
    my $yaml = lib::Tiny -> new ;
    $yaml =lib::Tiny -> read($filename);
    if(!defined($yaml)){
        print_error("[Fail]Yaml parse failed!Error Line is: $YAML::Tiny::errstr!\n");
        print STDOUT "[Fail]Error Code:NDEC-091001\n";
        return -1;
    }

    $$config_item{'SOURCE_OPERATIONS'} = $yaml->[0]->{SOURCE};
    $$config_item{'BIN_OPERATIONS'} = $yaml->[0]->{BIN};
    $$config_item{'DATA_OPERATIONS'} = $yaml->[0]->{DATA};
    return 0;
}
sub check_conf {
    my ($config_ref) = @_;

    my $err_msg = "";

    #DYNAMIC CONFIG
    if ( not defined( $$config_ref{"GROUP_NAME"} ) ) {
        print_error("[Fail][main]","argument [GROUP_NAME] is invalid",__FILE__,__LINE__);
        return -1;
    }

    if ( not defined( $$config_ref{"TOP_DIR"} ) ) {
        print_error("[Fail][main]","argument [TOP_DIR] is invalid.",__FILE__,__LINE__);
        return -1;
    }
    
    return 0;
}

sub init_operations {
    my $conf = shift;
    my @prepare_operations;
    my @main_operations;
    my @end_operations;
    my $operation_file;
    my $operation_file_dir = 'src/noahdes/';
    
    my $operation_file_tail_default = 'operation_list';
    my $operation_file_tail_yaml = 'operation_list.yaml';

    my $operation_file_tail = $operation_file_tail_default;
    my $operation_file_yaml = $operation_file_tail_yaml;
    
    my $is_yaml = 0;
    if (exists($conf->{'ACTION'})) {
        $operation_file_tail .= "_".$conf->{'ACTION'};
    }
    if(exists($conf->{'YAML'})){
        $is_yaml = 1;
    }
    $operation_file = $operation_file_dir.$operation_file_tail;
    my $line;
    my $fh;
    if (not -e $operation_file) {
        print_error("[Waring][main]","operation_list:$operation_file doesn't exists and we will use default action", __FILE__, __LINE__);
        $operation_file = $operation_file_dir.$operation_file_tail_default;
    }
    if (not -e $operation_file) {
        print_error("[Waring][main]","operation_list:$operation_file doesn't exists and we will use default", __FILE__, __LINE__);
        if($is_yaml eq 1){
            $operation_file = $operation_file_yaml;   
        }
        else{
            $operation_file = $operation_file_tail_default;
        }
    }
    if (not -e $operation_file) {
        print_error("[Fail][main]","No kinds of operation_list exists", __FILE__, __LINE__);
        return -1;
    }
    if($is_yaml == 1){
        my $yaml =lib::Tiny -> new;
        $yaml =lib::Tiny -> read($operation_file);

        if(!defined($yaml)){
            print_error("[Fail][main]","YAML parse Failed!! Error Line is:$YAML::Tiny::errstr!",__FILE__,__LINE__);
            print STDOUT "[Fail]Error Code:NDEC-091001\n";
            return -1;
        }
        my @main = $yaml->[0]->{MAIN};
        my @prepare =$yaml->[0]->{PREPARE};
        my @end=$yaml->[0]->{'END'};

        my $item;
        my $sub_item;
        foreach $item(@prepare){
            foreach $sub_item(@$item){
                push(@prepare_operations,$sub_item);
            }
        }
        foreach $item(@main){
            foreach $sub_item(@$item){
                push(@main_operations,$sub_item);
            }
        }
        foreach $item(@end){
            foreach $sub_item(@$item){
                push(@end_operations,$sub_item);
            }
        }
    }
    else {
        my $tag = "";
        if (open($fh, $operation_file)) {
            while($line = <$fh>) {
                chomp($line);
                my $tmpline = trim($line);
                if ($tmpline eq "[PREPARE]" || $tmpline eq "[MAIN]" || $tmpline eq "[END]") {
                    $tag = $tmpline;
                 }
                if ($tag eq "[PREPARE]" && not ($tmpline eq $tag)) {
                    push(@prepare_operations, $tmpline);
                }elsif ($tag eq "[MAIN]" && not ($tmpline eq $tag)) {
                    push(@main_operations, $tmpline);
                }elsif ($tag eq "[END]" && not ($tmpline eq $tag)) {
                    push(@end_operations, $tmpline);
                }
            }
        }
    }
    my @operations;
    push(@operations, \@prepare_operations);
    push(@operations, \@main_operations);
    push(@operations, \@end_operations);
    $conf->{'operations'} = \@operations;
    return 0;
}

sub execute_operations {
    my ($conf) = @_;
    if (not exists($conf->{'operations_map'}) || not exists($conf->{'operations'})) {
        print_error("[Fail][main]","operations_map or operations not set", __FILE__, __LINE__);
        return -1;
    }
    my $operations_map = $conf->{'operations_map'};
    my $operations = $conf->{'operations'};
    my $prepare_tag = 1;
    my $total_ret = 0;
    my $i = 1;
    foreach my $now_operations (@$operations) {
        my $ret = 0;
        if ($i > 1 && $prepare_tag != 1) {
            print_error("[Fail][main]","Prepare Failed and we will do nothing then", __FILE__, __LINE__);
            last;
        }
        foreach my $operation (@$now_operations) {
            my $index = index $operation,":";
            my ($method, $param) ;
            if($index == -1){
                $method = $operation;
            }
            else{
                $method = substr $operation,0,$index;
                $param =  substr $operation,$index+1;
            }
            if (exists($operations_map->{$method})) {
                my $method_infact = $operations_map->{$method};
                print_start($method);
                print_log($method,"Start $method", __FILE__, __LINE__);
                $ret = $method_infact->($conf, $param);
            }else {
                print_error("[Fail][$method]","No operation exists as:$method", __FILE__, __LINE__);
                return -1;
            }
            if ($ret != 0) {
                print_error("[Fail][$method]","execute function:$method Failed,error code:$ret", __FILE__, __LINE__);
                $total_ret = $ret;
                if ($i == 1) {
                    $prepare_tag = 0;
                }
                last;
            }
            print_log($method,"End $method\n",__FILE__,__LINE__);
        }
        if ($i == 2 && $ret == 0) {
            print_notice("[Notice][main]","main_operations execute successfully, now start to sleep:$conf->{'MACHINE_INTERVAL'} (s)", __FILE__, __LINE__);
            sleep($conf->{'MACHINE_INTERVAL'});
        }
        $i ++;
    }
    return $total_ret;
}
