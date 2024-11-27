#Tux94
ifconfig eth2 up
ifconfig eth2 172.16.51.253/24

/interface bridge port remove [find interface=ether2]
/interface bridge port add bridge=bridge91 interface=ether2

#3 Ip forwarding t4
echo 1 > /proc/sys/net/ipv4/ip_forward
#4 Disable ICMP echo ignore broadcast T4
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts

#Tux4
ifconfig

#Tux92
route add -net  172.16.90.0/24 gw 172.16.91.253 

#Tux93
route add -net  172.16.91.0/24 gw 172.16.90.254 

#Tux93
route -n


#Should all work
ping 172.16.50.254
ping 172.16.51.253
ping 172.16.51.1


arp -d 172.16.91.253 #Tux92
arp -d 172.16.90.254 #Tux93
arp -d 172.16.90.1 #Tux94
arp -d 172.16.91.1 #Tux94