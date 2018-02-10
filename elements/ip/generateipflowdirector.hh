#ifndef CLICK_GENERATEIPFLOWDIRECTOR_HH
#define CLICK_GENERATEIPFLOWDIRECTOR_HH

#include <click/timestamp.hh>

#include "generateipfilter.hh"
#include "generateipflowdirector.hh"

CLICK_DECLS

/*
=c

GenerateIPFlowDirector(PORT, NB_QUEUES, NB_RULES [, POLICY, KEEP_SPORT, KEEP_DPORT] )

=s ip

generates DPDK Flow Director patterns out of input traffic

=d

Expects IP packets as input. Should be placed downstream of a
CheckIPHeader or equivalent element.


Keyword arguments are:

=6

=item PORT

Integer. Port number to install the generated rules.
Default is 0.

=item NB_QUEUES

Integer. Number of hardware queues to redirect the different rules.
Default is 16.

=item NB_RULES

Integer. Upper limit of rules to be generated.
Default is 8000.

=item POLICY

String. Determines the policy to distribute the rules across the NIC queues.
Supported policies are LOAD_AWARE and ROUND_ROBIN.
Default is LOAD_AWARE.

=item KEEP_SPORT

Boolean. Encodes the source port value of each packet into the rule.
Default is false.

=item KEEP_DPORT

Boolean. Encodes the destination port value of each packet into the rule.
Default is false.

=back

=a DPDKDevice, GenerateIPFilter */

/**
 * Base class that offers IPFlow representation & storage.
 */
class GenerateIPFilter;

/**
 * Uses the base class to generate Flow Director patterns out of the traffic.
 */
class GenerateIPFlowDirector : public GenerateIPFilter {

    public:

        GenerateIPFlowDirector() CLICK_COLD;
        ~GenerateIPFlowDirector() CLICK_COLD;

        const char *class_name() const { return "GenerateIPFlowDirector"; }
        const char *port_count() const { return PORTS_1_1; }

        int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
        int initialize(ErrorHandler *) CLICK_COLD;
        void cleanup(CleanupStage) CLICK_COLD;
        void add_handlers() CLICK_COLD;

        static String read_handler(Element *handler, void *user_data);

        Packet *simple_action(Packet *p);
    #if HAVE_BATCH
        PacketBatch *simple_action_batch(PacketBatch *batch);
    #endif

    private:
        /**
         * NIC to host the rules.
         */
        uint16_t _port;
        /**
         * Number of NIC hardware queues to be used for load balancing.
         */
        uint16_t _nb_queues;
        /**
         * Load per NIC queue.
         */
        HashMap<uint16_t, uint64_t> _queue_load_map;

        /**
         * Rule generation logic.
         */
        enum QueueAllocPolicy {
            ROUND_ROBIN,
            LOAD_AWARE
        };
        QueueAllocPolicy _queue_alloc_policy;

        /**
         * Allocate a load counter per NIC queue,
         * according to the number of NIC queues
         * requested by the user.
         */
        void init_queue_load_map(uint16_t queues_nb);

        /**
         * Dumps rules to stdout (called by read handler dump).
         */
        static String dump_rules(GenerateIPFlowDirector *g);

        /**
         * Dumps load per queue to stdout (called by read handler load).
         */
        static String dump_load(GenerateIPFlowDirector *g);

        /**
         * Dumps load statistics to stdout (called by read handler stats).
         */
        static String dump_stats(GenerateIPFlowDirector *g);

        /**
         * Assign rules to NIC queues according to a policy.
         */
        static String policy_based_rule_generation(
            GenerateIPFlowDirector *g,
            const uint8_t aggregation_prefix,
            Timestamp elapsed
        );

        /**
         * Read handlers.
         */
        enum {
            h_dump,
            h_load,
            h_stats
        };

};

CLICK_ENDDECLS

#endif
