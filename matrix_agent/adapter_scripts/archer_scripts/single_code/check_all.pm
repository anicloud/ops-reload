#!/usr/bin/perl -w

package check_all;
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
    @EXPORT=qw(backup_and_deploy_all check_all);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;

sub check_brackets_description {
    my ($file_path) = @_;
    
    my $ret = 0;
    my $line = "";
    my $startLineTag = 0;
    my $contentLineTag = 0;
    my $endLineTag = 0;
    my $lineNo = 0;
    if (not -e $file_path) {
        print_error("[Fail][main]","no file:$file_path",__FILE__,__LINE__);
        $ret = -1;
    }elsif (open (FH, $file_path)) {
        while($line = <FH>) {
            $line = trim($line);
            $lineNo++;
            if ($line eq "") {
                next;
            } 
            if ($line =~ m/.*=\s*\{/ ) {
                if ($endLineTag != $startLineTag) {
                    print_notice("[Notice][main]","line[$lineNo] a single } without { before",__FILE__,__LINE__);
                    last;
                }
                $startLineTag++;
            }elsif ($line =~ m/.*=.*/) {
                if ($startLineTag != $endLineTag + 1) {
                    print_notice("[Notice][main]","line[$lineNo] is content but without the start line xx/xx/={\n",__FILE__,__LINE__);
                    last;
                }
                $contentLineTag ++;
            }elsif ($line =~ m/.*=.*\}/ || $line eq "}") {
                $endLineTag++;
                if ($startLineTag != $endLineTag) {
                    print_error("[Fail][main]","line[$lineNo] no or more than one { before",__FILE__,__LINE__);
                    last;
                } 
                if ($contentLineTag <= 0) {
                    print_error("[Fail][main]","line[$lineNo] no content in this {}",__FILE__,__LINE__);
                    last;
                }
                $contentLineTag = 0;
            }
        }
    }else {
        print_error("[Fail][main]","cannot open file:$file_path",__FILE__,__LINE__);
        $ret = -1;
    }
    
    return $ret;
}

sub check_group_exists {
    my ($groupName, $templateFileName) = @_;
    my $line = "";
    my $ifGroupExists = -1;
    my $crossMultiLines = 0;
    my $lineNo = 0;
    
    if (open(FH, $templateFileName)) {
        while ($line = <FH>) {
            $line = trim($line);
            if($line =~ m/^[#]/){
                next;
            }
            $lineNo ++;
            if ($line =~ /\[(.*)\]/) {
                if ($groupName eq $1) {
                    if ($ifGroupExists == 0) {
                        print_error("[Fail][main]"," line[$lineNo]:group $groupName exists more than one time",__FILE__,__LINE__);
                    }
                    $ifGroupExists = 0;
                    #print "line[$lineNo] is your group name:$1\n";
                }
            }elsif ($line =~ /(.*)=(.*)/) {
                #print "line[$lineNo] is [key]:$1 = [value]:$2\n";
            }elsif ($line eq "") {
                #print "line[$lineNo] is empty line\n";
            }else {
                print_error("[Notice][main]","line[$lineNo]:$line is illegal!!!",__FILE__,__LINE__);
            }
        }
        close FH;
    }
    
    if ($ifGroupExists != 0) {
        print_error("[Warning][main]","group:$groupName not exists in conf_template:$!\n", __FILE__, __LINE__);
        print STDOUT "[Warning]Error Code :NDEC-995001\n";
        return 0;
    }
}

sub check_all {
    my ($conf) = @_;
    my $dataDesPath = "src/noahdes/data.des";
    my $binDesPath = "src/noahdes/bin.des";
    my $confValuePath = "src/noahdes/conf_template.value";
    my $groupName;
    if (exists($conf->{'GROUP_NAME'}) && !($conf->{'GROUP_NAME'} eq "") ) {
        $groupName = $conf->{'GROUP_NAME'};
    }else {
        print_error("[Fail][main]","No group name specified", __FILE__, __LINE__);
        return -1;
    }
    my $ret = 0;
    print_start("conf_template.value analsy start");
    if (not -e $confValuePath) {
        print_notice("[Notice][main]","No conf value:$confValuePath", __FILE__, __LINE__);
    }elsif (0 != check_group_exists($groupName, $confValuePath)) {
        $ret = -1;
    }
    print_start("conf_template.value analsy end");
    print_start("start to check $dataDesPath");
    if (not -e $dataDesPath) {
        print_notice("[Notice][main]","No data des:$dataDesPath", __FILE__, __LINE__);
    }elsif (0 != check_brackets_description($dataDesPath)) {
        $ret = -1;
    }
    print_start("end of check $dataDesPath");
    print_start("start to check $binDesPath");
    if (not -e $binDesPath) {
        print_notice("[Notice][main]","No bin des:$binDesPath", __FILE__, __LINE__);
    }elsif (0 != check_brackets_description($binDesPath)) {
        $ret = -1;
    }
    print_start("end of check $binDesPath");
    return $ret;
}
$| = 1;
