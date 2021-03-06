/*-
 * Copyright (c) 2015 Wojciech Owczarek
 * Copyright (c) 2012 The IMS Company
 *                    Vincent Bernat
 *
 * All Rights Reserved
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   snmp.c
 * @author Vincent Bernat <bernat@luffy.cx>
 * @date   Sat Jun 23 23:08:05 2012
 *
 * @brief  SNMP related functions
 */

#include "../ptpd.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

static void sendNotif(int event);

/* Hard to get header... */
extern int header_generic(struct variable *, oid *, size_t *, int,
			  size_t *, WriteMethod **);


enum {

    /* PTPBASE-MIB core */
    PTPBASE_SYSTEM_PROFILE = 1,
    PTPBASE_DOMAIN_CLOCK_PORTS_TOTAL,
    PTPBASE_SYSTEM_DOMAIN_TOTALS,
    PTPBASE_CLOCK_CURRENT_DS_STEPS_REMOVED,
    PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER,
    PTPBASE_CLOCK_CURRENT_DS_MEAN_PATH_DELAY,
    PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ID,
    PTPBASE_CLOCK_PARENT_DS_PARENT_STATS,
    PTPBASE_CLOCK_PARENT_DS_OFFSET,
    PTPBASE_CLOCK_PARENT_DS_CLOCK_PH_CH_RATE,
    PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_IDENTITY,
    PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO1,
    PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO2,
    PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_CLASS,
    PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_ACCURACY,
    PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_OFFSET,
    PTPBASE_CLOCK_DEFAULT_DS_TWO_STEP_FLAG,
    PTPBASE_CLOCK_DEFAULT_DS_CLOCK_IDENTITY,
    PTPBASE_CLOCK_DEFAULT_DS_PRIO1,
    PTPBASE_CLOCK_DEFAULT_DS_PRIO2,
    PTPBASE_CLOCK_DEFAULT_DS_SLAVE_ONLY,
    PTPBASE_CLOCK_DEFAULT_DS_QUALITY_CLASS,
    PTPBASE_CLOCK_DEFAULT_DS_QUALITY_ACCURACY,
    PTPBASE_CLOCK_DEFAULT_DS_QUALITY_OFFSET,
    PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET_VALID,
    PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET,
    PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP59,
    PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP61,
    PTPBASE_CLOCK_TIME_PROPERTIES_DS_TIME_TRACEABLE,
    PTPBASE_CLOCK_TIME_PROPERTIES_DS_FREQ_TRACEABLE,
    PTPBASE_CLOCK_TIME_PROPERTIES_DS_PTP_TIMESCALE,
    PTPBASE_CLOCK_TIME_PROPERTIES_DS_SOURCE,
    PTPBASE_CLOCK_PORT_NAME,
    PTPBASE_CLOCK_PORT_ROLE,
    PTPBASE_CLOCK_PORT_SYNC_ONE_STEP,
    PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS_TYPE,
    PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS,
    PTPBASE_CLOCK_PORT_NUM_ASSOCIATED_PORTS,
    PTPBASE_CLOCK_PORT_DS_PORT_NAME,
    PTPBASE_CLOCK_PORT_DS_PORT_IDENTITY,
    PTPBASE_CLOCK_PORT_DS_ANNOUNCEMENT_INTERVAL,
    PTPBASE_CLOCK_PORT_DS_ANNOUNCE_RCT_TIMEOUT,
    PTPBASE_CLOCK_PORT_DS_SYNC_INTERVAL,
    PTPBASE_CLOCK_PORT_DS_MIN_DELAY_REQ_INTERVAL,
    PTPBASE_CLOCK_PORT_DS_PEER_DELAY_REQ_INTERVAL,
    PTPBASE_CLOCK_PORT_DS_DELAY_MECH,
    PTPBASE_CLOCK_PORT_DS_PEER_MEAN_PATH_DELAY,
    PTPBASE_CLOCK_PORT_DS_GRANT_DURATION,
    PTPBASE_CLOCK_PORT_DS_PTP_VERSION,
    PTPBASE_CLOCK_PORT_RUNNING_NAME,
    PTPBASE_CLOCK_PORT_RUNNING_STATE,
    PTPBASE_CLOCK_PORT_RUNNING_ROLE,
    PTPBASE_CLOCK_PORT_RUNNING_INTERFACE_INDEX,
    PTPBASE_CLOCK_PORT_RUNNING_IPVERSION,
    PTPBASE_CLOCK_PORT_RUNNING_ENCAPSULATION_TYPE,
    PTPBASE_CLOCK_PORT_RUNNING_TX_MODE,
    PTPBASE_CLOCK_PORT_RUNNING_RX_MODE,
    PTPBASE_CLOCK_PORT_RUNNING_PACKETS_RECEIVED,
    PTPBASE_CLOCK_PORT_RUNNING_PACKETS_SENT,

    /* PTPd additions to PTPBASE-MIB */
    PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER_STRING,
    PTPBASE_CLOCK_CURRENT_DS_MEAN_PATH_DELAY_STRING,
    PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER_THRESHOLD,

    PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ADDRESS_TYPE,
    PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ADDRESS,

    /* ptpbasePtpPortMessageCounters */
    PTPBASE_PORT_MESSAGE_COUNTERS_CLEAR,
    PTPBASE_PORT_MESSAGE_COUNTERS_CLEAR_ALL,
    PTPBASE_PORT_MESSAGE_COUNTERS_TOTAL_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_TOTAL_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_ANNOUNCE_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_ANNOUNCE_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_SYNC_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_SYNC_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_FOLLOWUP_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_FOLLOWUP_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_DELAYREQ_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_DELAYREQ_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_DELAYRESP_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_DELAYRESP_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYREQ_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYREQ_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_FOLLOWUP_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_FOLLOWUP_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_SIGNALING_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_SIGNALING_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_MANAGEMENT_SENT,
    PTPBASE_PORT_MESSAGE_COUNTERS_MANAGEMENT_RECEIVED,
    PTPBASE_PORT_MESSAGE_COUNTERS_DISCARDED_MESSAGES,
    PTPBASE_PORT_MESSAGE_COUNTERS_UNKNOWN_MESSAGES,
    /* ptpBasePtpProtocolCounters */
    PTPBASE_PORT_PROTOCOL_COUNTERS_CLEAR,
    PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_ADDED,
    PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_COUNT,
    PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_REMOVED,
    PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_OVERFLOWS,
    PTPBASE_PORT_PROTOCOL_COUNTERS_STATE_TRANSITIONS,
    PTPBASE_PORT_PROTOCOL_COUNTERS_BEST_MASTER_CHANGES,
    PTPBASE_PORT_PROTOCOL_COUNTERS_ANNOUNCE_TIMEOUTS,
    /* ptpBasePtpErrorCounters */
    PTPBASE_PORT_ERROR_COUNTERS_CLEAR,
    PTPBASE_PORT_ERROR_COUNTERS_MESSAGE_RECV,
    PTPBASE_PORT_ERROR_COUNTERS_MESSAGE_SEND,
    PTPBASE_PORT_ERROR_COUNTERS_MESSAGE_FORMAT,
    PTPBASE_PORT_ERROR_COUNTERS_PROTOCOL,
    PTPBASE_PORT_ERROR_COUNTERS_VERSION_MISMATCH,
    PTPBASE_PORT_ERROR_COUNTERS_DOMAIN_MISMATCH,
    PTPBASE_PORT_ERROR_COUNTERS_SEQUENCE_MISMATCH,
    PTPBASE_PORT_ERROR_COUNTERS_DELAYMECH_MISMATCH,
    /* ptpBasePtpUnicastNegotiationCounters */
    PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_CLEAR,
    PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_REQUESTED,
    PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_GRANTED,
    PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_DENIED,
    PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_SENT,
    PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_RECEIVED,
    PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_ACK_SENT,
    PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_ACK_RECEIVED,
    /* ptpBasePtpPerformanceCounters */
    PTPBASE_PORT_PERFORMANCE_COUNTERS_MESSAGE_SEND_RATE,
    PTPBASE_PORT_PERFORMANCE_COUNTERS_MESSAGE_RECEIVE_RATE,
    /* ptpBasePtpSecurityCounters */
    PTPBASE_PORT_SECURITY_COUNTERS_CLEAR,
    PTPBASE_PORT_SECURITY_COUNTERS_TIMING_ACL_DISCARDED,
    PTPBASE_PORT_SECURITY_COUNTERS_MANAGEMENT_ACL_DISCARDED,
    /* ptpBasePtpdSpecificCounters */
    PTPBASE_PTPD_SPECIFIC_COUNTERS_CLEAR,
    PTPBASE_PTPD_SPECIFIC_COUNTERS_IGNORED_ANNOUNCE,
    PTPBASE_PTPD_SPECIFIC_COUNTERS_CONSECUTIVE_SEQUENCE_ERRORS,
    PTPBASE_PTPD_SPECIFIC_COUNTERS_DELAYMS_OUTLIERS_FOUND,
    PTPBASE_PTPD_SPECIFIC_COUNTERS_DELAYSM_OUTLIERS_FOUND,
    PTPBASE_PTPD_SPECIFIC_COUNTERS_MAX_DELAY_DROPS
};

