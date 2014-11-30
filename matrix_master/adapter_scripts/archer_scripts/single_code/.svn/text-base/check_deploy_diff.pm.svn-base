#!/usr/bin/perl -w

package check_deploy_diff;
my $g_tmp_basedir;
BEGIN{
    use Cwd 'realpath';
    $g_tmp_basedir = __FILE__;
    $g_tmp_basedir = realpath($g_tmp_basedir);
    $g_tmp_basedir =~ s/[^\/]+$//;

    unshift(@INC,$g_tmp_basedir);
    use Exporter();
    use vars qw($VERSION @ISA @EXPORT);
    @ISA = qw(Exporter);
    @EXPORT = qw(check_deploy_diff);
}

use strict;
use File::stat;
use FileHandle;
use LWP::UserAgent;
use lib_aos_utility;

my $SERVER_URL = 'http://api.noah.baidu.com/ci-web/index.php?r=';
#my $SERVER_URL = 'http://tc-oped-dev02.tc.baidu.com:8899/ci-web/index.php?r=';
my $EXECUTE_DEPLOY_DIFF_URL=$SERVER_URL.'api/deploy/getDeployDiff';

sub check_deploy_diff{
    my ($conf,$listid,$deployPath) = @_;
    my $localHost;
    if(not defined($conf->{'SERVER_URL'}) || trim($conf->{'SERVER_URL'}) eq ''){
        print "[Fail]Can not find SERVER_URL in the config!";
        return -1;
    }
    if(not defined($conf->{'REQUEST_URL_SUFFIX'}) || trim($conf->{'REQUEST_URL_SUFFIX'}) eq ''){
        print "[Fail]Can not find REQUEST_URL_SUFFIX in the config!";
        return -1;
    }

    if(get_local_host_name(\$localHost) != 0){
        print "[Fail] get_local_host_name failed!";
        return -1;
    }
    my $EXECUTE_DEPLOY_DIFF_URL=$conf->{'SERVER_URL'}.$conf->{'REQUEST_URL_SUFFIX'};

    my $browser = LWP::UserAgent->new;
    my $response = $browser->post( $EXECUTE_DEPLOY_DIFF_URL,
                                ["listid"=> $listid,
                                 "hostname"=>$localHost,
                                ]);
    if(not $response->is_success){
        print $response->content;
        my $tempStr =  "Something is wrong with ci-web param get, browser ret status=".$response->status_line."\n";
        print $tempStr;
        return -1;
    }
    my $ret = $response->content;
    print $ret;
    if(defined($ret) && ($ret eq 'no records' || $ret eq 'no status' || $ret eq '')){
        print "can not find $listid in the database,or its path is empty\n";
        return -1;
    }
    else{
        $$deployPath = $ret;
        return 0;
    }
}
