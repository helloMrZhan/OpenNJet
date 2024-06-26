# vim:set ft= ts=4 sw=4 et fdm=marker:
use Test::Nginx::Socket::Lua;

#worker_connections(1014);
#master_process_enabled(1);
#log_level('warn');

repeat_each(2);
#repeat_each(1);

plan tests => repeat_each() * (blocks() * 2);

$ENV{TEST_NGINX_MEMCACHED_PORT} ||= 11211;

#no_diff();
#no_long_string();
run_tests();

__DATA__

=== TEST 1: DELETE
--- config
    location /other {
        default_type 'foo/bar';
        echo $echo_request_method;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/other",
                { method = njt.HTTP_DELETE });

            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
DELETE



=== TEST 2: DELETE (proxy method)
--- config
    location /other {
        default_type 'foo/bar';
        echo $echo_request_method;
    }

    location /foo {
        proxy_pass http://127.0.0.1:$server_port/other;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { method = njt.HTTP_DELETE });

            njt.print(res.body)
        ';

        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
DELETE



=== TEST 3: POST (nobody, proxy method)
--- config
    location /other {
        default_type 'foo/bar';
        echo $echo_request_method;
    }

    location /foo {
        proxy_pass http://127.0.0.1:$server_port/other;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { method = njt.HTTP_POST });

            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
POST



=== TEST 4: HEAD
--- config
    location /other {
        default_type 'foo/bar';
        echo $echo_request_method;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/other",
                { method = njt.HTTP_HEAD });

            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body



=== TEST 5: explicit GET
--- config
    location /other {
        default_type 'foo/bar';
        echo $echo_request_method;
    }

    location /foo {
        proxy_pass http://127.0.0.1:$server_port/other;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { method = njt.HTTP_GET });

            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
GET



=== TEST 6: implicit GET
--- config
    location /other {
        default_type 'foo/bar';
        echo $echo_request_method;
    }

    location /foo {
        proxy_pass http://127.0.0.1:$server_port/other;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo")

            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
GET



=== TEST 7: implicit GET (empty option table)
--- config
    location /other {
        default_type 'foo/bar';
        echo $echo_request_method;
    }

    location /foo {
        proxy_pass http://127.0.0.1:$server_port/other;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo", {})

            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
GET



=== TEST 8: PUT (nobody, proxy method)
--- config
    location /other {
        default_type 'foo/bar';
        echo_read_request_body;

        echo $echo_request_method;
        echo_request_body;
    }

    location /foo {
        proxy_pass http://127.0.0.1:$server_port/other;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { method = njt.HTTP_PUT, body = "hello" });

            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body chomp
PUT
hello



=== TEST 9: PUT (nobody, no proxy method)
--- config
    location /other {
        default_type 'foo/bar';
        #echo_read_request_body;

        echo $echo_request_method;
        #echo $echo_request_body;
        echo_request_body;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/other",
                { method = njt.HTTP_PUT, body = "hello" });

            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body chomp
PUT
hello



=== TEST 10: PUT (nobody, no proxy method)
--- config
    location /other {
        default_type 'foo/bar';
        #echo_read_request_body;

        echo $echo_request_method;
        #echo $echo_request_body;
        echo_request_body;
        #echo "[$http_content_length]";
        echo;
    }

    location /foo {
        echo $echo_request_method;
        echo -n "[$http_content_length]";
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/other",
                { method = njt.HTTP_PUT, body = "hello" });

            njt.print(res.body)

            res = njt.location.capture("/foo")
            njt.say(res.body)

        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
PUT
hello
GET
[]



=== TEST 11: POST (with body, proxy method)
--- config
    location /other {
        default_type 'foo/bar';
        echo_read_request_body;

        echo $echo_request_method;
        echo_request_body;
    }

    location /foo {
        proxy_pass http://127.0.0.1:$server_port/other;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { method = njt.HTTP_POST, body = "hello" });

            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body chomp
POST
hello



=== TEST 12: POST (with body, memc method)
--- config
    location /flush {
        set $memc_cmd flush_all;
        memc_pass 127.0.0.1:$TEST_NGINX_MEMCACHED_PORT;
    }

    location /memc {
        set $memc_key $echo_request_uri;
        set $memc_exptime 600;
        memc_pass 127.0.0.1:$TEST_NGINX_MEMCACHED_PORT;
    }

    location /lua {
        access_by_lua '
            njt.location.capture("/flush");

            local res = njt.location.capture("/memc");
            njt.say("GET: " .. res.status);

            res = njt.location.capture("/memc",
                { method = njt.HTTP_PUT, body = "hello" });
            njt.say("PUT: " .. res.status);

            res = njt.location.capture("/memc");
            njt.say("cached: " .. res.body);

        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
GET: 404
PUT: 201
cached: hello



=== TEST 13: POST (with body, memc method)
--- config
    location /flush {
        set $memc_cmd flush_all;
        memc_pass 127.0.0.1:$TEST_NGINX_MEMCACHED_PORT;
    }

    location /memc {
        set $memc_cmd "";
        set $memc_key $echo_request_uri;
        set $memc_exptime 600;
        memc_pass 127.0.0.1:$TEST_NGINX_MEMCACHED_PORT;
    }

    location /lua {
        access_by_lua '
            njt.location.capture("/flush",
                { share_all_vars = true });

            local res = njt.location.capture("/memc",
                { share_all_vars = true });
            njt.say("GET: " .. res.status);

            res = njt.location.capture("/memc",
                { method = njt.HTTP_PUT, body = "hello", share_all_vars = true });
            njt.say("PUT: " .. res.status);

            res = njt.location.capture("/memc", { share_all_vars = true });
            njt.say("cached: " .. res.body);

        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
GET: 404
PUT: 201
cached: hello



=== TEST 14: empty args option table
--- config
    location /foo {
        echo $query_string;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { args = {} })
            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body eval: "\n"



=== TEST 15: non-empty args option table (1 pair)
--- config
    location /foo {
        echo $query_string;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { args = { ["fo="] = "=>" } })
            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
fo%3D=%3D%3E



=== TEST 16: non-empty args option table (2 pairs)
--- config
    location /foo {
        echo $query_string;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { args = { ["fo="] = "=>",
                    ["="] = ":" } })
            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body_like chop
^(?:fo%3D=%3D%3E\&%3D=%3A|%3D=%3A\&fo%3D=%3D%3E)$



=== TEST 17: non-empty args option table (2 pairs, no special chars)
--- config
    location /foo {
        echo $query_string;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { args = { foo = 3,
                    bar = "hello" } })
            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body_like chop
^(?:bar=hello\&foo=3|foo=3\&bar=hello)$



=== TEST 18: non-empty args option table (number key)
--- config
    location /foo {
        echo $query_string;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { args = { [57] = "hi" } })
            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body_like: 500 Internal Server Error
--- error_code: 500



=== TEST 19: non-empty args option table (plain arrays)
--- config
    location /foo {
        echo $query_string;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo",
                { args = { "hi" } })
            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body_like: 500 Internal Server Error
--- error_code: 500



=== TEST 20: more args
--- config
    location /foo {
        echo $query_string;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo?a=3",
                { args = { b = 4 } })
            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
a=3&b=4



=== TEST 21: more args
--- config
    location /foo {
        echo $query_string;
    }

    location /lua {
        access_by_lua '
            local res = njt.location.capture("/foo?a=3",
                { args = "b=4" })
            njt.print(res.body)
        ';
        content_by_lua 'njt.exit(njt.OK)';
    }
--- request
GET /lua
--- response_body
a=3&b=4



=== TEST 22: I/O in named location
the nginx core requires the patch https://github.com/agentzh/njt_openresty/blob/master/patches/nginx-1.0.15-reset_wev_handler_in_named_locations.patch
--- config
    location /t {
        echo_exec @named;
    }

    location @named {
        access_by_lua '
            njt.location.capture("/hello")
        ';
        echo done;
    }

    location /hello {
        echo hello;
    }
--- request
    GET /t
--- response_body
done
