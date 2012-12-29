#include <stdio.h>
#include <string.h>
#include <osutil.h>
#include <math.h>
#include <mntent.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <linux/reboot.h>
#include <poll.h>
#include <sys/select.h>

namespace NS_ZH_UTIL {

int CaculateMemLoad()
{
	FILE *fd;
    char buff[1024] = {0};
    char* pbuff = buff;
    fd = fopen ("/proc/meminfo", "r");
	if(fd)
	{
		//MemTotal
		unsigned long MemTotal = 0; pbuff = buff;
		fgets (buff, sizeof(buff), fd); while(*pbuff<'0' || *pbuff>'9') ++pbuff; sscanf(pbuff, "%lu", &MemTotal);
		//MemFree
		unsigned MemFree = 0; pbuff = buff;
		fgets (buff, sizeof(buff), fd); while(*pbuff<'0' || *pbuff>'9') ++pbuff; sscanf(pbuff, "%lu", &MemFree);
		//Buffers
		unsigned Buffers = 0; pbuff = buff;
		fgets (buff, sizeof(buff), fd); while(*pbuff<'0' || *pbuff>'9') ++pbuff; sscanf(pbuff, "%lu", &Buffers);
		//Cached
		unsigned Cached = 0; pbuff = buff;
		fgets (buff, sizeof(buff), fd); while(*pbuff<'0' || *pbuff>'9') ++pbuff; sscanf(pbuff, "%lu", &Cached);
		fclose(fd);
		//
		return round(((float)(MemTotal-MemFree-Buffers-Cached)/(float)MemTotal)*100);
	}
	return -1;
}

int GetCPULoad(SCPU_Load &load)
{
	FILE *fd;
    char buff[1024] = {0};
    char* pbuff = buff;
    fd = fopen ("/proc/stat", "r");
	if(fd)
	{
		//cpu user nice system idle iowait irq softirq
		pbuff = buff;
		fgets (buff, sizeof(buff), fd); while(*pbuff<'0' || *pbuff>'9') ++pbuff;
		sscanf(pbuff, "%lu%lu%lu%lu%lu%lu%lu", &load._user, &load._nice, &load._system, &load._idle, &load._iowait, &load._irq, &load._softirq);
		::fclose(fd);
	}
	return -1;
}

int CaculateCPULoad(SCPU_Load &load1, SCPU_Load &load2)
{
	//return (float)(load2.used() - load1.used())/(float)(load2.total() - load1.total()) * 100;
	unsigned long total = load2.total() - load1.total();
	unsigned long idle = load2.idle() - load1.idle();
	return round((float)(total - idle)/(float)total * 100);
}

int CaculateDiskLoad()
{
	unsigned int blkfree = 0;
	unsigned int blksize = 0;
	unsigned int blkused = 0;
	unsigned int blkavlb = 0;
    FILE *fp = fopen("/etc/mtab","rb");
	if (NULL == fp) return -1;
	while(1)
	{
		struct mntent *mt = getmntent(fp);
		if(NULL==mt)
		{
			break;
		}
		if (NULL != strstr(mt->mnt_fsname,"/dev/"))
		{
			struct statfs buf;
			statfs(mt->mnt_dir,&buf);
			blksize+= (buf.f_bsize/1024*buf.f_blocks);
			blkfree+= (buf.f_bsize/1024*buf.f_bfree);
			blkavlb+= (buf.f_bsize/1024*buf.f_bavail);
		}
	}
	blkused = blksize - blkfree;
	fclose(fp);
	return round((float)(blksize - blkavlb)/(float)(blksize)*100);
}

static const char *HEX_TO_CHAR="0123456789abcdef";
void GetIP(const char *ifName, const SIP_Info& ip)
{

	int s;
	struct ifreq ifr;
	static char *none_ip="0.0.0.0";
	static char *none_phy="00:00:00:00:00:00";
	if(NULL==ifName)
	{
		::strcpy((char*)(&ip._ipaddr[0]), none_ip);
		::strcpy((char*)(&ip._gateway[0]), none_ip);
		::strcpy((char*)(&ip._mask[0]), none_ip);
		::strcpy((char*)(&ip._phyaddr[0]), none_phy);
		return ;
	}
	s=::socket(AF_INET, SOCK_DGRAM, 0);
	if(-1==s)
	{
		strcpy((char*)(&ip._ipaddr[0]), none_ip);
		strcpy((char*)(&ip._gateway[0]), none_ip);
		strcpy((char*)(&ip._mask[0]), none_ip);
		strcpy((char*)(&ip._phyaddr[0]), none_phy);
		return ;
	}
	::bzero(ifr.ifr_name, sizeof(ifr.ifr_name));
	strncpy((char*)(&ifr.ifr_name[0]), ifName, sizeof(ifr.ifr_name)-1);
	//ip
	if(-1==ioctl(s, SIOCGIFADDR, &ifr))
	{
		TEMP_FAILURE_RETRY(::close(s));
		strcpy((char*)(&ip._ipaddr[0]), none_ip);
		strcpy((char*)(&ip._gateway[0]), none_ip);
		strcpy((char*)(&ip._mask[0]), none_ip);
		strcpy((char*)(&ip._phyaddr[0]), none_phy);
		return ;
	}
	strcpy((char*)(&ip._ipaddr[0]), ::inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr));
	//mask
	if(-1==ioctl(s, SIOCGIFNETMASK, &ifr))
	{
		strcpy((char*)(&ip._mask[0]), none_ip);
		strcpy((char*)(&ip._phyaddr[0]), none_phy);
		strcpy((char*)(&ip._gateway[0]), none_ip);
		return ;
	}
	strcpy((char*)(&ip._mask[0]), ::inet_ntoa(((struct sockaddr_in *) &ifr.ifr_netmask)->sin_addr));
	//physical address
	if(-1==ioctl(s, SIOCGIFHWADDR, &ifr))
	{
		strcpy((char*)(&ip._phyaddr[0]), none_phy);
		strcpy((char*)(&ip._gateway[0]), none_ip);
		return ;
	}
	char* p = (char*)(&ip._phyaddr[0]);
	for(int ii = 0; ii < 6; ++ii)
	{
		if(ii > 0) *p++ =':';
		*p++ = HEX_TO_CHAR[ifr.ifr_hwaddr.sa_data[ii]>>4];
		*p++ = HEX_TO_CHAR[ifr.ifr_hwaddr.sa_data[ii]&0x0F];
	}
	TEMP_FAILURE_RETRY(::close(s));
	//gate way
	//read file /proc/net/route
	//Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask            MTU     Window  IRTT
	//eth0    0000FEA9        00000000        0001    0       0       0       0000FFFF        0       0       0
	//eth0    0000000A        00000000        0001    0       0       0       000000FF        0       0       0
	//lo      0000007F        00000000        0001    0       0       0       000000FF        0       0       0
	//eth0    00000000        0108080A        0003    0       0       0       00000000        0       0       0
	FILE* fd = ::fopen("/proc/net/route", "r");
	if(NULL == fd)
	{
		strcpy((char*)(&ip._gateway[0]), none_ip);
		return;
	}
	char line[1024] = {0};
	char *pline = 0;
	::fgets(line, sizeof(line), fd);
	unsigned dest, gateway, flags;
	char ifname_[128];
	while(::fgets(line, sizeof(line), fd))
	{
		//pline = line;
		//while(*pline == ' ' || *pline == '\t') ++pline;
		//while(*pline != ' ' && *pline != '\t') ++pline;
		//while(*pline == ' ' || *pline == '\t') ++pline;
		::bzero(ifname_, sizeof(ifname_));
		::sscanf(line, "%s%x%x%x", ifname_, &dest, &gateway, &flags);
		if(strcmp(ifName, ifname_) == 0 &&
			flags == 0x0003)
		{
			strcpy((char*)(&ip._gateway[0]), ::inet_ntoa(*(struct in_addr*)&gateway));
			return;
		}
	}
	strcpy((char*)(&ip._gateway[0]), none_ip);
	return ;
}

