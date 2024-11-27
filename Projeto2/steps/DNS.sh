#Configure DNS at tux93, tux94, tux92
#(use DNS server services.netlab.fe.up.pt (10.227.20.3)

#um dos 2 deve funcionar:
nameserver 172.16.1.1
nameserver 10.227.20.3

#Em todos os tux:
ping goole.com
