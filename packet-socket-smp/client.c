/*
* Please make sure that you enter server's MAC address in 'smp_directory_service' file
* before executing the program. The file should be present at the place of executable.
* This program needs super user permission in orrder to execute.
*/

#define ETH_SMP_TYPE 0x88B5  // SMP Protocol type
#define ETH_SDS_TYPE 0x88B6

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>

unsigned char dest[ETH_ALEN];
int sock_requested = 0;
char *iface = "eth0";  // Change it to wlan0 if using wifi
unsigned short proto = ETH_SMP_TYPE; // Ethernet payload type

union ethernetframe{
    struct{
      struct ethhdr    header;
      unsigned char    data[ETH_DATA_LEN];
    } field;
    unsigned char    buffer[ETH_FRAME_LEN];
};

// specifies the role a communication endpoint is likely to play
enum role{
    CLIENT = '0',
    PEER = '1',
    SERVER = '2',
    TRACKER = '3',
    PROXY = '4'
};

//each SMP end point is identified by an application level≠unique serviceID
//and the role played by the end point
struct smp_ep {
    enum role r;
    char serviceID[10];
};

//each SMP end point is temporarily mapped to a socket≠like combination of
//<ETH address, port>
struct smp_ep_eth{
    char eth_addr[6];
    int port;
};

int socket_smp(){
    // Creating packet socket
    sock_requested++;
    if(sock_requested == 2) return -2;  // Duplicate request
    int s;
    if ((s = socket(AF_PACKET, SOCK_RAW, htons(proto))) < 0){
        printf("Error: could not open socket\n");
        return -1;
    }
    return s;
}

int read_address(){
    FILE *fp;
    char ch;
    char addr_data[21];

    fp = fopen("smp_directory_service","r");

    if( fp == NULL ){
        perror("Error while opening the file.\n");
        exit(1);
    }

    int i=0;
    while((ch = fgetc(fp)) != '\n'){
      addr_data[i] = ch;
      i++;
    }
    fclose(fp);

    char mac_str[17];
    if(addr_data[0] == '0' && addr_data[2] == SERVER){
        for(i=0; i< 17; i++){
          mac_str[i] = addr_data[i + 4];
        }
    }
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
      &dest[0], &dest[1], &dest[2], &dest[3], &dest[4], &dest[5]);
}


int main(int argc, char **argv) {

    unsigned char *data = "Hello World!";
    unsigned short data_len = strlen(data);

    // Creating packet socket
    int s = socket_smp();
    read_address();
    // Looking up the interface index
    struct ifreq buffer;
    int index;
    memset(&buffer, 0x00, sizeof(buffer));
    strncpy(buffer.ifr_name, iface, IFNAMSIZ);
    if (ioctl(s, SIOCGIFINDEX, &buffer) < 0) {
        printf("Error: could not get interface index\n");
        close(s);
        return -1;
    }
    index = buffer.ifr_ifindex;

    // Looking up source MAC address
    unsigned char source[ETH_ALEN];
    if (ioctl(s, SIOCGIFHWADDR, &buffer) < 0) {
        printf("Error: could not get interface address\n");
        close(s);
        return -1;
    }
    memcpy((void*)source, (void*)(buffer.ifr_hwaddr.sa_data),
           ETH_ALEN);


    // Filling in the packet fields
    union ethernetframe frame;
    memcpy(frame.field.header.h_dest, dest, ETH_ALEN);
    memcpy(frame.field.header.h_source, source, ETH_ALEN);
    frame.field.header.h_proto = htons(proto);
    memcpy(frame.field.data, data, data_len);

    unsigned int frame_len = data_len + ETH_HLEN;

    // Filling in the sockaddr_ll struct
    struct sockaddr_ll saddrll;
    memset((void*)&saddrll, 0, sizeof(saddrll));
    saddrll.sll_family = PF_PACKET;
    saddrll.sll_ifindex = index;
    saddrll.sll_halen = ETH_ALEN;
    memcpy((void*)(saddrll.sll_addr), (void*)dest, ETH_ALEN);

    // Sending the packet
    if (sendto(s, frame.buffer, frame_len, 0,
               (struct sockaddr*)&saddrll, sizeof(saddrll)) > 0){
      printf("Data sent successfully!\n");
    }else{
      printf("Error, could not send data.\n");
    }

    // Closing socket
    close(s);


    // Initiating receiver socket
    short int sock_desc=0, count=0;
    if ((sock_desc = socket(AF_PACKET, SOCK_RAW, htons(proto))) < 0)
          printf("\nError creating socket\n");
    else{
          printf("\nListening...\n");
          struct sockaddr_ll sock_addrll;
          unsigned char frame_buffer[ETH_FRAME_LEN];
          int data_rcvd=0;
          socklen_t sock_addrll_len = (socklen_t)sizeof(sock_addrll);
          data_rcvd = recvfrom(
                      sock_desc,
                      frame_buffer,
                      ETH_FRAME_LEN,
                      0x00,
                      (struct sockaddr*) &sock_addrll,
                      &sock_addrll_len
                  );
          printf("\nData received: %s\n",frame_buffer+sizeof(struct ethhdr));
    }
    return 0;
}
