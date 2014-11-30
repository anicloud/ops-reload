#!/usr/bin/perl -w

package derive_conf;
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
    @EXPORT=qw(handle_conf);
}

use strict;

use Getopt::Long;
use FileHandle;
use File::Basename;
use File::Glob ':globally';
use Data::Dumper;

use lib_aos_utility;
use file_operations;

sub handle_conf {
    my $conf = shift;
    my $groupName = "DEFAULT";
    if (exists($conf->{'GROUP_NAME'}) && !($conf->{'GROUP_NAME'} eq "") ) {
        $groupName = $conf->{'GROUP_NAME'};
    }
    if (not -e "./src/noahdes/conf_template.value") {
        print_notice("[Notice][DERIVE_CONF]","no noahdes/conf_template.value so will not handle it!!!", __FILE__, __LINE__);
        return 0;
    }

    my $matchCount = 0;
    my @matchedTag = ("DEFAULT");

    my @tag = split(',', $groupName);
    open FH, './src/noahdes/conf_template.value';

    while (<FH>){
        if (my ($group) = m/^\[(.*)\][\s]*$/){
            my @tags = split(',', $group);
            my $matched = 1;
            for my $e (@tags){
                $matched &= in_array($e, @tag);
            }
            if ($matched){
                print_notice("[Notice][DERIVE_CONF]","tag $groupName matched value:$group", __FILE__, __LINE__);
                if (!in_array($group, @matchedTag)) {
                    $matchCount += $matched;
                    push(@matchedTag, $group);
                }
            }
        }
    }
    close FH;

    if ($matchCount <= 1){
        $groupName = pop(@matchedTag);
    }
    elsif ($matchCount > 1){
        print_error("[Fail][DERIVE_CONF]","handle_conf Failed because:too many group matching the tag", __FILE__, __LINE__);
        print STDOUT "[Fail]Error Code: NDEC-995002 \n";
        return -1;
    }

    my $cmd = "cd cg && perl ./noah_cg -g '$groupName'";
    if (exists($conf->{'EXTENSION_KEY'}) && !($conf->{'EXTENSION_KEY'} eq "") ) {
        my $keyword = $conf->{'EXTENSION_KEY'};
        $cmd .= " -k '$keyword'";
    }
    print "CMD:$cmd\n";
    my $ret = system("$cmd 2>&1");
    if ($ret != 0) {
        print_error("[Fail][DERIVE_CONF]","handle_conf Failed because:$!", __FILE__, __LINE__);
        print STDOUT "[Fail]Error Code: NDEC-995002 \n";
        return -1;
    }
}

sub in_array{
    my ($e, @a) = @_;
    return (grep {$_ eq $e} @a);
}
