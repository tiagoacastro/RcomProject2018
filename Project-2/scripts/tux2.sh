#!/bin/bash
/etc/init.d/networking restart				
ifconfig eth0 up					
ifconfig eth0 172.16.41.1/24			
route add -net 172.16.40.0/24 gw 172.16.41.253
route add default gw 172.16.41.254

cp /etc/resolv.conf /etc/resolv.conf.backup
echo "search netlab.fe.up.pt" > /etc/resolv.conf
echo "nameserver 172.16.1.1" >> /etc/resolv.conf
echo "nameserver 193.136.28.10" >> /etc/resolv.conf