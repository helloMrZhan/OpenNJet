#!/bin/bash
#tgtdir=/Users/`whoami`/njet
tgtdir=/etc/njet
tgbindir=/usr/sbin/njet
tglogdir=/var/log/njet/error.log
modulesdir=/usr/lib/njet/modules
export LUAJIT_INC='/etc/njet/luajit/include/luajit-2.1'
export LUAJIT_LIB='/etc/njet/luajit/lib'
#--with-ld-opt='-Wl,-rpath,/usr/local/tassl/openssl/lib'
#--with-cc-opt=-I'auto/lib/tassl/include' --with-ld-opt='-Wl,-rpath,/usr/local/tassl/openssl/lib' 
flags=" --conf-path=/etc/njet/njet.conf  --with-openssl=auto/lib/tassl   --with-debug --with-stream --build=NJT1.0 --with-http_addition_module --with-http_auth_request_module --with-http_dav_module --with-http_flv_module --with-http_gunzip_module --with-http_gzip_static_module --with-http_mp4_module --with-http_random_index_module --with-http_realip_module --with-http_secure_link_module --with-http_slice_module --with-http_ssl_module --with-http_stub_status_module --with-http_sub_module --with-http_v2_module --with-mail --with-mail_ssl_module  --with-stream_realip_module --with-stream_ssl_module --with-stream_ssl_preread_module  --add-module=./modules/njet-stream-proto-module  --add-module=src/ext/lua/kit  --add-module=src/ext/lua/http --add-module=src/ext/lua/stream  --add-module=./modules/njet-stream-upstream-dynamic-servers-module  --add-module=./modules/njet-http-upstream-dynamic-servers-module --add-module=./modules/njet-http-match-module  --add-module=./modules/njet-http-health-check-module --add-module=./modules/njet-stream-health-check-module --add-module=./modules/njet-http-sticky-module  --add-module=./modules/njet-http-location  --with-cc=/usr/bin/cc --with-cc-opt=-O0     --prefix=$tgtdir --sbin-path=$tgbindir --modules-path=$modulesdir --error-log-path=$tglogdir --with-pcre=auto/lib/pcre-8.45" 
cdir=`cd $(dirname $0); pwd`
(
    cd $cdir
    set -e
    for option; do
        case $option in
            conf*)
		if [ ! -d /etc/njet/luajit ]; then
		   cd luajit;make;make install;cd -;
		fi
		if [ ! -d /etc/njet/lualib ]; then
		   cp -fr lualib /etc/njet/lualib
		fi
                #./configure $flags
		./configure --with-cc-opt=" -ggdb -fsanitize=address -fno-omit-frame-pointer -static-libgcc -static-libasan" \
			--with-ld-opt=" -fsanitize=address -static-libgcc -static-libasan" $flags
                ;;
            make)
                make
                ;;
            install)
                make install
                ;;
            clean)
                make clean
                ;;
            release)
                if [ -f ./objs/njet ]; then
                    make clean
                fi
                ./configure $flags
                 make;
                cp objs/njet objs/njet.debug
                objcopy --strip-unneeded ./objs/njet
                ;;
            modules)
                if [ -f ./objs/njet ]; then
                    make clean
                fi
                ./configure $module_flags
                 make modules;
                 ;;
            *)
                echo "$0 [conf[igure]|make|install|clean|release]"
                ;;
        esac
    done
    set +e
)