/* trap / notification definitions */
enum {
	PTPBASE_NOTIFS_EXPECTED_PORT_STATE,
	PTPBASE_NOTIFS_UNEXPECTED_PORT_STATE,
	PTPBASE_NOTIFS_SLAVE_OFFSET_THRESHOLD_EXCEEDED,
	PTPBASE_NOTIFS_SLAVE_OFFSET_THRESHOLD_ACCEPTABLE,
	PTPBASE_NOTIFS_SLAVE_CLOCK_STEP,
	PTPBASE_NOTIFS_SLAVE_NO_SYNC,
	PTPBASE_NOTIFS_SLAVE_RECEIVING_SYNC,
	PTPBASE_NOTIFS_SLAVE_NO_DELAY,
	PTPBASE_NOTIFS_SLAVE_RECEIVING_DELAY,
	PTPBASE_NOTIFS_BEST_MASTER_CHANGE,
	PTPBASE_NOTIFS_NETWORK_FAULT,
	PTPBASE_NOTIFS_NETWORK_FAULT_CLEARED,
	PTPBASE_NOTIFS_FREQADJ_FAST,
	PTPBASE_NOTIFS_FREQADJ_NORMAL,
	PTPBASE_NOTIFS_OFFSET_SECONDS,
	PTPBASE_NOTIFS_OFFSET_SUB_SECONDS,
};

#define SNMP_PTP_ORDINARY_CLOCK 1
#define SNMP_PTP_CLOCK_INSTANCE 1	/* Only one instance */
#define SNMP_PTP_PORT_MASTER 1
#define SNMP_PTP_PORT_SLAVE  2
#define SNMP_IPv4 1
#define SNMP_PTP_TX_UNICAST 1
#define SNMP_PTP_TX_MULTICAST 2
#define SNMP_PTP_TX_MULTICAST_MIX 3

#define TRUTHVALUE_TRUE 1
#define TRUTHVALUE_FALSE 2

#define PTPBASE_MIB_OID \
	1, 3, 6, 1, 4, 1, 46649, 1, 1
#define PTPBASE_MIB_INDEX2 \
	snmpPtpClock->domainNumber, SNMP_PTP_CLOCK_INSTANCE
#define PTPBASE_MIB_INDEX3 \
	snmpPtpClock->domainNumber, SNMP_PTP_ORDINARY_CLOCK, SNMP_PTP_CLOCK_INSTANCE
#define PTPBASE_MIB_INDEX4 \
	snmpPtpClock->domainNumber, SNMP_PTP_ORDINARY_CLOCK, SNMP_PTP_CLOCK_INSTANCE, snmpPtpClock->portIdentity.portNumber

static oid  ptp_oid[] = { PTPBASE_MIB_OID };

static PtpClock *snmpPtpClock;
static RunTimeOpts *snmpRtOpts;

/* Helper functions to build header_*indexed_table() functions.  Those
   functions keep an internal state. They are not reentrant!
*/
/* {{{ */
struct snmpHeaderIndex {
	struct variable *vp;
	oid             *name;	 /* Requested/returned OID */
	size_t          *length; /* Length of above OID */
	int              exact;
	oid              best[MAX_OID_LEN]; /* Best OID */
	size_t           best_len;	    /* Best OID length */
	void            *entity;	    /* Best entity */
};

static int
snmpHeaderInit(struct snmpHeaderIndex *idx,
	       struct variable *vp, oid *name, size_t *length,
	       int exact, size_t *var_len, WriteMethod **write_method)
{
	/* If the requested OID name is less than OID prefix we
	   handle, adjust it to our prefix. */
        if ((snmp_oid_compare(name, *length, vp->name, vp->namelen)) < 0) {
                memcpy(name, vp->name, sizeof(oid) * vp->namelen);
                *length = vp->namelen;
        }
	/* Now, we can only handle OID matching our prefix. Those two
	   tests are not really necessary since NetSNMP won't give us
	   OID "above" our prefix. But this makes unit tests
	   easier.  */
	if (*length < vp->namelen) return 0;
	if (memcmp(name, vp->name, vp->namelen * sizeof(oid))) return 0;

	if(write_method != NULL) *write_method = 0;
	*var_len = sizeof(long);

	/* Initialize our header index structure */
	idx->vp = vp;
	idx->name = name;
	idx->length = length;
	idx->exact = exact;
	idx->best_len = 0;
	idx->entity = NULL;
	return 1;
}

static void
snmpHeaderIndexAdd(struct snmpHeaderIndex *idx,
		   oid *index, size_t len, void *entity)
{
	int      result;
	oid     *target;
	size_t   target_len;

        target = idx->name + idx->vp->namelen;
        target_len = *idx->length - idx->vp->namelen;
	if ((result = snmp_oid_compare(index, len, target, target_len)) < 0)
		return;		/* Too small. */
	if (idx->exact) {
		if (result == 0) {
			memcpy(idx->best, index, sizeof(oid) * len);
			idx->best_len = len;
			idx->entity = entity;
			return;
		}
		return;
	}
	if (result == 0)
		return;		/* Too small. */
	if (idx->best_len == 0 ||
	    (snmp_oid_compare(index, len,
			      idx->best,
			      idx->best_len) < 0)) {
		memcpy(idx->best, index, sizeof(oid) * len);
		idx->best_len = len;
		idx->entity = entity;
	}
}

static void*
snmpHeaderIndexBest(struct snmpHeaderIndex *idx)
{
	if (idx->entity == NULL)
		return NULL;
	if (idx->exact) {
		if (snmp_oid_compare(idx->name + idx->vp->namelen,
				     *idx->length - idx->vp->namelen,
				     idx->best, idx->best_len) == 0)
			return idx->entity;
		return NULL;
	}
	memcpy(idx->name + idx->vp->namelen,
	       idx->best, sizeof(oid) * idx->best_len);
	*idx->length = idx->vp->namelen + idx->best_len;
	return idx->entity;
}
/* }}} */


#define SNMP_LOCAL_VARIABLES			\
	static unsigned long long_ret;		\
	static U64 counter64_ret;		\
	static uint32_t ipaddr;			\
	Integer32 i32_ret;			\
	Integer64 bigint;			\
	struct snmpHeaderIndex idx;		\
	char tmpStr[64];			\
	(void)long_ret;				\
	(void)counter64_ret;			\
	(void)ipaddr;				\
	(void)bigint;				\
	(void)i32_ret;				\
	(void)tmpStr;				\
	(void)idx
#define SNMP_INDEXED_TABLE			\
	if (!snmpHeaderInit(&idx, vp, name,	\
			    length, exact,	\
			    var_len,		\
			    write_method))	\
		return NULL
#define SNMP_ADD_INDEX(index, len, w)		\
	snmpHeaderIndexAdd(&idx, index, len, w)
#define SNMP_BEST_MATCH				\
	snmpHeaderIndexBest(&idx)
#define SNMP_OCTETSTR(V, L)			\
	( *var_len = L,				\
	  (u_char *)V )
#define SNMP_COUNTER64(V)			\
	( counter64_ret.low = (V) & 0xffffffff,	\
	  counter64_ret.high = (V) >> 32,	\
	  *var_len = sizeof (counter64_ret),	\
	  (u_char*)&counter64_ret )
#define SNMP_TIMEINTERNAL(V)				\
	( *var_len = sizeof(counter64_ret),		\
	  internalTime_to_integer64(V, &bigint),	\
	  counter64_ret.low = htonl(bigint.lsb),	\
	  counter64_ret.high = htonl(bigint.msb),      	\
	  (u_char *)&counter64_ret )
#define SNMP_INTEGER(V)		    \
	( long_ret = (V),	    \
	  *var_len = sizeof (long_ret),		\
	  (u_char*)&long_ret )
#define SNMP_IPADDR(A)				\
	( ipaddr = A,				\
	  *var_len = sizeof (ipaddr),		\
	  (u_char*)&ipaddr )
#define SNMP_GAUGE SNMP_INTEGER
#define SNMP_UNSIGNED SNMP_INTEGER
#define SNMP_TRUE SNMP_INTEGER(1)
#define SNMP_FALSE SNMP_INTEGER(2)
#define SNMP_BOOLEAN(V)				\
	(V == TRUE)?SNMP_TRUE:SNMP_FALSE
#define SNMP_SIGNATURE struct variable *vp,	\
		oid *name,			\
		size_t *length,			\
		int exact,			\
		size_t *var_len,		\
		WriteMethod **write_method

/**
 * Handle SNMP scalar values.
 */
static u_char*
snmpScalars(SNMP_SIGNATURE) {
    SNMP_LOCAL_VARIABLES;
    if (header_generic(vp, name, length, exact, var_len, write_method))
	    return NULL;

    switch (vp->magic) {
    case PTPBASE_SYSTEM_PROFILE:
	    return SNMP_INTEGER(1);
    }

    return NULL;
}

/**
 * Handle ptpbaseSystemTable
 */
