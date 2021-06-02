 
# Module Dependencies

Kconfig does not allow for CONFIG_* variables to switch package dependencies in Makefile.dep (seems there is a looping dependency somewhere)

At the same time, the application Makefile cannot access CONFIG_* variables, as they are only processed once.

So the CONFIG_DCA_{PS,NETWORK,SAUL,COAP} cannot control the module's dependency list.
Especially gnrc is a huge dependency that is only required for CONFIG_DCA_NETWORK.

## Possible solution

The "/network/" database branch should only be shown when CONFIG_DCA_NETWORK is set AND all relevant modules are settled in the application's Makefile.
So, we do not require all these modules within Makefile.dep

Modules are:

For "/network/":  gnrc_sock gnrc_sock_udp gnrc_icmpv6_echo gnrc_ipv6_nib gnrc_ipv6
For "/saul/":     saul_default
For coap:         gcoap
For dcafs:        

# LCap

(lcap)[https://code.ovgu.de/doriot/wp4/lcap] has a RIOT branch, but does not compile well.

# libcoap

The CoAP interface runs with gcoap. Porting to libcoap is required from the project.
