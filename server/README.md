dns_server

A caching DNS server

## Compiling

```bash
$ make
```

## Simple tutorial

```bash
$ ./server.out -h
Usage: dnsproxy [options]
  -r <local-ip> or --local-addr=<port>
                       (local server ip, default 127.0.0.1)
  -p <port> or --local-port=<port>
                       (local bind port, default 53)
  -R <ip> or --remote-addr=<ip>
                       (remote server ip, default 8.8.8.8)
  -P <port> or --remote-port=<port>
                       (remote server port, default 53)
  -T or --remote-tcp
                       (connect remote server in tcp, default no)
  -h, --help           (print help and exit)
  -v, --version        (print version and exit)
```

## Example

```
$ ./server.out
127.0.0.1 example.com www.example.com
192.168.0.1 *.test.com
192.168.0.2 2*.test.com
192.168.0.3 *3.test.com
```