static u_char*
snmpSystemTable(SNMP_SIGNATURE) {
	oid index[2];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one index: one domain, one instance */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_CLOCK_INSTANCE;
	SNMP_ADD_INDEX(index, 2, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
	case PTPBASE_DOMAIN_CLOCK_PORTS_TOTAL:
		return SNMP_GAUGE(snmpPtpClock->numberPorts);
	}

	return NULL;
}

/**
 * Handle ptpbaseSystemDomainTable
 */
static u_char*
snmpSystemDomainTable(SNMP_SIGNATURE) {
	oid index[1];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one index: ordinary clock */
	index[0] = SNMP_PTP_ORDINARY_CLOCK;
	SNMP_ADD_INDEX(index, 1, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
	case PTPBASE_SYSTEM_DOMAIN_TOTALS:
		/* We only handle one domain... */
		return SNMP_UNSIGNED(1);
	}

	return NULL;
}

/**
 * Handle various ptpbaseClock*DSTable
 */
static u_char*
snmpClockDSTable(SNMP_SIGNATURE) {
	oid index[3];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	memset(tmpStr, 0, sizeof(tmpStr));

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	SNMP_ADD_INDEX(index, 3, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
	/* ptpbaseClockCurrentDSTable */
	case PTPBASE_CLOCK_CURRENT_DS_STEPS_REMOVED:
		return SNMP_UNSIGNED(snmpPtpClock->stepsRemoved);
	case PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER:
		return SNMP_TIMEINTERNAL(snmpPtpClock->offsetFromMaster);
	case PTPBASE_CLOCK_CURRENT_DS_MEAN_PATH_DELAY:
		return SNMP_TIMEINTERNAL(snmpPtpClock->meanPathDelay);
	/* PTPd: offsets as string */
	case PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER_STRING:
		snprintf(tmpStr, 64, "%.09f", timeInternalToDouble(&snmpPtpClock->offsetFromMaster));
		return SNMP_OCTETSTR(&tmpStr, strlen(tmpStr));
	case PTPBASE_CLOCK_CURRENT_DS_MEAN_PATH_DELAY_STRING:
		snprintf(tmpStr, 64, "%.09f", timeInternalToDouble(&snmpPtpClock->meanPathDelay));
		return SNMP_OCTETSTR(&tmpStr, strlen(tmpStr));
	case PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER_THRESHOLD:
		return SNMP_INTEGER(snmpRtOpts->ofmAlarmThreshold);

	/* ptpbaseClockParentDSTable */
	case PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ID:
		return SNMP_OCTETSTR(&snmpPtpClock->parentPortIdentity,
				     sizeof(PortIdentity));
	case PTPBASE_CLOCK_PARENT_DS_PARENT_STATS:
		return SNMP_BOOLEAN(snmpPtpClock->parentStats);
	case PTPBASE_CLOCK_PARENT_DS_OFFSET:
		return SNMP_INTEGER(snmpPtpClock->observedParentOffsetScaledLogVariance);
	case PTPBASE_CLOCK_PARENT_DS_CLOCK_PH_CH_RATE:
		return SNMP_INTEGER(snmpPtpClock->observedParentClockPhaseChangeRate);
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_IDENTITY:
		return SNMP_OCTETSTR(&snmpPtpClock->grandmasterIdentity,
				     sizeof(ClockIdentity));
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO1:
		return SNMP_UNSIGNED(snmpPtpClock->grandmasterPriority1);
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO2:
		return SNMP_UNSIGNED(snmpPtpClock->grandmasterPriority2);
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_CLASS:
		return SNMP_UNSIGNED(snmpPtpClock->grandmasterClockQuality.clockClass);
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_ACCURACY:
		return SNMP_INTEGER(snmpPtpClock->grandmasterClockQuality.clockAccuracy);
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_OFFSET:
		return SNMP_INTEGER(snmpPtpClock->grandmasterClockQuality.offsetScaledLogVariance);
	/* PTPd addition */
	case PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ADDRESS_TYPE:
		/* Only supports IPv4 */
		return SNMP_INTEGER(SNMP_IPv4);
	case PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ADDRESS:
		if(snmpRtOpts->transport != UDP_IPV4)
		    return SNMP_IPADDR(0);
		if(!snmpPtpClock->bestMaster)
		    return SNMP_IPADDR(0);
		return SNMP_IPADDR(snmpPtpClock->bestMaster->sourceAddr);
	/* ptpbaseClockDefaultDSTable */
	case PTPBASE_CLOCK_DEFAULT_DS_TWO_STEP_FLAG:
		return SNMP_BOOLEAN(snmpPtpClock->twoStepFlag);
	case PTPBASE_CLOCK_DEFAULT_DS_CLOCK_IDENTITY:
		return SNMP_OCTETSTR(&snmpPtpClock->clockIdentity,
				     sizeof(ClockIdentity));
	case PTPBASE_CLOCK_DEFAULT_DS_PRIO1:
		return SNMP_UNSIGNED(snmpPtpClock->priority1);
	case PTPBASE_CLOCK_DEFAULT_DS_PRIO2:
		return SNMP_UNSIGNED(snmpPtpClock->priority2);
	case PTPBASE_CLOCK_DEFAULT_DS_SLAVE_ONLY:
		return SNMP_BOOLEAN(snmpPtpClock->slaveOnly);
	case PTPBASE_CLOCK_DEFAULT_DS_QUALITY_CLASS:
		return SNMP_UNSIGNED(snmpPtpClock->clockQuality.clockClass);
	case PTPBASE_CLOCK_DEFAULT_DS_QUALITY_ACCURACY:
		return SNMP_INTEGER(snmpPtpClock->clockQuality.clockAccuracy);
	case PTPBASE_CLOCK_DEFAULT_DS_QUALITY_OFFSET:
		return SNMP_INTEGER(snmpPtpClock->clockQuality.offsetScaledLogVariance);
	/* ptpbaseClockTimePropertiesDSTable */
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET_VALID:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.currentUtcOffsetValid);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET:
		return SNMP_INTEGER(snmpPtpClock->timePropertiesDS.currentUtcOffset);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP59:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.leap59);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP61:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.leap61);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_TIME_TRACEABLE:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.timeTraceable);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_FREQ_TRACEABLE:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.frequencyTraceable);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_PTP_TIMESCALE:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.ptpTimescale);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_SOURCE:
		return SNMP_INTEGER(snmpPtpClock->timePropertiesDS.timeSource);
	}

	return NULL;
}

/**
 * Handle ptpbaseClockPort*Table
 */
