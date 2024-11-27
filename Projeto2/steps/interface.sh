admin
/system reset-configuration
y

/interface bridge add name=bridge90
/interface bridge add name=bridge91

#o ehter é consoante aos cabos
/interface bridge port remove [find interface=ether3] 
/interface bridge port remove [find interface=ether11] 
/interface bridge port remove [find interface=ether12] 

/interface bridge port add bridge=bridge90 interface=ether11
/interface bridge port add bridge=bridge90 interface=ether12 
/interface bridge port add bridge=bridge91 interface=ether3

/interface bridge port print

#Tux93 -> Tux94 Tudo ok
$ ping 172.16.50.254

$ ping 172.16.51.1
#Tux92 -> connect: Network is unreachable