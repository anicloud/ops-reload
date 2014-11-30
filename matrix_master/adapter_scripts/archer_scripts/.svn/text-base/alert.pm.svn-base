#!/usr/bin/perl -w

package alert;
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
    @EXPORT=qw(fail_alert);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;

sub fail_alert {
    my ($email_alertlist, $tele_alertlist, $listid) = @_;
    my $ret = 0;

    #发送报警短信
    $ret = message_alert($tele_alertlist, $listid);
    if ($ret != 0) {
        if ($ret == 1) {
            print_error("[NOTICE][alert]","报警短信未发送：报警人电话号码名单为空message alert list is null", __FILE__, __LINE__);
        }else {
            print_error("[Fail][alert]发送报警短信失败can not successfully send alert message", __FILE__, __LINE__);
        }
    }else {
        print_error("[NOTICE][alert]","已向接受报警人员发送短信，请查收",__FILE__,__LINE__);
    }

    #发送报警邮件
    $ret = email_alert($email_alertlist, $listid);
    if ($ret != 0) {
        if ($ret == 1) {
            print_error("[NOTICE][alert]","报警邮件未发送：报警人邮箱名单为空email alert list is null", __FILE__, __LINE__);
        }else {
            print_error("[Fail][alert]","发送报警邮件失败can not successfully send alert email", __FILE__, __LINE__);
        }
    }else {
        print_error("[NOTICE][alert]","已向接受报警人员发送邮件，请查收",__FILE__,__LINE__);
    }
}

sub get_alert_name {
    my ($alertlist) = @_;

    if (!defined($alertlist) || trim($alertlist) eq "") {
        return -1;
    }
    my @lines = ( split ",", $alertlist );
    
    return @lines;
}

sub email_alert {
    my ($alertlist, $listid) = @_;
    my @email_line = get_alert_name($alertlist);

    if ($email_line[0] == -1) {
        print_error("[NOTICE][alert]","email alert list is null", __FILE__, __LINE__);
        return 1;
    }

    my $msgContent = "一键上线部署失败\nlistid=".$listid;
    my $cmd;
    my $ret = 0;
    my $host_name;
    my $success_num = 0;

    $ret = get_local_host_name(\$host_name);
    if ($ret != 0) {
        print_error("[Fail][alert]","can not successfully get local host name", __FILE__, __LINE__);
        return -1;
    }
    $msgContent .= ",\nhostname=".$host_name."\n"; 

    my $mail_file_name = "alertmail.txt.$$";
    open(FILE, ">$mail_file_name");
    syswrite(FILE, $msgContent);
    close(FILE);
    
    foreach my $one(@email_line) {
        if (trim($one) ne '') {
            $cmd = "mail -s \"[archer]一键上线部署失败\" ".$one." < ".$mail_file_name;
            $ret = system("$cmd 2>&1");
            if ($ret != 0) {
                print_error("[Fail][alert]","$cmd: can not successfully send email to $one", __FILE__, __LINE__);
            }else {
                $success_num += 1;
            }
        }
    }
    system("rm $mail_file_name");

    my $size = @email_line;
    if ($success_num == $size) {
        return 0;
    }elsif ($success_num == 0) {
        return -1;
    }else {
        print_notice("[Fail][alert]","部分报警人员未成功发送邮件can not successfully send email to all", __FILE__, __LINE__);
        return 0;
    }

}

sub message_alert {
    my ($alertlist, $listid) = @_;
    my @tele_line = get_alert_name($alertlist);
    if ($tele_line[0] == -1) {
        print_error("[NOTICE][alert]","message alert list is null", __FILE__, __LINE__);
        return 1;
    }

    my $msgContent = "一键上线部署失败listid=".$listid;
    my $cmd;
    my $ret = 0;
    my $host_name;
    my $success_num = 0;

    $ret = get_local_host_name(\$host_name);
    if ($ret != 0) {
        print_error("[Fail][alert]","can not successfully get local host name", __FILE__, __LINE__);
        return -1;
    }
    $msgContent .= ",hostname=".$host_name; 

    foreach my $one(@tele_line) {
        if (trim($one) ne '') {
            $cmd = "gsmsend -s emp01.baidu.com:15002 ".$one."@".$msgContent;
            $ret = system("$cmd 2>&1");
            if ($ret != 0) {
                print_error("[Fail][alert]","$cmd: can not successfully send message to $one", __FILE__, __LINE__);
            }else {
                $success_num += 1;
            }
        }
    }

    my $size = @tele_line;
    if ($success_num == $size) {
        return 0;
    }elsif ($success_num == 0) {
        return -1;
    }else {
        print_notice("[NOTICE][alert]","部分报警人员未成功发送短信",__FILE__,__LINE__);
        return 0;
    }

}