static u_char*
snmpClockPortTable(SNMP_SIGNATURE) {
	oid index[4];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	index[3] = snmpPtpClock->portIdentity.portNumber;
	SNMP_ADD_INDEX(index, 4, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
	/* ptpbaseClockPortTable */
	case PTPBASE_CLOCK_PORT_NAME:
	case PTPBASE_CLOCK_PORT_DS_PORT_NAME:
	case PTPBASE_CLOCK_PORT_RUNNING_NAME:
		return SNMP_OCTETSTR(snmpPtpClock->userDescription,
				     strlen(snmpPtpClock->userDescription));
	case PTPBASE_CLOCK_PORT_ROLE:
		return SNMP_INTEGER((snmpPtpClock->portState == PTP_MASTER)?
				    SNMP_PTP_PORT_MASTER:SNMP_PTP_PORT_SLAVE);
	case PTPBASE_CLOCK_PORT_SYNC_ONE_STEP:
		return (snmpPtpClock->twoStepFlag == TRUE)?SNMP_FALSE:SNMP_TRUE;
	case PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS_TYPE:
		/* Only supports IPv4 */
		return SNMP_INTEGER(SNMP_IPv4);
	case PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS:
		if(snmpRtOpts->transport != UDP_IPV4)
		    return SNMP_IPADDR(0);
		return(SNMP_IPADDR(snmpPtpClock->netPath.interfaceAddr.s_addr));
	case PTPBASE_CLOCK_PORT_NUM_ASSOCIATED_PORTS:
		/* Either we are master and we use multicast and we
		 * consider we have a session or we are slave and we
		 * have only one master. */
		return SNMP_INTEGER(1);

	/* ptpbaseClockPortDSTable */
	case PTPBASE_CLOCK_PORT_DS_PORT_IDENTITY:
		return SNMP_OCTETSTR(&snmpPtpClock->portIdentity,
				     sizeof(PortIdentity));
	case PTPBASE_CLOCK_PORT_DS_ANNOUNCEMENT_INTERVAL:
		/* TODO: is it really logAnnounceInterval? */
		return SNMP_INTEGER(snmpPtpClock->logAnnounceInterval);
	case PTPBASE_CLOCK_PORT_DS_ANNOUNCE_RCT_TIMEOUT:
		return SNMP_INTEGER(snmpPtpClock->announceReceiptTimeout);
	case PTPBASE_CLOCK_PORT_DS_SYNC_INTERVAL:
		/* TODO: is it really logSyncInterval? */
		return SNMP_INTEGER(snmpPtpClock->logSyncInterval);
	case PTPBASE_CLOCK_PORT_DS_MIN_DELAY_REQ_INTERVAL:
		/* TODO: is it really logMinDelayReqInterval? */
		return SNMP_INTEGER(snmpPtpClock->logMinDelayReqInterval);
	case PTPBASE_CLOCK_PORT_DS_PEER_DELAY_REQ_INTERVAL:
		/* TODO: is it really logMinPdelayReqInterval? */
		return SNMP_INTEGER(snmpPtpClock->logMinPdelayReqInterval);
	case PTPBASE_CLOCK_PORT_DS_DELAY_MECH:
		return SNMP_INTEGER(snmpPtpClock->delayMechanism);
	case PTPBASE_CLOCK_PORT_DS_PEER_MEAN_PATH_DELAY:
		return SNMP_TIMEINTERNAL(snmpPtpClock->peerMeanPathDelay);
	case PTPBASE_CLOCK_PORT_DS_GRANT_DURATION:
		if(snmpRtOpts->unicastNegotiation && snmpPtpClock->parentGrants) {
			return SNMP_UNSIGNED(snmpPtpClock->parentGrants->grantData[SYNC_INDEXED].duration);
		}
		return SNMP_UNSIGNED(0);
	case PTPBASE_CLOCK_PORT_DS_PTP_VERSION:
		return SNMP_INTEGER(snmpPtpClock->versionNumber);

	/* ptpbaseClockPortRunningTable */
	case PTPBASE_CLOCK_PORT_RUNNING_STATE:
		return SNMP_INTEGER(snmpPtpClock->portState);
	case PTPBASE_CLOCK_PORT_RUNNING_ROLE:
		return SNMP_INTEGER((snmpPtpClock->portState == PTP_MASTER)?
				    SNMP_PTP_PORT_MASTER:SNMP_PTP_PORT_SLAVE);
	case PTPBASE_CLOCK_PORT_RUNNING_INTERFACE_INDEX:
		/* TODO: maybe we can have it from the general configuration? */
		return SNMP_INTEGER(0);
	case PTPBASE_CLOCK_PORT_RUNNING_IPVERSION:
		/* IPv4 only */
		/* TODO: shouldn't we return SNMP_IPv4??? */
		return SNMP_INTEGER(4);
	case PTPBASE_CLOCK_PORT_RUNNING_ENCAPSULATION_TYPE:
		/* None. Moreover, the format is not really described in the MIB... */
		return SNMP_INTEGER(0);
	case PTPBASE_CLOCK_PORT_RUNNING_TX_MODE:
	case PTPBASE_CLOCK_PORT_RUNNING_RX_MODE:	
		if (snmpRtOpts->ipMode == IPMODE_UNICAST)
			return SNMP_INTEGER(SNMP_PTP_TX_UNICAST);
		if (snmpRtOpts->ipMode == IPMODE_HYBRID)
			return SNMP_INTEGER(SNMP_PTP_TX_MULTICAST_MIX);
		return SNMP_INTEGER(SNMP_PTP_TX_MULTICAST);
	case PTPBASE_CLOCK_PORT_RUNNING_PACKETS_RECEIVED:
		return SNMP_COUNTER64(snmpPtpClock->netPath.receivedPacketsTotal);
	case PTPBASE_CLOCK_PORT_RUNNING_PACKETS_SENT:
		return SNMP_COUNTER64(snmpPtpClock->netPath.sentPacketsTotal);
	}


	return NULL;
}

/* clear counter sets based on oid. WARNING: USES MAGIC NUMBERS... */
static int
snmpWriteClearCounters (int action, u_char *var_val, u_char var_val_type, size_t var_val_len,
			    u_char *statP, oid *name, size_t name_len) {

	/* table: 6 oids from end (index fields, entry, field) */
	oid myOid1 = name[name_len - 1 - 6];
	/* field: 4 oids from end (index fields) */
	oid myOid2 = name[name_len - 1 - 4];

	if(var_val_type != ASN_INTEGER) {
	    return SNMP_ERR_WRONGTYPE;
	}

	if(var_val_len > sizeof(long)) {
	    return SNMP_ERR_WRONGLENGTH;
	}

	if(action==COMMIT) {

	    long *val = (long*) var_val;

	    if (*val == TRUTHVALUE_TRUE) {

		switch (myOid1) {

		    case 12: /* message counters */
				/* all counters */
				if(myOid2 == 5) {
					memset(&snmpPtpClock->counters, 0, sizeof(PtpdCounters));
					return SNMP_ERR_NOERROR;
				}
				/* message counters */
				if(myOid2 == 6) {
					snmpPtpClock->counters.announceMessagesSent = 0;
					snmpPtpClock->counters.announceMessagesReceived = 0;
					snmpPtpClock->counters.syncMessagesSent = 0;
					snmpPtpClock->counters.syncMessagesReceived = 0;
					snmpPtpClock->counters.followUpMessagesSent = 0;
					snmpPtpClock->counters.followUpMessagesReceived = 0;
					snmpPtpClock->counters.delayReqMessagesSent = 0;
					snmpPtpClock->counters.delayReqMessagesReceived = 0;
					snmpPtpClock->counters.delayRespMessagesSent = 0;
					snmpPtpClock->counters.delayRespMessagesReceived = 0;
					snmpPtpClock->counters.pdelayReqMessagesSent = 0;
					snmpPtpClock->counters.pdelayReqMessagesReceived = 0;
					snmpPtpClock->counters.pdelayRespMessagesSent = 0;
					snmpPtpClock->counters.pdelayRespMessagesReceived = 0;
					snmpPtpClock->counters.pdelayRespFollowUpMessagesSent = 0;
					snmpPtpClock->counters.pdelayRespFollowUpMessagesReceived = 0;
					snmpPtpClock->counters.signalingMessagesSent = 0;
					snmpPtpClock->counters.signalingMessagesReceived = 0;
					snmpPtpClock->counters.managementMessagesSent = 0;
					snmpPtpClock->counters.managementMessagesReceived = 0;
					snmpPtpClock->counters.discardedMessages = 0;
					snmpPtpClock->counters.unknownMessages = 0;
					return SNMP_ERR_NOERROR;
				}
			break;
		    case 13: /* protocol counters */
				/* clear counters */
				if(myOid2 == 5) {
					snmpPtpClock->counters.foreignAdded = 0;
					/* snmpPtpClock->counters.foreignCount = 0; */ /* we don't clear this */
					snmpPtpClock->counters.foreignRemoved = 0;
					snmpPtpClock->counters.foreignOverflows = 0;
					snmpPtpClock->counters.stateTransitions = 0;
					snmpPtpClock->counters.bestMasterChanges = 0;
					snmpPtpClock->counters.announceTimeouts = 0;
					return SNMP_ERR_NOERROR;
				}
			break;
		    case 14: /* error counters */
				/* clear counters */
				if(myOid2 == 5) {
					snmpPtpClock->counters.messageRecvErrors = 0;
					snmpPtpClock->counters.messageSendErrors = 0;
					snmpPtpClock->counters.messageFormatErrors = 0;
					snmpPtpClock->counters.protocolErrors = 0;
					snmpPtpClock->counters.versionMismatchErrors = 0;
					snmpPtpClock->counters.domainMismatchErrors = 0;
					snmpPtpClock->counters.sequenceMismatchErrors = 0;
					snmpPtpClock->counters.delayMechanismMismatchErrors = 0;
					return SNMP_ERR_NOERROR;
				}
			break;
		    case 15: /* unicast negotiation counters */
				/* clear counters */
				if(myOid2 == 5) {
					snmpPtpClock->counters.unicastGrantsRequested = 0;
					snmpPtpClock->counters.unicastGrantsGranted = 0;
					snmpPtpClock->counters.unicastGrantsDenied = 0;
					snmpPtpClock->counters.unicastGrantsCancelSent = 0;
					snmpPtpClock->counters.unicastGrantsCancelReceived = 0;
					snmpPtpClock->counters.unicastGrantsCancelAckSent = 0;
					snmpPtpClock->counters.unicastGrantsCancelAckReceived = 0;
					return SNMP_ERR_NOERROR;
				}
			break;
		    case 17: /* security counters */
				/* clear counters */
				if(myOid2 == 5) {
					snmpPtpClock->counters.aclTimingMessagesDiscarded = 0;
					snmpPtpClock->counters.aclManagementMessagesDiscarded = 0;
					return SNMP_ERR_NOERROR;
				}
			break;
		    case 21: /* ptpd counters */
				/* clear counters */
				if(myOid2 == 5) {
					snmpPtpClock->counters.consecutiveSequenceErrors = 0;
					snmpPtpClock->counters.ignoredAnnounce = 0;
					snmpPtpClock->counters.delayMSOutliersFound = 0;
					snmpPtpClock->counters.delaySMOutliersFound = 0;
					snmpPtpClock->counters.maxDelayDrops = 0;
					return SNMP_ERR_NOERROR;
				}
			break;
		    default:
			return SNMP_ERR_WRONGVALUE;
		}
		return SNMP_ERR_WRONGVALUE;
	    }

	    return SNMP_ERR_WRONGVALUE;

	}

	return SNMP_ERR_NOERROR;

}

