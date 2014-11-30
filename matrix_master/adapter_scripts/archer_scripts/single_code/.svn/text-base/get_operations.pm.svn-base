#! /usr/bin/perl -w
package get_operations;
my $g_cur_basedir;
BEGIN{
    use Cwd 'realpath';
    $g_cur_basedir = __FILE__;
    $g_cur_basedir = realpath($g_cur_basedir);
    $g_cur_basedir =~ s/[^\/]+$//;

    unshift(@INC,$g_cur_basedir);
    @ISA = qw(Exporter);
    @EXPORT = qw(init_operations get_source_operations get_bin_and_data_operations);
}

use strict;
use FileHandle;
use Data::Dumper;
use lib_aos_utility;
use file_operations;

use constant SOURCE_OPERATION => 'souce';
use constant COMPONENT_OPERATION => 'component';


#用于初始化操作序列数据结构
sub init_operations{
    my ($operations) = @_;
    $$operations{'ADD'} = ();
    $$operations{'DEL'} = ();
    $$operations{'UPDATE'} = ();
    $$operations{'SKIP'} = ();
    $$operations{'EMPTY'} = ();
}

#@brief用于获取部署中的空目录
#@param $file1  存放空目录路径的文件名
#@param $operations 用于存放操作序列
sub get_emptyDirectory{
    my ($file1,$operations) = @_;
    if(! -e $file1){
        print_notice("[Notice][main]","$file1 not exists!",__FILE__,__LINE__);
        return 0;
    }
    my $fh = new FileHandle $file1,"r";
    my $line ;
    my @empty = ();
    while($line = <$fh>){
        chomp($line);
        if($line eq ''){
            next;
        }
        push(@empty,$line);
    }
    close($fh);
    $$operations{'EMPTY'} = \@empty;
    return 0;
}

#获取本地文件和中转机文件中的不同部分
sub get_hash{
    my ($file1,$file2,$hash1,$hash2,$type) = @_;
    my $cmd = "comm -23 $file1 $file2 >onlyInFirst.ope";
    my $ret = system($cmd);
    if($ret ne 0){
        print_error("[Fail][main]","Execute comm -23 $file1 $file2 Failes!",__FILE__,__LINE__);
        return -1;
    }
    $cmd = "comm -23 $file2 $file1 >onlyInSecond.ope";
    if (system($cmd) ne 0){
        print_error("[Fail][main]","Execute comm -23 $file1 $file2 Failes!",__FILE__,__LINE__);
        return -1;
    }

    my $fh = new FileHandle "onlyInFirst.ope";
    my $line;
    while($line =<$fh>){
        chomp($line);
        if($line eq ''){
            next;
        }
        my ($md5,$file,$adm,$dest_path) = split(/\s+/,$line);
        if($type eq SOURCE_OPERATION){
            if(!defined($adm)){
                $adm = '0755';
            }
            $$hash1{$file}{'mode'} = $adm;
        }
        elsif($type eq COMPONENT_OPERATION){
            if(!defined($adm)){
                print_error("[Fail][main]","bin.md5 or data.md5 pattern not correct.Please contact with RDs",__FILE__,__LINE__);
                return -1;
            }
            $$hash1{$file}{'source'} = $adm;
            $$hash1{$file}{'destpath'} = $dest_path;
        }
        else{
            print_error("[Fail][main]","$type undefined! Please contact with RDs",__FILE__,__LINE__);
            return -1;
        }
        $$hash1{$file}{'file'} = $file;
        $$hash1{$file}{'md5'} = $md5;
    }
    close($fh);

    $ret = system("rm -rf onlyInFirst.ope");
    if($ret ne 0){
        print_error("[Fail][main]","rm onlyInFirst.ope Failed!!",__FILE__,__LINE__);
        return -1;
    }

    $fh = new FileHandle "onlyInSecond.ope";
    while($line = <$fh>){
        chomp($line);
        if($line eq ''){
            next;
        }
        my ($md5,$file,$adm,$dest_path) = split(/\s+/,$line);
        if($type eq SOURCE_OPERATION){
            if(!defined($adm)){
                $adm = '0755';
            }
            $$hash2{$file}{'mode'} = $adm;
        }
        elsif($type eq COMPONENT_OPERATION){
            if(!defined($adm)){
                print_error("[Fail][main]","bin.md5 or data.md5 pattern not correct! Please contact with RDs",__FILE__,__LINE__);
                return -1;
            }
            $$hash2{$file}{'source'} = $adm;
            $$hash2{$file}{'destpath'} = $dest_path;
        }
        else{
            print_error("[Fail][main]","$type undefined !! Please contact with RDs",__FILE__,__LINE__);
            return -1;
        }
        $$hash2{$file}{'file'} = $file;
        $$hash2{$file}{'md5'} = $md5;
    }
    close($fh);
    $ret = system("rm -rf onlyInSecond.ope");
    if($ret ne 0){
        print_error("[Fail][main]","rm file onlyInSecond.ope Failed!",__FILE__,__LINE__);
        return -1;
    }
    return 0;
}
#@brief 用于获取文件的操作序列，包括skip、add、update和del
#@param $file1 本地md5文件
#@param $file2 远程md5文件
#@param $operations 保存得到的操作序列
sub get_operations{
    my ($file1,$file2,$operations,$type) = @_;
    my @add;
    my @del;
    my @update;
    my @skip;

    if(! -e $file1){
    #md5_local不存在，则只有add操作序列
        my $fh = new FileHandle $file2,"r" ;
        if(not defined($fh)){
            print_error("[Fail][main]"," new FileHandle $file2 faield!",__FILE__,__LINE__);
            return -1;
        }
        my $line ;
        my $i = 0;
        while($line = <$fh>){
            chomp($line);
            $line =  trim($line);
            my %tmp = ();
            my($md5,$file,$adm,$dest_path) = split(/\s+/,$line);
            
            if($type eq SOURCE_OPERATION){
                if(!defined($adm)){
                    $adm = '0755';
                }
                $tmp{'mode'} = $adm;
            }
            elsif($type eq COMPONENT_OPERATION ){
                if(!defined($adm)){
                    print_error("[Fail][main]","bin.des or data.des pattern not correct! Please contanct with Rds",__FILE__,__LINE__);
                    return -1;
                }
                $tmp{'source'} = $adm;
                $tmp{'destpath'} = $dest_path;
            }
            $tmp{'file'} = $file;
            $tmp{'md5'} = $md5;
            $add[$i] = \%tmp;
            $i++;
        }
        close($fh);
        $$operations{'ADD'} = \@add;
        return 0;
    }
    my $ret = get_SkipOperations($file1,$file2,$operations,\@skip,$type);
    if($ret ne 0){
        print_error("[Fail][main]"," get_SkipOperations($file1,$file2,$operations,\@skip) Failed!",__FILE__,__LINE__);
        return -1;
    }

    my %hash1 =();
    my %hash2 = ();

    $ret = get_hash($file1,$file2,\%hash1,\%hash2,$type);
    if($ret ne 0){
        print_error("[Fail][main]"," get $file1 ,$file2 hashs Failed!",__FILE__,__LINE__);
        return -1;
    }
    foreach my $item(sort keys %hash1){
        my $value = $hash1{$item};
        if(defined($hash2{$item}) && $hash2{$item}{'md5'} ne $hash1{$item}{'md5'}){
            #说明文件存在，但是md5不同，则为update序列
            push(@update,$hash2{$item});
        }
        elsif(defined($hash2{$item}) && $hash2{$item}{'md5'} eq $hash1{$item}{'md5'}){
            push(@skip,$value);
        }
        else{
            #该文件只存在于文件1，因而为del序列
            push(@del,$value);
        }
    }

    foreach my $item(sort keys %hash2){
        my $value = $hash2{$item};
        if(defined($hash1{$item})){
            #说明文件在1、2中都存在，只是md5不同，则为update序列，因为前面已经判断，则不需要考虑
            next;
        }
        else{
            #该文件至存在于文件2，因而为add序列
            push(@add,$value);
        }
    }
    $$operations{'ADD'} = \@add;
    $$operations{'DEL'}  =\@del;
    $$operations{'UPDATE'} = \@update;
    $$operations{'SKIP'} = \@skip;

    return 0;
}

