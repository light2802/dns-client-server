dns_client                                                                  
                                                                               
A simple dns client like nslookup
Shows all answers sent by the dns server (well currently only shows A,AAAA,MX,
CNAME)
                                                                               
## Compiling                                                                   
                                                                               
```bash                                                                        
$ make
$ make check
```                                                                            
                                                                               
## Simple tutorial                                                             
                                                                               
```bash                                                                        
$ ./client.out http://<domain_name>                                                           
```                                                                            
                                                                               
## Example                                                          
                                                                               
```                                   
$ ./client.out http://www.google.com
Host : www.google.com
SEND SUCESS
Recieve success
Storing done
IPv4 for www.google.com : 142.250.199.164                                 
```