/**
 * Handle ptpbasePtpPortMessageCounters
 */
static u_char*
snmpPtpPortMessageCountersTable(SNMP_SIGNATURE) {
	oid index[4];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	index[3] = snmpPtpClock->portIdentity.portNumber;
	SNMP_ADD_INDEX(index, 4, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
	case PTPBASE_PORT_MESSAGE_COUNTERS_CLEAR:
	    *write_method = snmpWriteClearCounters;
	    return SNMP_FALSE;
	case PTPBASE_PORT_MESSAGE_COUNTERS_CLEAR_ALL:
	    *write_method = snmpWriteClearCounters;
	    return SNMP_FALSE;
	case PTPBASE_PORT_MESSAGE_COUNTERS_TOTAL_SENT:
	    return SNMP_INTEGER(0);
	case PTPBASE_PORT_MESSAGE_COUNTERS_TOTAL_RECEIVED:
	    return SNMP_INTEGER(0);
	case PTPBASE_PORT_MESSAGE_COUNTERS_ANNOUNCE_SENT:
	    return SNMP_INTEGER(snmpPtpClock->counters.announceMessagesSent);
	case PTPBASE_PORT_MESSAGE_COUNTERS_ANNOUNCE_RECEIVED:
	    return SNMP_INTEGER(snmpPtpClock->counters.announceMessagesReceived);
	case PTPBASE_PORT_MESSAGE_COUNTERS_SYNC_SENT:
	    return SNMP_INTEGER(snmpPtpClock->counters.syncMessagesSent);
	case PTPBASE_PORT_MESSAGE_COUNTERS_SYNC_RECEIVED:
	    return SNMP_INTEGER(snmpPtpClock->counters.syncMessagesReceived);
	case PTPBASE_PORT_MESSAGE_COUNTERS_FOLLOWUP_SENT:
	    return SNMP_INTEGER(snmpPtpClock->counters.followUpMessagesSent);
	case PTPBASE_PORT_MESSAGE_COUNTERS_FOLLOWUP_RECEIVED:
	    return SNMP_INTEGER(snmpPtpClock->counters.followUpMessagesReceived);
	case PTPBASE_PORT_MESSAGE_COUNTERS_DELAYREQ_SENT:
	    return SNMP_INTEGER(snmpPtpClock->counters.delayReqMessagesSent);
	case PTPBASE_PORT_MESSAGE_COUNTERS_DELAYREQ_RECEIVED:
	    return SNMP_INTEGER(snmpPtpClock->counters.delayReqMessagesReceived);
	case PTPBASE_PORT_MESSAGE_COUNTERS_DELAYRESP_SENT:
	    return SNMP_INTEGER(snmpPtpClock->counters.delayRespMessagesSent);
	case PTPBASE_PORT_MESSAGE_COUNTERS_DELAYRESP_RECEIVED:
	    return SNMP_INTEGER(snmpPtpClock->counters.delayRespMessagesReceived);
	case PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYREQ_SENT:
	    return SNMP_INTEGER(snmpPtpClock->counters.pdelayReqMessagesSent);
	case PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYREQ_RECEIVED:
	    return SNMP_INTEGER(snmpPtpClock->counters.pdelayReqMessagesReceived);
	case PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_SENT:
	    return SNMP_INTEGER(snmpPtpClock->counters.pdelayRespMessagesSent);
	case PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_RECEIVED:
	    return SNMP_INTEGER(snmpPtpClock->counters.pdelayRespMessagesReceived);
	case PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_FOLLOWUP_SENT:
	    return SNMP_INTEGER(snmpPtpClock->counters.pdelayRespFollowUpMessagesSent);
	case PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_FOLLOWUP_RECEIVED:
	    return SNMP_INTEGER(snmpPtpClock->counters.pdelayRespFollowUpMessagesReceived);
	case PTPBASE_PORT_MESSAGE_COUNTERS_SIGNALING_SENT:
	    return SNMP_INTEGER(snmpPtpClock->counters.signalingMessagesSent);
	case PTPBASE_PORT_MESSAGE_COUNTERS_SIGNALING_RECEIVED:
	    return SNMP_INTEGER(snmpPtpClock->counters.signalingMessagesReceived);
	case PTPBASE_PORT_MESSAGE_COUNTERS_MANAGEMENT_SENT:
	    return SNMP_INTEGER(snmpPtpClock->counters.managementMessagesSent);
	case PTPBASE_PORT_MESSAGE_COUNTERS_MANAGEMENT_RECEIVED:
	    return SNMP_INTEGER(snmpPtpClock->counters.managementMessagesReceived);
	case PTPBASE_PORT_MESSAGE_COUNTERS_DISCARDED_MESSAGES:
	    return SNMP_INTEGER(snmpPtpClock->counters.discardedMessages);
	case PTPBASE_PORT_MESSAGE_COUNTERS_UNKNOWN_MESSAGES:
	    return SNMP_INTEGER(snmpPtpClock->counters.unknownMessages);

    }

	return NULL;
}


/**
 * Handle ptpbasePtpPortProtocolCounters
 */
static u_char*
snmpPtpPortProtocolCountersTable(SNMP_SIGNATURE) {
	oid index[4];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	index[3] = snmpPtpClock->portIdentity.portNumber;
	SNMP_ADD_INDEX(index, 4, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
    case PTPBASE_PORT_PROTOCOL_COUNTERS_CLEAR:
	    *write_method = snmpWriteClearCounters;
	    return SNMP_FALSE;
    case PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_ADDED:
	return SNMP_INTEGER(snmpPtpClock->counters.foreignAdded);
    case PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_COUNT:
	return SNMP_INTEGER(snmpPtpClock->counters.foreignCount);
    case PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_REMOVED:
	return SNMP_INTEGER(snmpPtpClock->counters.foreignRemoved);
    case PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_OVERFLOWS:
	return SNMP_INTEGER(snmpPtpClock->counters.foreignOverflows);
    case PTPBASE_PORT_PROTOCOL_COUNTERS_STATE_TRANSITIONS:
	return SNMP_INTEGER(snmpPtpClock->counters.stateTransitions);
    case PTPBASE_PORT_PROTOCOL_COUNTERS_BEST_MASTER_CHANGES:
	return SNMP_INTEGER(snmpPtpClock->counters.bestMasterChanges);
    case PTPBASE_PORT_PROTOCOL_COUNTERS_ANNOUNCE_TIMEOUTS:
	return SNMP_INTEGER(snmpPtpClock->counters.announceTimeouts);
	}

	return NULL;
}

/**
 * Handle ptpbasePtpPortErrorCounters
 */
static u_char*
snmpPtpPortErrorCountersTable(SNMP_SIGNATURE) {
	oid index[4];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	index[3] = snmpPtpClock->portIdentity.portNumber;
	SNMP_ADD_INDEX(index, 4, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
    case PTPBASE_PORT_ERROR_COUNTERS_CLEAR:
	    *write_method = snmpWriteClearCounters;
	    return SNMP_FALSE;
    case PTPBASE_PORT_ERROR_COUNTERS_MESSAGE_RECV:
	return SNMP_INTEGER(snmpPtpClock->counters.messageRecvErrors);
    case PTPBASE_PORT_ERROR_COUNTERS_MESSAGE_SEND:
	return SNMP_INTEGER(snmpPtpClock->counters.messageSendErrors);
    case PTPBASE_PORT_ERROR_COUNTERS_MESSAGE_FORMAT:
	return SNMP_INTEGER(snmpPtpClock->counters.messageFormatErrors);
    case PTPBASE_PORT_ERROR_COUNTERS_PROTOCOL:
	return SNMP_INTEGER(snmpPtpClock->counters.protocolErrors);
    case PTPBASE_PORT_ERROR_COUNTERS_VERSION_MISMATCH:
	return SNMP_INTEGER(snmpPtpClock->counters.versionMismatchErrors);
    case PTPBASE_PORT_ERROR_COUNTERS_DOMAIN_MISMATCH:
	return SNMP_INTEGER(snmpPtpClock->counters.domainMismatchErrors);
    case PTPBASE_PORT_ERROR_COUNTERS_SEQUENCE_MISMATCH:
	return SNMP_INTEGER(snmpPtpClock->counters.sequenceMismatchErrors);
    case PTPBASE_PORT_ERROR_COUNTERS_DELAYMECH_MISMATCH:
	return SNMP_INTEGER(snmpPtpClock->counters.delayMechanismMismatchErrors);
	}

	return NULL;
}

/**
 * Handle ptpbasePtpPortUnicastNegotiationCounters
 */
