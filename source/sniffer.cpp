#include "sniffer.h"
#include "protocoltype.h"
#include <QDebug>
#include <iostream>

Sniffer::Sniffer()
{
    iNetDevsNum = 0;
}

Sniffer::~Sniffer()
{

}

pcap_if_t * Sniffer::findAllDevs(char *szFlag){
    if (pcap_findalldevs_ex(szFlag, NULL, &alldevs, errbuf) == -1) {
         qDebug() << "error " ;
     }

    /* 打印列表 */
    for(dev= alldevs; dev != NULL; dev= dev->next)
    {       iNetDevsNum++;
            qDebug() << dev->name << ":" <<dev->description;
    }
    return alldevs;
}

bool Sniffer::openNetDev(int devNum){
    int i;
    if(devNum < 0 || devNum >= iNetDevsNum)
        {
            qDebug() << "\nInterface number out of range.\n";
            /* 释放设备列表 */
            freeDevsMem();
            return -1;
        }
        /* 跳转到选中的适配器 */
        for(dev=alldevs, i=0; i< devNum-1 ;dev=dev->next, i++);

        /* 打开设备 */
        if ( (adhandle= pcap_open(dev->name,          // 设备名
                                  65536,            // 65535保证能捕获到不同数据链路层上的每个数据包的全部内容
                                  PCAP_OPENFLAG_PROMISCUOUS,    // 混杂模式
                                  1000,             // 读取超时时间
                                  NULL,             // 远程机器验证
                                  errbuf            // 错误缓冲池
                                  ) ) == NULL)
        {
            qDebug() << "\nInterface number out of range.\n";
            /* 释放设备列表 */
            freeDevsMem();
            return -1;
        }
}

bool Sniffer::openNetDev(char *szDevName)
{


    fp = pcap_open(szDevName,			// 设备名
                        65536,		// 数据包大小限制
                        PCAP_OPENFLAG_PROMISCUOUS,				// 网卡设置打开模式
                        1000,				// 读取超时时间
                        NULL,				// 远程机器验证
                        errbuf);			// 错误缓冲

    if (fp == NULL) {
        return -1;
    }

    return 1;
}

bool Sniffer::setDevsFilter(char *szFilter)
{
    /* 检查数据链路层，为了简单，我们只考虑以太网 */
        if(pcap_datalink(adhandle) != DLT_EN10MB)
        {
            qDebug() <<"\nThis program works only on Ethernet networks.\n" ;
            /* 释放设备列表 */
            freeDevsMem();
            return -1;
        }
        u_int	netmask;

        if(dev->addresses != NULL)
            /* 获得接口第一个地址的掩码 */
            netmask=((struct sockaddr_in *)(dev->addresses->netmask))->sin_addr.S_un.S_addr;
        else
            /* 如果接口没有地址，那么我们假设一个C类的掩码 */
            netmask=0xffffff;

        qDebug() <<"netmask is "<< adhandle<<endl;
        //编译过滤器
        if (pcap_compile(adhandle, &fcode, szFilter, 1, netmask) <0 )
        {
            freeDevsMem();
            return -1;
        }

        //设置过滤器
        if (pcap_setfilter(adhandle, &fcode)<0)
        {
            /* 释放设备列表 */
            freeDevsMem();
            return -1;
        }
}

void Sniffer::captureByCallBack(snifferCB func){
    /* 开始捕捉 */
    pcap_loop(adhandle, 0, func, NULL);
    qDebug() << adhandle;
}

int Sniffer::captureOnce(){
   return   pcap_next_ex( adhandle, &header, &pkt_data);
}


bool Sniffer::openDumpFile(const char *szFileName){
    dumpfile = pcap_dump_open(adhandle, szFileName);
    if(dumpfile!=NULL)
        return true;
    return false;
}

void Sniffer::saveDumpFile(){
   if(dumpfile != NULL)
        pcap_dump((unsigned char *)dumpfile, header, pkt_data);
}

bool Sniffer::openSavedDumpFile(const char *szFileName,int packNum){
    char source[PCAP_BUF_SIZE];
    int i;
    if ( pcap_createsrcstr( source,         // 源字符串
                                PCAP_SRC_FILE,  // 我们要打开的文件
                                NULL,           // 远程主机
                                NULL,           // 远程主机端口
                                szFileName,        // 我们要打开的文件名
                                errbuf          // 错误缓冲区
                                ) != 0)
        {
            fprintf(stderr,"\nError creating a source string\n");
            return false;
        }
    else {
        openNetDev(source);
        int num = 0;
        if(packNum == 1)
            return pcap_next_ex( fp, &header, &pkt_data);
        while((pcap_next_ex( fp, &header, &pkt_data)) >= 0)
           {
                num = num + 1;
//               /* 打印pkt时间戳和pkt长度 */
//               printf("%ld:%ld (%ld)\n", header->ts.tv_sec, header->ts.tv_usec, header->len);

//               /* 打印数据包 */
//               for (i=1; (i < header->caplen + 1 ) ; i++)
//               {
//                   printf("%.2x ", pkt_data[i-1]);
//                   if ( (i % 16) == 0) printf("\n");
//               }

//               printf("\n\n");
//               printf("\n\n");

                if(num+1 == packNum){
                    if(pcap_next_ex( fp, &header, &pkt_data) >= 0)
                        return pcap_next_ex( fp, &header, &pkt_data);
                }
           }

        return false;

    }
}



void Sniffer::freeDevsMem(){
    if(alldevs){
        pcap_freealldevs(alldevs);
        alldevs = NULL;
    }
}


