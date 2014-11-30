#!/usr/bin/perl -w
package check_file;
my $g_tmp_basedir;
BEGIN{
    use Cwd 'realpath';
    $g_tmp_basedir =__FILE__;
    $g_tmp_basedir = realpath($g_tmp_basedir);
    $g_tmp_basedir =~ s/[^\/]+$//;
    
    use Exporter();
    use vars qw($VERSION @ISA @EXPORT);

    @ISA=qw(Exporter);
    @EXPORT=qw(check_scp_file check_ftp_file);
}
use Net::FTP;
use strict;

sub check_scp_file{
    my ($passwd,$remote) = @_;
    my ($host,$dir)= split(":",$remote);

    my @files =`$g_tmp_basedir/sshpass -p $passwd ssh -oStrictHostKeyChecking=no $host ls $dir 2>/dev/null`;

    if($? ne 0) {
        print_error("[Fail]","$host or $dir not correct!",__FILE__,__LINE__);
        return  -1;
    }
    else{
        if($#files eq  0){
            return 0;
        }
        else{
            return 1;
        }
    }
}

sub check_ftp_file{
    $_ = shift;
    defined($_) or die "perl check_ftp_file ftp://xxx/path\n";
    
    s|/+|/|g;  
    m|ftp:/([^:/]*):?(/.*)|;

    my $ftp = Net::FTP->new($1, Timeout => 30) or die "Could not connect\n";
    $ftp->login('ftp', 'ftp') or die "Could not login\n";
    my @files = $ftp->ls($2);
    my $file = $files[0]; 
    if ($#files == 0 && $file le $2){
        return 0;
    }
    elsif($#files > 0){
        return 1;
    }
    else{
        return -1;
    }
}

