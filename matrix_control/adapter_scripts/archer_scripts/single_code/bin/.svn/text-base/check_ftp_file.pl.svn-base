#!/usr/bin/perl -w
$_ = shift;
defined($_) or die "perl check_ftp_file.pl ftp://xxx/path\n";
s|/+|/|g;
m|ftp:/([^:/]*):?(/.*)|;

use Net::FTP;
$ftp = Net::FTP->new($1, Timeout => 30) or die "Could not connect\n";
$ftp->login('ftp', 'ftp') or die "Could not login\n";
@files = $ftp->ls($2);
$file = $files[0];
if ($#files == 0 && $file le $2){
    exit 0;
}
exit 1;
