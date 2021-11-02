dns_server

A caching DNS server

## Compiling

```bash
$ make
```

## Simple tutorial

```bash
$ server -h
Usage: dnsproxy [options]
  -d or --daemon
                       (daemon mode)
  -p <port> or --port=<port>
                       (local bind port, default 53)
  -R <ip> or --remote-addr=<ip>
                       (remote server ip, default 8.8.8.8)
  -P <port> or --remote-port=<port>
                       (remote server port, default 53)
  -T or --remote-tcp
                       (connect remote server in tcp, default no)
  -f <file> or --hosts-file=<file>
                       (user-defined hosts file)
  -h, --help           (print help and exit)
  -v, --version        (print version and exit)
```

## Hosts file example

```
127.0.0.1 example.com www.example.com
192.168.0.1 *.test.com
192.168.0.2 2*.test.com
192.168.0.3 *3.test.com
```