int SetIP(const char* ifName, const SIP_Info& ip)
{
	char command[512] = {0};
	//
	::snprintf(command, sizeof(command)-1, "ifconfig %s %s netmask %s", ifName, ip._ipaddr, ip._mask);
	if(::system(command) != 0) return -1;
	//
	::bzero(command, sizeof(command));
	::snprintf(command, sizeof(command)-1, "route add default gw %s", ip._gateway);
	if(::system(command) != 0) return -1;
}

int Reboot()
{
	::sync();
	return ::reboot(LINUX_REBOOT_CMD_RESTART);
}

int send_n(int sock, const void* data, size_t len, struct timeval timeout)
{
	size_t bytes_transferred = 0;
	ssize_t n;

	for (bytes_transferred = 0; bytes_transferred < len; bytes_transferred += n)
	{
		// Try to transfer as much of the remaining data as possible.
		// Since the socket is in non-blocking mode, this call will not
		// block.
		n = TEMP_FAILURE_RETRY(::send(sock, (char *)data + bytes_transferred, len - bytes_transferred, 0));
		// Check for errors.
		if(n == -1)
		{
			if(errno == EWOULDBLOCK || errno == EAGAIN || errno == ENOBUFS)
			{
				struct pollfd polls; polls.fd = sock; polls.events = POLLWRNORM; polls.revents = 0;
				int wait = timeout.tv_sec*1000+timeout.tv_usec/1000; wait = wait == 0 ? -1 : wait;
				// Wait upto <timeout> for the blocking to subside.
				int const rtn = TEMP_FAILURE_RETRY(::poll(&polls, 1, wait));
				if(rtn == 0)
				{
					errno = ETIME;
					return bytes_transferred; 	//timeout
				}
				else if(rtn == -1)
				{
					return bytes_transferred; 	//poll error
				}
				else if(polls.revents & POLLERR)//sock error
				{
					return bytes_transferred;
				}
				n = 0;
				//@zh for test
				//printf("********test: poll return %d*****************\n", rtn);
				//@zh
				continue;
			}
			else return bytes_transferred;
		}
		else if (n == 0)
		{
			return bytes_transferred;
		}
	}
	return bytes_transferred;
}

