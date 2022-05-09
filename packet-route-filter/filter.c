#define KBUILD_MODNAME "filter"
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/udp.h>
#include <linux/tcp.h>

const volatile unsigned long long port = 0;


//BPF_ARRAY(last,u64,1);
BPF_HASH(last);

/*xdp_md defined in /include/uapi/linux/bpf.h*/
int udpfilter(struct xdp_md *ctx) {


  u64 dest;
  u64 *org_src;
  u64 key = 0;

  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;


  /*ethhdr defined in: /include/uapi/linux/if_ether.h*/
  struct ethhdr *eth = data;
  if ((void*)eth + sizeof(*eth) <= data_end) {

    /*iphdr: defined in /include/uapi/linux/ip.h*/  
    struct iphdr *ip = data + sizeof(*eth);
    if ((void*)ip + sizeof(*ip) <= data_end) {
      if (ip->protocol == IPPROTO_TCP) {

	/*udphdr: defined in /include/uapi/linux/udp.h*/
        struct tcphdr *tcp = (void*)ip + sizeof(*ip);
        if ((void*)tcp + sizeof(*tcp) <= data_end) {

          if (tcp->dest == ntohs(7999) && tcp->source != ntohs(8000) /*tcp->source == ntohs(5000)*/) {

	    dest = tcp->source;
	    //src = tcp->source;

	    last.update(&key, &dest);

	    org_src = last.lookup(&key);
	    if (org_src != NULL)
            	bpf_trace_printk("hash element:  %lld\n",*org_src);

            //bpf_trace_printk("tcp port 7999\n");
            bpf_trace_printk("forward to 8000 from 7999 source port %d\n",ntohs(tcp->source));
            tcp->dest = ntohs(8000);
            tcp->source = ntohs(7999);
  	    //return XDP_TX;
          }
	  if (tcp->dest == ntohs(8000))
		  bpf_trace_printk("8000 received packet from 7999...    dest: %d source: %d ",ntohs(tcp->dest), ntohs(tcp->source) );
	  if (tcp->source == ntohs(8000))
		  bpf_trace_printk("8000 send packet back to 7999...    dest: %d source: %d ",ntohs(tcp->dest), ntohs(tcp->source) );

	  if (tcp->dest == ntohs(7999) && tcp->source == ntohs(8000)) {
            //org_src = last.lookup(&key);
	    //if (org_src != NULL) {
		    //tcp->dest = ntohs(5000);
		    
	    	    org_src = last.lookup(&key);
	   	    if (org_src != NULL) {
		    	tcp->dest = *org_src;
		    	tcp->source = ntohs(7999);
            	    	bpf_trace_printk("return from 7999 to 5000 dest: %d   source:  %d\n",ntohs(tcp->dest),ntohs(tcp->source));
		    }
	    //}
		

	  }

        }
      }
    }
  }
  return XDP_PASS;
  //return XDP_REDIRECT
}
