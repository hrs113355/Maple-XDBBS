#include<stdio.h>

char * ipnum2str(unsigned long ipaddr, int reverse);

char * str_ip(unsigned long ipaddr)
{
    return ipnum2str(ipaddr, 0);
}

char * rstr_ip(unsigned long ipaddr)
{
    return ipnum2str(ipaddr, 1); // yes, please reverse it
}

char * ipnum2str(unsigned long ipaddr, int reverse)
{
    static char str_ipaddr[20];
    unsigned long int i, ip[4];

    for (i=0; i<=3; i++)
    {
	ip[i]   = ipaddr >> (3-i) * 8;
	ipaddr -= ip[i]  << (3-i) * 8;
    }

    if (reverse)
	sprintf(str_ipaddr, "%d.%d.%d.%d\0", ip[3], ip[2], ip[1], ip[0]);
    else
	sprintf(str_ipaddr, "%d.%d.%d.%d\0", ip[0], ip[1], ip[2], ip[3]);

    return str_ipaddr;
}
