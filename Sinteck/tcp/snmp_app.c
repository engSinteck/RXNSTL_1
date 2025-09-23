/*
 * snmp_app.c
 *
 *  Created on: 1 de jun de 2020
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#include "snmp_app.h"

#include "lwip/apps/snmp.h"
#include "lwip/apps/snmpv3.h"
#include "lwip/apps/snmp_opts.h"
#include "lwip/snmp.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_core.h"
#include "lwip/apps/snmp_mib2.h"
#include "lwip/apps/snmp_scalar.h"

//#define SNMP_SERVER_IP "172.16.0.250"
#define TRAP_DESTINATION_INDEX 0

/* transport information to my_mib.c */
extern const struct snmp_mib gpio_mib;
/*
 * ----- TODO: Global variables for SNMP Trap vvv
 * Define your own vars SNMP_SYSDESCR for System Description, SNMP_SYSCONTACT
 * for your contact mail, SNMP_SYSNAME for your system name, SNMP_SYSLOCATION
 * for your location. Also consider the size of each string in _LEN functions.
*/
static const struct snmp_mib *my_snmp_mibs[] = { &mib2, &gpio_mib };
//
const uint8_t * SNMP_SYSDESCR = (uint8_t*) "Sinteck_TXNSTL_agent";
const uint16_t SNMP_SYSDESCR_LEN = sizeof("Sinteck_TXNSTL_agent");
//
uint8_t * SNMP_SYSCONTACT = (uint8_t*) "fabio@sinteck.com";
uint16_t SNMP_SYSCONTACT_LEN = sizeof("fabio@sinteck.com");
//
uint8_t * SNMP_SYSNAME = (uint8_t*) "Sinteck XT Series";
uint16_t SNMP_SYSNAME_LEN = sizeof("Sinteck XT Series");
//
uint8_t * SNMP_SYSLOCATION = (uint8_t*) "Sinteck Next";
uint16_t SNMP_SYSLOCATION_LEN = sizeof("Sinteck Next");
/*
 * ----- TODO: Global variables for SNMP Trap ^^^
 */

/* buffer for snmp service */
uint16_t snmp_buffer = 512;

void initialize_snmp(void)
{
	snmp_threadsync_init(&snmp_mib2_lwip_locks, snmp_mib2_lwip_synchronizer);
	snmp_mib2_set_syscontact(SNMP_SYSCONTACT, &SNMP_SYSCONTACT_LEN, snmp_buffer);
	snmp_mib2_set_syslocation(SNMP_SYSLOCATION, &SNMP_SYSLOCATION_LEN, snmp_buffer);
	snmp_set_auth_traps_enabled(DISABLE);
	snmp_mib2_set_sysdescr(SNMP_SYSDESCR, &SNMP_SYSDESCR_LEN);
	snmp_mib2_set_sysname(SNMP_SYSNAME, &SNMP_SYSNAME_LEN, snmp_buffer);

	const struct snmp_obj_id my_base_oid_2 = {7, {1, 3, 6, 1, 4, 1, 5432}};
	snmp_set_device_enterprise_oid(&my_base_oid_2);

//	ip_addr_t gw = { 0 };
//  ipaddr_aton(SNMP_SERVER_IP, &gw);
//	snmp_trap_dst_ip_set(TRAP_DESTINATION_INDEX, &gw);

	snmp_trap_dst_enable(TRAP_DESTINATION_INDEX, DISABLE);
	snmp_set_mibs(my_snmp_mibs, LWIP_ARRAYSIZE(my_snmp_mibs));

    snmp_init();
}