static u_char*
snmpPtpPortUnicastNegotiationCountersTable(SNMP_SIGNATURE) {
	oid index[4];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	index[3] = snmpPtpClock->portIdentity.portNumber;
	SNMP_ADD_INDEX(index, 4, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
    case PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_CLEAR:
	    *write_method = snmpWriteClearCounters;
	    return SNMP_FALSE;
    case PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_REQUESTED:
	return SNMP_INTEGER(snmpPtpClock->counters.unicastGrantsRequested);
    case PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_GRANTED:
	return SNMP_INTEGER(snmpPtpClock->counters.unicastGrantsGranted);
    case PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_DENIED:
	return SNMP_INTEGER(snmpPtpClock->counters.unicastGrantsDenied);
    case PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_SENT:
	return SNMP_INTEGER(snmpPtpClock->counters.unicastGrantsCancelSent);
    case PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_RECEIVED:
	return SNMP_INTEGER(snmpPtpClock->counters.unicastGrantsCancelReceived);
    case PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_ACK_SENT:
	return SNMP_INTEGER(snmpPtpClock->counters.unicastGrantsCancelAckSent);
    case PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_ACK_RECEIVED:
	return SNMP_INTEGER(snmpPtpClock->counters.unicastGrantsCancelAckReceived);
	}

	return NULL;
}

/**
 * Handle ptpbasePtpPortPerformanceCounters
 */
static u_char*
snmpPtpPortPerformanceCountersTable(SNMP_SIGNATURE) {
	oid index[4];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	index[3] = snmpPtpClock->portIdentity.portNumber;
	SNMP_ADD_INDEX(index, 4, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
    case PTPBASE_PORT_PERFORMANCE_COUNTERS_MESSAGE_SEND_RATE:
	return SNMP_INTEGER(snmpPtpClock->counters.messageSendRate);
    case PTPBASE_PORT_PERFORMANCE_COUNTERS_MESSAGE_RECEIVE_RATE:
	return SNMP_INTEGER(snmpPtpClock->counters.messageReceiveRate);
	}

	return NULL;
}


/**
 * Handle ptpbasePtpPortSecurityCounters
 */
static u_char*
snmpPtpPortSecurityCountersTable(SNMP_SIGNATURE) {
	oid index[4];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	index[3] = snmpPtpClock->portIdentity.portNumber;
	SNMP_ADD_INDEX(index, 4, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
    case PTPBASE_PORT_SECURITY_COUNTERS_CLEAR:
	    *write_method = snmpWriteClearCounters;
	    return SNMP_FALSE;
    case PTPBASE_PORT_SECURITY_COUNTERS_TIMING_ACL_DISCARDED:
	return SNMP_INTEGER(snmpPtpClock->counters.aclTimingMessagesDiscarded);
    case PTPBASE_PORT_SECURITY_COUNTERS_MANAGEMENT_ACL_DISCARDED:
	return SNMP_INTEGER(snmpPtpClock->counters.aclManagementMessagesDiscarded);
	}

	return NULL;
}

/**
 * Handle ptpbasePtpdSpecificCounters
 */
static u_char*
snmpPtpdSpecificCountersTable(SNMP_SIGNATURE) {
	oid index[4];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	index[3] = snmpPtpClock->portIdentity.portNumber;
	SNMP_ADD_INDEX(index, 4, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
    case PTPBASE_PTPD_SPECIFIC_COUNTERS_CLEAR:
	    *write_method = snmpWriteClearCounters;
	    return SNMP_FALSE;
    case PTPBASE_PTPD_SPECIFIC_COUNTERS_IGNORED_ANNOUNCE:
	return SNMP_INTEGER(snmpPtpClock->counters.ignoredAnnounce);
    case PTPBASE_PTPD_SPECIFIC_COUNTERS_CONSECUTIVE_SEQUENCE_ERRORS:
	return SNMP_INTEGER(snmpPtpClock->counters.consecutiveSequenceErrors);
    case PTPBASE_PTPD_SPECIFIC_COUNTERS_DELAYMS_OUTLIERS_FOUND:
	return SNMP_INTEGER(snmpPtpClock->counters.delayMSOutliersFound);
    case PTPBASE_PTPD_SPECIFIC_COUNTERS_DELAYSM_OUTLIERS_FOUND:
	return SNMP_INTEGER(snmpPtpClock->counters.delaySMOutliersFound);
    case PTPBASE_PTPD_SPECIFIC_COUNTERS_MAX_DELAY_DROPS:
	return SNMP_INTEGER(snmpPtpClock->counters.maxDelayDrops);
	}

	return NULL;
}


/**
 * MIB definition
 */
