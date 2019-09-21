#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <string.h>
#include <unistd.h>

#define NETLINK_TEST 30
#define MAX_PAYLOAD 7000 //max payload size(?

struct sockaddr_nl src_addr, dest_addr;
struct iovec iov;
int netlink_socket;
struct nlmsghdr *nlh = NULL;
struct msghdr msg;
int main(int argc, char *argv[])
{

    int enterpid = 1;
    int my_pid = getpid();
    char mode = 'c';
    char payload[20];

    if(argv[1] != NULL) {
        sscanf(argv[1],"%*c%c",&mode);
        if(mode == 's' || mode == 'p') {
            enterpid = my_pid;
        }
        sscanf(argv[1],"%*c%c%d",&mode,&enterpid);
    }
    //printf("%c %d",mode,enterpid);




    sprintf(payload,"%c %d",mode,enterpid);
    netlink_socket = socket(AF_NETLINK,SOCK_RAW,NETLINK_TEST);
    if(netlink_socket == -1) {
        printf("create socket error\n");
        return -1;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = my_pid;// pid for self
    src_addr.nl_groups = 0 ;
    bind(netlink_socket, (struct sockaddr*)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;//the pid for kernel
    dest_addr.nl_groups = 0;

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));

    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
    strcpy(NLMSG_DATA(nlh), payload);//this is to sending the message

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sendmsg(netlink_socket, &msg, 0);
//    printf("waiting for kernel\n");

    memset(nlh,0,NLMSG_SPACE(MAX_PAYLOAD));
    recvmsg(netlink_socket, &msg, 0);
    printf("%s",(char *)NLMSG_DATA(nlh));




    close(netlink_socket);
    free(nlh);
    return 0;
}