long operator-(struct timespec& t1, struct timespec& t2)
{
	return t1.tv_sec*1000+t1.tv_nsec/1000000 - (t2.tv_sec*1000+t2.tv_nsec/1000000);
}

void delay(int ms)
{
	struct timeval timeout; timeout.tv_sec = ms/1000; timeout.tv_usec=(ms%1000)*1000;
	::select(0, 0, 0, 0, &timeout);
}

//256*5+8*8+256*8+256*8=5440
unsigned char BitMaskBit1[256]={0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8};
unsigned char BitMaskFirstBit0[256]={0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,0xff};
unsigned char BitMaskFirstBit1[256]={0xff,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0};
unsigned char BitMaskLastBit0[256]={7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,2,2,2,2,1,1,0,0xff};
unsigned char BitMaskLastBit1[256]={0xff,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
unsigned char BitMaskAll1Mask[8][8] = {{1,3,7,15,31,63,127,255},{2,6,14,30,62,126,254,0},{4,12,28,60,124,252,0,0},{8,24,56,120,248,0,0,0},{16,48,112,240,0,0,0,0},{32,96,224,0,0,0,0,0},{64,192,0,0,0,0,0,0},{128,0,0,0,0,0,0,0}};
unsigned char BitMaskContinuousBit1[8][256]={
{0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,8},
{0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,4,4,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,5,5,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,4,4,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,6,6,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,4,4,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,5,5,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,4,4,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,7,7},
{0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,3,3,3,3,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,4,4,4,4,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,3,3,3,3,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,5,5,5,5,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,3,3,3,3,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,4,4,4,4,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,3,3,3,3,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,6,6,6,6},
{0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,5,5,5,5,5,5,5,5},
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4},
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};
unsigned char BitMaskContinuousBit0[8][256]={
{8,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0},
{7,7,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,4,4,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,5,5,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,4,4,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,6,6,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,4,4,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,5,5,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0,4,4,0,0,1,1,0,0,2,2,0,0,1,1,0,0,3,3,0,0,1,1,0,0,2,2,0,0,1,1,0,0},
{6,6,6,6,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,3,3,3,3,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,4,4,4,4,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,3,3,3,3,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,5,5,5,5,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,3,3,3,3,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,4,4,4,4,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,3,3,3,3,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0},
{5,5,5,5,5,5,5,5,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
{4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

} // namespace