static struct variable7 snmpVariables[] = {
	/* ptpbaseSystemTable */
	{ PTPBASE_DOMAIN_CLOCK_PORTS_TOTAL, ASN_GAUGE, HANDLER_CAN_RONLY,
	  snmpSystemTable, 5, {1, 1, 1, 1, 3}},
	/* ptpbaseSystemDomainTable */
	{ PTPBASE_SYSTEM_DOMAIN_TOTALS, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpSystemDomainTable, 5, {1, 1, 2, 1, 2}},
	/* Scalars */
	{ PTPBASE_SYSTEM_PROFILE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpScalars, 3, {1, 1, 3}},
	/* ptpbaseClockCurrentDSTable */
	{ PTPBASE_CLOCK_CURRENT_DS_STEPS_REMOVED, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 1, 1, 4}},
	{ PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 1, 1, 5}},
	{ PTPBASE_CLOCK_CURRENT_DS_MEAN_PATH_DELAY, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 1, 1, 6}},
	/* PTPd enhancements */
	{ PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER_STRING, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 1, 1, 7}},
	{ PTPBASE_CLOCK_CURRENT_DS_MEAN_PATH_DELAY_STRING, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 1, 1, 8}},
	{ PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER_THRESHOLD, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 1, 1, 9}},

	/* ptpbaseClockParentDSTable */
	{ PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ID, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 4}},
	{ PTPBASE_CLOCK_PARENT_DS_PARENT_STATS, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 5}},
	{ PTPBASE_CLOCK_PARENT_DS_OFFSET, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 6}},
	{ PTPBASE_CLOCK_PARENT_DS_CLOCK_PH_CH_RATE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 7}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_IDENTITY, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 8}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO1, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 9}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO2, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 10}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_CLASS, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 11}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_ACCURACY, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 12}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_OFFSET, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 13}},

	/* PTPd enhancements */
	{ PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ADDRESS_TYPE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 14}},
	{ PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ADDRESS, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 15}},

	/* ptpbaseClockDefaultDSTable */
	{ PTPBASE_CLOCK_DEFAULT_DS_TWO_STEP_FLAG, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 4}},
	{ PTPBASE_CLOCK_DEFAULT_DS_CLOCK_IDENTITY, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 5}},
	{ PTPBASE_CLOCK_DEFAULT_DS_PRIO1, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 6}},
	{ PTPBASE_CLOCK_DEFAULT_DS_PRIO2, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 7}},
	{ PTPBASE_CLOCK_DEFAULT_DS_SLAVE_ONLY, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 8}},
	{ PTPBASE_CLOCK_DEFAULT_DS_QUALITY_CLASS, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 9}},
	{ PTPBASE_CLOCK_DEFAULT_DS_QUALITY_ACCURACY, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 10}},
	{ PTPBASE_CLOCK_DEFAULT_DS_QUALITY_OFFSET, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 11}},
	/* ptpbaseClockRunningTable: no data available for this table? */
	/* ptpbaseClockTimePropertiesDSTable */
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET_VALID, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 4}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 5}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP59, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 6}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP61, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 7}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_TIME_TRACEABLE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 8}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_FREQ_TRACEABLE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 9}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_PTP_TIMESCALE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 10}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_SOURCE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 11}},
	/* ptpbaseClockPortTable */
	{ PTPBASE_CLOCK_PORT_NAME, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 5}},
	{ PTPBASE_CLOCK_PORT_ROLE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 6}},
	{ PTPBASE_CLOCK_PORT_SYNC_ONE_STEP, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 7}},
	{ PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS_TYPE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 8}},
	{ PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 9}},
	{ PTPBASE_CLOCK_PORT_NUM_ASSOCIATED_PORTS, ASN_GAUGE, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 10}},
	/* ptpbaseClockPortDSTable */
	{ PTPBASE_CLOCK_PORT_DS_PORT_NAME, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 5}},
	{ PTPBASE_CLOCK_PORT_DS_PORT_IDENTITY, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 6}},
	{ PTPBASE_CLOCK_PORT_DS_ANNOUNCEMENT_INTERVAL, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 7}},
	{ PTPBASE_CLOCK_PORT_DS_ANNOUNCE_RCT_TIMEOUT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 8}},
	{ PTPBASE_CLOCK_PORT_DS_SYNC_INTERVAL, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 9}},
	{ PTPBASE_CLOCK_PORT_DS_MIN_DELAY_REQ_INTERVAL, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 10}},
	{ PTPBASE_CLOCK_PORT_DS_PEER_DELAY_REQ_INTERVAL, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 11}},
	{ PTPBASE_CLOCK_PORT_DS_DELAY_MECH, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 12}},
	{ PTPBASE_CLOCK_PORT_DS_PEER_MEAN_PATH_DELAY, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 13}},
	{ PTPBASE_CLOCK_PORT_DS_GRANT_DURATION, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	   snmpClockPortTable, 5, {1, 2, 8, 1, 14}},
	{ PTPBASE_CLOCK_PORT_DS_PTP_VERSION, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 15}},
	/* ptpbaseClockPortRunningTable */
	{ PTPBASE_CLOCK_PORT_RUNNING_NAME, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 5}},
	{ PTPBASE_CLOCK_PORT_RUNNING_STATE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 6}},
	{ PTPBASE_CLOCK_PORT_RUNNING_ROLE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 7}},
	/* { PTPBASE_CLOCK_PORT_RUNNING_INTERFACE_INDEX, ASN_INTEGER, HANDLER_CAN_RONLY, */
	/*   snmpClockPortTable, 5, {1, 2, 9, 1, 8}}, */
	{ PTPBASE_CLOCK_PORT_RUNNING_IPVERSION, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 9}},
	/* { PTPBASE_CLOCK_PORT_RUNNING_ENCAPSULATION_TYPE, ASN_INTEGER, HANDLER_CAN_RONLY, */
	/*   snmpClockPortTable, 5, {1, 2, 9, 1, 10}}, */
	{ PTPBASE_CLOCK_PORT_RUNNING_TX_MODE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 11}},
	{ PTPBASE_CLOCK_PORT_RUNNING_RX_MODE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 12}},
	{ PTPBASE_CLOCK_PORT_RUNNING_PACKETS_RECEIVED, ASN_COUNTER64, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 13}},
	{ PTPBASE_CLOCK_PORT_RUNNING_PACKETS_SENT, ASN_COUNTER64, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 14}},
	/* ptpbasePtpPortMessageCounters */
	{ PTPBASE_PORT_MESSAGE_COUNTERS_CLEAR_ALL, ASN_INTEGER, HANDLER_CAN_RWRITE,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 5}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_CLEAR, ASN_INTEGER, HANDLER_CAN_RWRITE,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 6}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_TOTAL_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 7}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_TOTAL_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 8}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_ANNOUNCE_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 9}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_ANNOUNCE_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 10}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_SYNC_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 11}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_SYNC_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 12}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_FOLLOWUP_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 13}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_FOLLOWUP_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 14}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_DELAYREQ_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 15}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_DELAYREQ_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 16}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_DELAYRESP_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 17}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_DELAYRESP_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 18}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYREQ_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 19}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYREQ_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 20}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 21}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 22}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_FOLLOWUP_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 23}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_PDELAYRESP_FOLLOWUP_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 24}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_SIGNALING_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 25}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_SIGNALING_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 26}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_MANAGEMENT_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 27}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_MANAGEMENT_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 28}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_DISCARDED_MESSAGES, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 29}},
	{ PTPBASE_PORT_MESSAGE_COUNTERS_UNKNOWN_MESSAGES, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortMessageCountersTable, 5, {1, 2, 12, 1, 30}},
	/* ptpbasePtpPortProtocolCounters */
	{ PTPBASE_PORT_PROTOCOL_COUNTERS_CLEAR, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortProtocolCountersTable, 5, {1, 2, 13, 1, 5}},
	{ PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_ADDED, ASN_INTEGER, HANDLER_CAN_RWRITE,
	  snmpPtpPortProtocolCountersTable, 5, {1, 2, 13, 1, 6}},
	{ PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_COUNT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortProtocolCountersTable, 5, {1, 2, 13, 1, 7}},
	{ PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_REMOVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortProtocolCountersTable, 5, {1, 2, 13, 1, 8}},
	{ PTPBASE_PORT_PROTOCOL_COUNTERS_FOREIGN_OVERFLOWS, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortProtocolCountersTable, 5, {1, 2, 13, 1, 9}},
	{ PTPBASE_PORT_PROTOCOL_COUNTERS_STATE_TRANSITIONS, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortProtocolCountersTable, 5, {1, 2, 13, 1, 10}},
	{ PTPBASE_PORT_PROTOCOL_COUNTERS_BEST_MASTER_CHANGES, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortProtocolCountersTable, 5, {1, 2, 13, 1, 11}},
	{ PTPBASE_PORT_PROTOCOL_COUNTERS_ANNOUNCE_TIMEOUTS, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortProtocolCountersTable, 5, {1, 2, 13, 1, 12}},
	/* ptpbasePtpPortErrorCounters */
	{ PTPBASE_PORT_ERROR_COUNTERS_CLEAR, ASN_INTEGER, HANDLER_CAN_RWRITE,
	  snmpPtpPortErrorCountersTable, 5, {1, 2, 14, 1, 5}},
	{ PTPBASE_PORT_ERROR_COUNTERS_MESSAGE_RECV, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortErrorCountersTable, 5, {1, 2, 14, 1, 6}},
	{ PTPBASE_PORT_ERROR_COUNTERS_MESSAGE_SEND, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortErrorCountersTable, 5, {1, 2, 14, 1, 7}},
	{ PTPBASE_PORT_ERROR_COUNTERS_MESSAGE_FORMAT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortErrorCountersTable, 5, {1, 2, 14, 1, 8}},
	{ PTPBASE_PORT_ERROR_COUNTERS_PROTOCOL, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortErrorCountersTable, 5, {1, 2, 14, 1, 9}},
	{ PTPBASE_PORT_ERROR_COUNTERS_VERSION_MISMATCH, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortErrorCountersTable, 5, {1, 2, 14, 1, 10}},
	{ PTPBASE_PORT_ERROR_COUNTERS_DOMAIN_MISMATCH, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortErrorCountersTable, 5, {1, 2, 14, 1, 11}},
	{ PTPBASE_PORT_ERROR_COUNTERS_SEQUENCE_MISMATCH, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortErrorCountersTable, 5, {1, 2, 14, 1, 12}},
	{ PTPBASE_PORT_ERROR_COUNTERS_DELAYMECH_MISMATCH, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortErrorCountersTable, 5, {1, 2, 14, 1, 13}},
	/* ptpbasePtpPortUnicastNegotiationCounters */
	{ PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_CLEAR, ASN_INTEGER, HANDLER_CAN_RWRITE,
	  snmpPtpPortUnicastNegotiationCountersTable, 5, {1, 2, 15, 1, 5}},
	{ PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_REQUESTED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortUnicastNegotiationCountersTable, 5, {1, 2, 15, 1, 6}},
	{ PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_GRANTED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortUnicastNegotiationCountersTable, 5, {1, 2, 15, 1, 7}},
	{ PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_DENIED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortUnicastNegotiationCountersTable, 5, {1, 2, 15, 1, 8}},
	{ PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortUnicastNegotiationCountersTable, 5, {1, 2, 15, 1, 9}},
	{ PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortUnicastNegotiationCountersTable, 5, {1, 2, 15, 1, 10}},
	{ PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_ACK_SENT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortUnicastNegotiationCountersTable, 5, {1, 2, 15, 1, 11}},
	{ PTPBASE_PORT_UNICAST_NEGOTIATION_COUNTERS_GRANTS_CANCEL_ACK_RECEIVED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortUnicastNegotiationCountersTable, 5, {1, 2, 15, 1, 12}},
	/* ptpbasePtpPortPerformanceCounters - no clear here */
	{ PTPBASE_PORT_PERFORMANCE_COUNTERS_MESSAGE_SEND_RATE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortPerformanceCountersTable, 5, {1, 2, 16, 1, 5}},
	{ PTPBASE_PORT_PERFORMANCE_COUNTERS_MESSAGE_RECEIVE_RATE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortPerformanceCountersTable, 5, {1, 2, 16, 1, 6}},
	/* ptpbasePtpPortSecurityCounters */
	{ PTPBASE_PORT_SECURITY_COUNTERS_CLEAR, ASN_INTEGER, HANDLER_CAN_RWRITE,
	  snmpPtpPortSecurityCountersTable, 5, {1, 2, 17, 1, 5}},
	{ PTPBASE_PORT_SECURITY_COUNTERS_TIMING_ACL_DISCARDED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortSecurityCountersTable, 5, {1, 2, 17, 1, 6}},
	{ PTPBASE_PORT_SECURITY_COUNTERS_MANAGEMENT_ACL_DISCARDED, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpPortSecurityCountersTable, 5, {1, 2, 17, 1, 7}},
	/* ptpbasePtpdSpecificCounters */
	{ PTPBASE_PTPD_SPECIFIC_COUNTERS_CLEAR, ASN_INTEGER, HANDLER_CAN_RWRITE,
	  snmpPtpdSpecificCountersTable, 5, {1, 2, 21, 1, 5}},
	{ PTPBASE_PTPD_SPECIFIC_COUNTERS_IGNORED_ANNOUNCE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpdSpecificCountersTable, 5, {1, 2, 21, 1, 6}},
	{ PTPBASE_PTPD_SPECIFIC_COUNTERS_CONSECUTIVE_SEQUENCE_ERRORS, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpdSpecificCountersTable, 5, {1, 2, 21, 1, 7}},
	{ PTPBASE_PTPD_SPECIFIC_COUNTERS_DELAYMS_OUTLIERS_FOUND, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpdSpecificCountersTable, 5, {1, 2, 21, 1, 8}},
	{ PTPBASE_PTPD_SPECIFIC_COUNTERS_DELAYSM_OUTLIERS_FOUND, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpdSpecificCountersTable, 5, {1, 2, 21, 1, 9}},
	{ PTPBASE_PTPD_SPECIFIC_COUNTERS_MAX_DELAY_DROPS, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpPtpdSpecificCountersTable, 5, {1, 2, 21, 1, 10}}
};

