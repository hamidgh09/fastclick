/*
 * Multi-Threaded packet generator reading directly from a trace. Uses DPDK.
 *
 * This version does not compute latency or receive packets back on another interface.
 * It uses 4 threads to play packets. A single thread reads packet from the trace
 * and passes them in an RR fashion to the 4 playing threads.
 *
 * See also the REplaycap version that will preload packets in memory, it's faster.
 *
 * Example usage: bin/click --dpdk -l 0-15 -- conf/pktgen/pktgen-l2-mt-playpcap.click trace=/tmp/trace.pcap
 */

define($trace /path/to/trace.pcap)
define($time 30)

define($bout 32)

define($INsrcmac 04:3f:72:dc:4a:64)
define($INdstmac 50:6b:4b:f3:7c:70)

define($ignore 0)
define($replay_count 1)
define($port 0000:51:00.0)
define($quick true)
define($txverbose 99)
define($rxverbose 99)

d :: DPDKInfo(262143)

fdIN0 :: FromDump($trace, STOP true, TIMING false, TIMING_FNT "", END_AFTER 0, ACTIVE true, BURST 1);

tdIN :: ToDPDKDevice($port, BLOCKING true, BURST $bout, VERBOSE $txverbose, IQUEUE $bout, NDESC 0, IPCO true )

elementclass Numberise { $magic |
    input
     -> Strip(14)
     -> check :: MarkIPHeader
     -> ResetIPChecksum()
     -> Unstrip(14) 
     -> output
}

ender :: Script(TYPE PASSIVE,
                print "Limit of 50000000 reached",
                stop,
                stop);
  rr :: RoundRobinSwitch(SPLITBATCH false);

fdIN0 -> limit0   :: Counter(COUNT_CALL 50000000 ender.run) -> [0]rr

elementclass Generator { $magic |
input
  -> replay :: Pipeliner(BLOCKING false, CAPACITY 1024, ALWAYS_UP true)
  -> MarkMACHeader
  -> EnsureDPDKBuffer
  -> doethRewrite :: { input[0] -> active::Switch(OUTPUT 0)[0] -> rwIN :: EtherRewrite($INsrcmac,$INdstmac) -> [0]output;   active[1] -> [0]output}
  -> Pad
  -> Numberise($magic)
  -> avgSIN :: AverageCounter(IGNORE $ignore)
  -> output;
}

rr[0] -> gen0 :: Generator(\<5700>) -> tdIN;StaticThreadSched(gen0/replay 0/1);
rr[1] -> gen1 :: Generator(\<5701>) -> tdIN;StaticThreadSched(gen1/replay 0/2);
rr[2] -> gen2 :: Generator(\<5702>) -> tdIN;StaticThreadSched(gen2/replay 0/3);
rr[3] -> gen3 :: Generator(\<5703>) -> tdIN;StaticThreadSched(gen3/replay 0/4);


StaticThreadSched(fdIN0 0/0);

ig :: Script(TYPE ACTIVE,
    goto end $(eq 0 0),
    set s $(now),
    set lastcount 0,
    set lastbytes 0,
    set lastbytessent 0,
    set lastsent 0,
    set lastdrop 0,
    set last $s,
    set indexA 0,
    set indexB 0,
    set indexC 0,
    set indexD 0,
    label loop,
    wait 0s,
    set n $(now),
    set t $(sub $n $s),
    set elapsed $(sub $n $last),
    set last $n,

    set count $(RIN/avg.add count),
    set sent $(avgSIN.add count),
    set bytessent $(avgSIN.add byte_count),
    set bytes $(RIN/avg.add byte_count),
    print "IG-$t-RESULT-IGCOUNT $(sub $count $lastcount)",
    print "IG-$t-RESULT-IGSENT $(sub $sent $lastsent)",
    print "IG-$t-RESULT-IGBYTESSENT $(sub $bytessent $lastbytessent)",
    set drop $(sub $sent $count),
    print "IG-$t-RESULT-IGDROPPED $(sub $drop $lastdrop)",
    set lastdrop $drop,
    print "IG-$t-RESULT-IGTHROUGHPUT $(div $(mul $(add $(mul $(sub $count $lastcount) 24) $(sub $bytes $lastbytes)) 8) $elapsed)",
    goto next $(eq 0 0),
//                print "IG-$t-RESULT-ILAT01 $(RIN/tsd0.perc01 $indexA)",
//                print "IG-$t-RESULT-ILAT50 $(RIN/tsd0.median $indexA)",
    print "IG-$t-RESULT-ILATENCY $(RIN/tsd0.average $indexA)",
//                print "IG-$t-RESULT-ILAT99 $(RIN/tsd0.perc99 $indexA)",
    set indexA $(RIN/tsd0.index),
    label next,
    set lastcount $count,
    set lastsent $sent,
    set lastbytes $bytes,
    set lastbytessent $bytessent,
    goto loop,
    label end
)

StaticThreadSched(ig 15);


avgSIN :: HandlerAggregate( ELEMENT gen0/avgSIN,ELEMENT gen1/avgSIN,ELEMENT gen2/avgSIN,ELEMENT gen3/avgSIN );


starter :: Script(TYPE PASSIVE,
			print 'Thread 0 started', wait 100ms,
            print 'Thread 1 started', write unqueue1.active true, wait 100ms,
            print 'Thread 2 started', write unqueue2.active true, wait 100ms,
            print 'Thread 3 started', write unqueue3.active true, wait 100ms,
);
dm :: DriverManager(  print "Waiting 2 seconds before launching generation...",
                wait 2s,

                print "EVENT GEN_BEGIN",
                print "Starting gen...",

                print "Starting timer wait...",
                set starttime $(now),
                wait $time,
                set stoptime $(now),
                print "EVENT GEN_DONE",
                wait 1s,
                print "RESULT-TESTTIME $(sub $stoptime $starttime)",
                set sent $(avgSIN.add count),
                print "RESULT-SENT $sent",
                print "RESULT-TX $(avgSIN.add link_rate)",
                print "RESULT-TXPPS $(avgSIN.add rate)",
                stop);

StaticThreadSched(dm 15);
