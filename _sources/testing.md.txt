# Testing Procedures

## Network Bandwidth

Requires ttcp from rtems-net-services. This can be trivially compiled on the host using `gcc -o ttcp ttcp.c`

Server side:
```
$ ./ttcp -r -s
```

On the RTEMS target:
```
# Replace 10.0.2.2 with your server; in this case 10.0.2.2 is the host system under Qemu
$ ttcp -t -s 10.0.2.2 -v 
```
