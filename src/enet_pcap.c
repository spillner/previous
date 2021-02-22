#include "m68000.h"
#include "ethernet.h"
#include "enet_pcap.h"
#include "queue.h"
#include "host.h"

#if HAVE_PCAP
#if defined _WIN32
#undef mkdir
#endif
#include <pcap.h>

/****************/
/* --- PCAP --- */


/* PCAP prototypes */
pcap_t *pcap_handle;

/* queue prototypes */
queueADT	pcapq;

int pcap_started;
static SDL_mutex *pcap_mutex = NULL;
SDL_Thread *pcap_tick_func_handle;

//This function is to be periodically called
//to keep the internal packet state flowing.
static void pcap_tick(void)
{
    struct pcap_pkthdr h;
    const unsigned char *data;
    
    if (pcap_started) {
        SDL_LockMutex(pcap_mutex);
        data = pcap_next(pcap_handle,&h);
        SDL_UnlockMutex(pcap_mutex);

        if (data && h.caplen > 0) {
            if (h.caplen > 1516)
                h.caplen = 1516;
            
            struct queuepacket *p;
            p=(struct queuepacket *)malloc(sizeof(struct queuepacket));
            SDL_LockMutex(pcap_mutex);
            p->len=h.caplen;
            memcpy(p->data,data,h.caplen);
            QueueEnter(pcapq,p);
            SDL_UnlockMutex(pcap_mutex);
            Log_Printf(LOG_WARN, "[PCAP] Output packet with %i bytes to queue",h.caplen);
        }
    }
}

static int tick_func(void *arg)
{
    while(pcap_started)
    {
        host_sleep_ms(10);
        pcap_tick();
    }
    return 0;
}


void enet_pcap_queue_poll(void)
{
    if (pcap_started) {
        SDL_LockMutex(pcap_mutex);
        if (QueuePeek(pcapq)>0)
        {
            struct queuepacket *qp;
            qp=QueueDelete(pcapq);
            Log_Printf(LOG_WARN, "[PCAP] Getting packet from queue");
            enet_receive(qp->data,qp->len);
            free(qp);
        }
        SDL_UnlockMutex(pcap_mutex);
    }
}

void enet_pcap_input(Uint8 *pkt, int pkt_len) {
    if (pcap_started) {
        Log_Printf(LOG_WARN, "[PCAP] Input packet with %i bytes",enet_tx_buffer.size);
        SDL_LockMutex(pcap_mutex);
        if (pcap_sendpacket(pcap_handle, pkt, pkt_len) < 0) {
            Log_Printf(LOG_WARN, "[PCAP] Error: Couldn't transmit packet!");
        }
        SDL_UnlockMutex(pcap_mutex);
    }
}

void enet_pcap_stop(void) {
    int ret;
    
    if (pcap_started) {
        Log_Printf(LOG_WARN, "Stopping PCAP");
        pcap_started=0;
        QueueDestroy(pcapq);
        SDL_DestroyMutex(pcap_mutex);
        SDL_WaitThread(pcap_tick_func_handle, &ret);
        pcap_close(pcap_handle);
    }
}

void enet_pcap_start(Uint8 *mac) {
    char errbuf[PCAP_ERRBUF_SIZE];
    char *dev;
    struct bpf_program fp;
    char filter_exp[255];
    bpf_u_int32 net = 0xffffffff;

    if (!pcap_started) {
        Log_Printf(LOG_WARN, "Starting PCAP");
        
#if 0
        dev = pcap_lookupdev(errbuf);
#else
        dev = ConfigureParams.Ethernet.szInterfaceName;
#endif
        if (dev == NULL) {
            Log_Printf(LOG_WARN, "[PCAP] Error: Couldn't find device: %s", errbuf);
            return;
        }
        Log_Printf(LOG_WARN, "Device: %s", dev);
        
        pcap_handle = pcap_open_live(dev, 1518, 1, 1000, errbuf);
        
        if (pcap_handle == NULL) {
            Log_Printf(LOG_WARN, "[PCAP] Error: Couldn't open device %s: %s", dev, errbuf);
            return;
        }
        
        if (pcap_getnonblock(pcap_handle, errbuf) == 0) {
            Log_Printf(LOG_WARN, "[PCAP] Setting interface to non-blocking mode.");
            if (pcap_setnonblock(pcap_handle, 1, errbuf) != 0) {
                Log_Printf(LOG_WARN, "[PCAP] Error: Couldn't set interface to non-blocking mode: %s", errbuf);
                return;
            }
        } else {
            Log_Printf(LOG_WARN, "[PCAP] Error: Unexpected error: %s", errbuf);
            return;
        }
        
#if 1 // TODO: Check if we need to take care of RXMODE_ADDR_SIZE and RX_PROMISCUOUS/RX_ANY
        sprintf(filter_exp,"(((ether dst ff:ff:ff:ff:ff:ff) or (ether dst %02x:%02x:%02x:%02x:%02x:%02x) or (ether[0] & 0x01 = 0x01)))",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        if (pcap_compile(pcap_handle, &fp, filter_exp, 0, net) == -1) {
            Log_Printf(LOG_WARN, "[PCAP] Warning: Couldn't parse filter %s: %s", filter_exp, pcap_geterr(pcap_handle));
        } else {
            if (pcap_setfilter(pcap_handle, &fp) == -1) {
                Log_Printf(LOG_WARN, "[PCAP] Warning: Couldn't install filter %s: %s", filter_exp, pcap_geterr(pcap_handle));
            }
        }
#endif
        pcap_started=1;
        pcapq = QueueCreate();
        pcap_mutex=SDL_CreateMutex();
        pcap_tick_func_handle=SDL_CreateThread(tick_func,"PCAPTickThread", (void *)NULL);
    }
}
#endif
