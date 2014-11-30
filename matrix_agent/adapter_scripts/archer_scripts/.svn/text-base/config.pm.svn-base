#!/usr/bin/perl -w
package config;
BEGIN{
   use Exporter();
   use vars qw($VERSION @ISA @EXPORT);
   @ISA=qw(Exporter);
   @EXPORT=qw(get_basic_config);
}

my %self_config =(
       'CN_ZONE_DOMAIN' => 'ftp://cn.scm.noah.baidu.com:/home/work/data/archer/',
       'HK_ZONE_DOMAIN' => 'ftp://hk.scm.noah.baidu.com:/home/work/data/archer/',
       'JP_ZONE_DOMAIN' => 'ftp://jp.scm.noah.baidu.com:/home/work/data/archer/',
       'UPLOAD_CN_ZONE_DOMAIN' => 'ftp://upload.cn.scm.noah.baidu.com:/home/work/data/archer/',
       'SERVER_URL' => 'http://api.noah.baidu.com/ci-web/index.php?r=',
       'REQUEST_URL_SUFFIX' => 'api/deploy/getDeployDiff',
       'LIMITE_RATE' => '3m',
       'SCM_BASIC_PATH' => 'ftp://getprod:getprod@getprod.scm.baidu.com/data/prod-aos/',
       'SCM_NOT_AOS_PATH' => 'getprod@product.scm.baidu.com:/data/',
       'BLOCK_HOST_URL' => 'http://api.noah.baidu.com/noah/index.php?r=machine/stopMonitorByDeploy',
       'UNBLOCK_HOST_URL' => 'http://api.noah.baidu.com/noah/index.php?r=machine/startMonitorByDeploy',
        );
sub get_basic_config{
    return \%self_config;
}