/**
 * Log messages from NetSNMP subsystem.
 */
static int
snmpLogCallback(int major, int minor,
		void *serverarg, void *clientarg)
{
	struct snmp_log_message *slm = (struct snmp_log_message *)serverarg;
	char *msg = strdup (slm->msg);
	if (msg) msg[strlen(msg)-1] = '\0';

	switch (slm->priority)
	{
	case LOG_EMERG:   EMERGENCY("snmp[emerg]: %s\n",   msg?msg:slm->msg); break;
	case LOG_ALERT:   ALERT    ("snmp[alert]: %s\n",   msg?msg:slm->msg); break;
	case LOG_CRIT:    CRITICAL ("snmp[crit]: %s\n",    msg?msg:slm->msg); break;
	case LOG_ERR:     ERROR    ("snmp[err]: %s\n",     msg?msg:slm->msg); break;
	case LOG_WARNING: WARNING  ("snmp[warning]: %s\n", msg?msg:slm->msg); break;
	case LOG_NOTICE:  NOTICE   ("snmp[notice]: %s\n",  msg?msg:slm->msg); break;
	case LOG_INFO:    INFO     ("snmp[info]: %s\n",    msg?msg:slm->msg); break;
	case LOG_DEBUG:   DBGV     ("snmp[debug]: %s\n",   msg?msg:slm->msg); break;
	}
	free(msg);
	return SNMP_ERR_NOERROR;
}

static const int
getNotifIndex(int eventType) {

	switch (eventType) {
		case PTPBASE_NOTIFS_UNEXPECTED_PORT_STATE:
		    return 1;
		case PTPBASE_NOTIFS_EXPECTED_PORT_STATE:
		    return 2;
		case PTPBASE_NOTIFS_SLAVE_OFFSET_THRESHOLD_EXCEEDED:
		    return 3;
		case PTPBASE_NOTIFS_SLAVE_OFFSET_THRESHOLD_ACCEPTABLE:
		    return 4;
		case PTPBASE_NOTIFS_SLAVE_CLOCK_STEP:
		    return 5;
		case PTPBASE_NOTIFS_SLAVE_NO_SYNC:
		    return 6;
		case PTPBASE_NOTIFS_SLAVE_RECEIVING_SYNC:
		    return 7;
		case PTPBASE_NOTIFS_SLAVE_NO_DELAY:
		    return 8;
		case PTPBASE_NOTIFS_SLAVE_RECEIVING_DELAY:
		    return 9;
		case PTPBASE_NOTIFS_BEST_MASTER_CHANGE:
		    return 10;
		case PTPBASE_NOTIFS_NETWORK_FAULT:
		    return 11;
		case PTPBASE_NOTIFS_NETWORK_FAULT_CLEARED:
		    return 12;
		case PTPBASE_NOTIFS_FREQADJ_FAST:
		    return 13;
		case PTPBASE_NOTIFS_FREQADJ_NORMAL:
		    return 14;
		case PTPBASE_NOTIFS_OFFSET_SECONDS:
		    return 15;
		case PTPBASE_NOTIFS_OFFSET_SUB_SECONDS:
		    return 16;
		default:
		    return 0;
	}
}

static void
populateNotif (netsnmp_variable_list** varBinds, int eventType) {

	switch (eventType) {
		case PTPBASE_NOTIFS_UNEXPECTED_PORT_STATE:
		case PTPBASE_NOTIFS_EXPECTED_PORT_STATE:
		    {
			oid portStateOid[] = { PTPBASE_MIB_OID, 1, 2, 9, 1, 6, PTPBASE_MIB_INDEX4 };

			snmp_varlist_add_variable(varBinds, portStateOid, OID_LENGTH(portStateOid),
			    ASN_INTEGER, (u_char *) &snmpPtpClock->portState, sizeof(snmpPtpClock->portState));
		    }
		    return;
		case PTPBASE_NOTIFS_SLAVE_OFFSET_THRESHOLD_EXCEEDED:
		case PTPBASE_NOTIFS_SLAVE_OFFSET_THRESHOLD_ACCEPTABLE:
		    {
			U64 ofmNum;
			Integer64  tmpi64;
			internalTime_to_integer64(snmpPtpClock->offsetFromMaster, &tmpi64);
			ofmNum.low = htonl(tmpi64.lsb);
			ofmNum.high = htonl(tmpi64.msb);

			tmpsnprintf(ofmStr, 64, "%.09f", timeInternalToDouble(&snmpPtpClock->offsetFromMaster));

			oid ofmOid[] = { PTPBASE_MIB_OID, 1, 2, 1, 1, 5, PTPBASE_MIB_INDEX3 };
			oid ofmStringOid[] = { PTPBASE_MIB_OID, 1, 2, 1, 1, 8, PTPBASE_MIB_INDEX3 };
			oid thresholdOid[] = { PTPBASE_MIB_OID, 1, 2, 1, 1, 9, PTPBASE_MIB_INDEX3 };

			snmp_varlist_add_variable(varBinds, ofmOid, OID_LENGTH(ofmOid),
			    ASN_OCTET_STR, (u_char *) &ofmNum, sizeof(ofmNum));
			snmp_varlist_add_variable(varBinds, ofmStringOid, OID_LENGTH(ofmStringOid),
			    ASN_OCTET_STR, (u_char *) ofmStr, strlen(ofmStr));
			snmp_varlist_add_variable(varBinds, thresholdOid, OID_LENGTH(thresholdOid),
			    ASN_INTEGER, (u_char *) &snmpRtOpts->ofmAlarmThreshold, sizeof(snmpRtOpts->ofmAlarmThreshold));
		    }
		    return;
		case PTPBASE_NOTIFS_SLAVE_CLOCK_STEP:
		    return;
		case PTPBASE_NOTIFS_SLAVE_NO_SYNC:
		    return;
		case PTPBASE_NOTIFS_SLAVE_RECEIVING_SYNC:
		    return;
		case PTPBASE_NOTIFS_SLAVE_NO_DELAY:
		    return;
		case PTPBASE_NOTIFS_SLAVE_RECEIVING_DELAY:
		    return;
		case PTPBASE_NOTIFS_BEST_MASTER_CHANGE:
		    return;
		case PTPBASE_NOTIFS_NETWORK_FAULT:
		    return;
		case PTPBASE_NOTIFS_NETWORK_FAULT_CLEARED:
		    return;
		case PTPBASE_NOTIFS_FREQADJ_FAST:
		    return;
		case PTPBASE_NOTIFS_FREQADJ_NORMAL:
		    return;
		case PTPBASE_NOTIFS_OFFSET_SECONDS:
		    return;
		case PTPBASE_NOTIFS_OFFSET_SUB_SECONDS:
		    return;
		default:
		    return;
	}

}

static void
sendNotif(int eventType) {

    /* snmpTrapOID.0 */
    oid trapOid[] = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0 };
    netsnmp_variable_list *varBinds = NULL;

    int notifIndex = getNotifIndex(eventType);

    if(!notifIndex) {
	DBG("SNMP trap request for unknown event type: %d\n", eventType);
	return;
    }

    /* the notification  oid*/
    oid notifOid[] = { PTPBASE_MIB_OID, 0, (oid)notifIndex };

    /* add notification oid */
    snmp_varlist_add_variable(&varBinds, trapOid, OID_LENGTH(trapOid),
		ASN_OBJECT_ID, (u_char *) notifOid, OID_LENGTH(notifOid) * sizeof(oid));

    /* add the accompanying varbinds */
    populateNotif(&varBinds, eventType);

    send_v2trap(varBinds);

    snmp_free_varbind(varBinds);

}


/**
 * Initialisation of SNMP subsystem.
 */
void
snmpInit(RunTimeOpts *rtOpts, PtpClock *ptpClock) {

	netsnmp_enable_subagent();
	snmp_disable_log();
	snmp_enable_calllog();
	snmp_register_callback(SNMP_CALLBACK_LIBRARY,
			       SNMP_CALLBACK_LOGGING,
			       snmpLogCallback,
			       NULL);
	init_agent("ptpAgent");
	REGISTER_MIB("ptpMib", snmpVariables, variable7, ptp_oid);
	init_snmp("ptpAgent");

	/* Currently, ptpd only handle one clock. We put it in a
	 * global variable for the need of our subsystem. */
	snmpPtpClock = ptpClock;
	snmpRtOpts = rtOpts;

}

/**
 * Clean up and shut down the SNMP subagent
 */

void
snmpShutdown() {

	unregister_mib(ptp_oid, sizeof(ptp_oid) / sizeof(oid));
	snmp_shutdown("ptpMib");
	SOCK_CLEANUP;

}
