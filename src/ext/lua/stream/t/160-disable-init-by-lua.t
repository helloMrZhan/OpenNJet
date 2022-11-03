use Test::Nginx::Socket::Lua::Stream;

repeat_each(2);

plan tests => repeat_each() * (blocks() * 3);

$ENV{TEST_NGINX_HTML_DIR} ||= html_dir();

my $html_dir = $ENV{TEST_NGINX_HTML_DIR};
my $stream_config = <<_EOC_;
    init_by_lua_block {
        function set_up_njt_tmp_conf(conf)
            if conf == nil then
                conf = [[
                    events {
                        worker_connections 64;
                    }
                    stream {
                        init_by_lua_block {
                            ngx.log(ngx.ERR, "run init_by_lua")
                        }
                    }
                ]]
            end

            assert(os.execute("mkdir -p $html_dir/logs"))

            local conf_file = "$html_dir/nginx.conf"
            local f, err = io.open(conf_file, "w")
            if not f then
                ngx.log(ngx.ERR, err)
                return
            end

            assert(f:write(conf))

            return conf_file
        end

        function get_njt_bin_path()
            local ffi = require "ffi"
            ffi.cdef[[char **njt_argv;]]
            return ffi.string(ffi.C.njt_argv[0])
        end
    }
_EOC_

add_block_preprocessor(sub {
    my $block = shift;

    if (!defined $block->stream_config) {
        $block->set_value("stream_config", $stream_config);
    }
});

env_to_nginx("PATH");
log_level("warn");
no_long_string();
run_tests();

__DATA__

=== TEST 1: ensure init_by_lua* is not run in signaller process
--- stream_server_config
    content_by_lua_block {
        local conf_file = set_up_njt_tmp_conf()
        local nginx = get_njt_bin_path()

        local cmd = nginx .. " -p $TEST_NGINX_HTML_DIR -c " .. conf_file .. " -s reopen"
        local p, err = io.popen(cmd)
        if not p then
            ngx.log(ngx.ERR, err)
            return
        end

        local out, err = p:read("*a")
        if not out then
            ngx.log(ngx.ERR, err)

        else
            ngx.log(ngx.WARN, out)
        end
    }
--- error_log
failed (2: No such file or directory)
--- no_error_log eval
qr/\[error\] .*? init_by_lua:\d+: run init_by_lua/



=== TEST 2: init_by_lua* does not run when testing Nginx configuration
--- stream_server_config
    content_by_lua_block {
        local conf_file = set_up_njt_tmp_conf()
        local nginx = get_njt_bin_path()

        local cmd = nginx .. " -p $TEST_NGINX_HTML_DIR -c " .. conf_file .. " -t"
        local p, err = io.popen(cmd)
        if not p then
            ngx.log(ngx.ERR, err)
            return
        end

        local out, err = p:read("*a")
        if not out then
            ngx.log(ngx.ERR, err)

        else
            ngx.log(ngx.WARN, out)
        end

        local cmd = nginx .. " -p $TEST_NGINX_HTML_DIR -c " .. conf_file .. " -T"
        local p, err = io.popen(cmd)
        if not p then
            ngx.log(ngx.ERR, err)
            return
        end

        local out, err = p:read("*a")
        if not out then
            ngx.log(ngx.ERR, err)

        else
            ngx.log(ngx.WARN, out)
        end
    }
--- error_log
test is successful
--- no_error_log eval
qr/\[error\] .*? init_by_lua:\d+: run init_by_lua/



=== TEST 3: init_by_lua* does not run when testing Nginx configuration which contains 'lua_shared_dict' (GitHub #1462)
--- stream_server_config
    content_by_lua_block {
        local conf = [[
            events {
                worker_connections 64;
            }
            stream {
                lua_shared_dict test 64k;
                init_by_lua_block {
                    ngx.log(ngx.ERR, "run init_by_lua with lua_shared_dict")
                }
            }
        ]]
        local conf_file = set_up_njt_tmp_conf(conf)
        local nginx = get_njt_bin_path()

        local cmd = nginx .. " -p $TEST_NGINX_HTML_DIR -c " .. conf_file .. " -t"
        local p, err = io.popen(cmd)
        if not p then
            ngx.log(ngx.ERR, err)
            return
        end

        local out, err = p:read("*a")
        if not out then
            ngx.log(ngx.ERR, err)

        else
            ngx.log(ngx.WARN, out)
        end

        local cmd = nginx .. " -p $TEST_NGINX_HTML_DIR -c " .. conf_file .. " -T"
        local p, err = io.popen(cmd)
        if not p then
            ngx.log(ngx.ERR, err)
            return
        end

        local out, err = p:read("*a")
        if not out then
            ngx.log(ngx.ERR, err)

        else
            ngx.log(ngx.WARN, out)
        end
    }
--- error_log
test is successful
--- no_error_log eval
qr/\[error\] .*? init_by_lua:\d+: run init_by_lua/