#获取skip的操作序列
sub get_SkipOperations{
    my ($file1,$file2,$operations,$skip,$type) = @_;
    my $cmd = "comm -12 $file1 $file2 >skip.ope";
    my $ret = system($cmd);
    if($ret ne 0){
        print_error("[Fail][main]","Execute comm -12 $file1 $file2 Failed!",__FILE__,__LINE__);
        return -1;
    }
    my $fh = new FileHandle "skip.ope" ,"r";
    my $line;
    my $i = 0;
    while($line = <$fh>){
        chomp($line);
        my %tmp = ();
        my ($md5,$file,$adm,$dest_path) = split(/\s+/,$line);
        if($type eq SOURCE_OPERATION){
            if(!defined($adm)){
                $adm = '0755';
            }   
            $tmp{'mode'} = $adm;
        }   
        elsif($type eq COMPONENT_OPERATION){
            if(!defined($adm)){
                print_error("[Fail][main]","bin.md5 or data.md5 pattern not correct.Please contact with RDs",__FILE__,__LINE__);
                return -1; 
            }   
            $tmp{'source'} = $adm;
            $tmp{'destpath'} = $dest_path;
        }   
        else{
            print_error("[Fail][main]","$type undefined! Please contact with RDs",__FILE__,__LINE__);
            return -1; 
        }   
        $tmp{'file'} = $file;
        $tmp{'md5'} = $md5;
        $$skip[$i] = \%tmp;
        $i++;
    }
    close($fh);

    $ret = system("rm -rf skip.ope");
    if($ret ne 0){
        return -1;
    }
    return 0;
}

#@brief  获取源包的操作序列
#@param md5_local 本地MD5文件
#@param md5_souce 远程MD5文件
#@param empty 空目录文件
sub get_source_operations{
    my ($md5_local,$md5_source,$empty,$operation) = @_;
    init_operations($operation);
    if(trim($empty) ne ""){
        my $ret = get_emptyDirectory($empty,$operation);
        if($ret ne 0){
            print_error("[Fail][main]","get_emptyDirectory($empty) Failed!",__FILE__,__LINE__);
            return -1;
        }
    }

    my $ret = get_operations($md5_local,$md5_source,$operation,SOURCE_OPERATION);
    if($ret ne 0){
        print_error("[Fail][main]","get_operations($md5_local,$md5_source) Failed!",__FILE__,__LINE__);
        return -1;
    }
    return 0;
}

#bin和data组件的操作序列
sub get_bin_and_data_operations{
    my ($md5_local,$md5_source,$operation) = @_;
    init_operations($operation);

    my $ret = get_operations($md5_local,$md5_source,$operation,COMPONENT_OPERATION);
    if($ret ne 0){
        print_error("[Fail][main]","get_operations($md5_local,$md5_source) Failed!",__FILE__,__LINE__);
        return -1;
    }
    return 0;
}
