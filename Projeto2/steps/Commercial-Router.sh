
#verificar ether
/interface bridge port remove [find interface=ether5]
/interface bridge port add bridge=bridge91 interface=ether5




#No Tux92 conectar ao router desde o GTKterm com:

Serial Port: /dev/ttyS0
Baudrate: 115200
Username: admin
Password: <ENTER>

/system reset-configuration

/ip address add address=172.16.1.99/24 interface=ether1
/ip address add address=172.16.91.254/24 interface=ether2


#Configurar as rotas default nos Tuxs e no Router:
route add default gw 172.16.91.254 # Tux92
route add default gw 172.16.90.254 # Tux93
route add default gw 172.16.91.254 # Tux94
/ip route add dst-address=172.16.90.0/24 gateway=172.16.91.253  # Router console
/ip route add dst-address=0.0.0.0/0 gateway=172.16.1.254        # Router console

#No Tux92:
sysctl net.ipv4.conf.eth0.accept_redirects=0
sysctl net.ipv4.conf.all.accept_redirects=0


#No Tux94:
route del -net 172.16.90.0 gw 172.16.51.253 netmask 255.255.255.0

#Tux92 -> Tux93
ping 172.16.90.1 # Figura 4

#In tux92, do traceroute tux93
traceroute -n 172.16.90.1
traceroute to 172.16.90.1 (172.16.90.1), 30 hops max, 60 byte packets
#Example output:
   #1  172.16.91.254 (172.16.91.254)  0.200 ms  0.204 ms  0.224 ms
   #2  172.16.91.253 (172.16.91.253)  0.354 ms  0.345 ms  0.344 ms
   #3  tux51 (172.16.90.1)  0.596 ms  0.587 ms  0.575 ms

#Adicionar de novo a rota que liga Tux92 ao Tux94
route add -net 172.16.90.0/24 gw 172.16.91.253 


#No Tux92, traceroute para o Tux53
traceroute -n 172.16.90.1
traceroute to 172.16.90.1 (172.16.90.1), 30 hops max, 60 byte packets
1  172.16.91.253 (172.16.91.253)  0.196 ms  0.180 ms  0.164 ms
2  tux51 (172.16.90.1)  0.414 ms  0.401 ms  0.375 ms



#No Tux92, reativar o accept_redirects

#No Tux92:
#Um dos 2 comandos seguintes deve funcionar
echo 0 > /proc/sys/net/ipv4/conf/eth0/accept_redirects
echo 0 > /proc/sys/net/ipv4/conf/all/accept_redirects

sysctl net.ipv4.conf.eth0.accept_redirects=1
sysctl net.ipv4.conf.all.accept_redirects=1 

#tux93
ping 172.16.1.10

#desativar NAT do router:
/ip firewall nat disable 0

#tux93 ping again
ping 172.16.1.10

#Reativar Nat do Router
/ip firewall nat enable 0


