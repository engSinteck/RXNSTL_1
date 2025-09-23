/*
 * XT_mib.c
 *
 *  Created on: 1 de jun de 2020
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#include "main.h"
#include "stdio.h"
#include "string.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/snmpv3.h"
#include "lwip/apps/snmp_opts.h"
#include "lwip/snmp.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_core.h"
#include "lwip/apps/snmp_mib2.h"
#include "lwip/apps/snmp_scalar.h"

#include "../Sinteck/src/defines.h"
#include "../Sinteck/src/audio.h"
#include "../Sinteck/src/eeprom.h"
#include "../Sinteck/src/pwm.h"

extern volatile float tempeartura;
extern Cfg_var cfg;

const char* str_PowerState[] = { "OFF", "ON", "Power-Safe" };
//char string_snmp[100] = {0};

/*
 * SNMP general function prototype
 * Add here any function to be usable in specific private MIB node.
 */
static int16_t get_forward_value(struct snmp_node_instance* instance, void* value);
static int16_t get_reflected_value(struct snmp_node_instance* instance, void* value);
static int16_t get_rfin_value(struct snmp_node_instance* instance, void* value);
static int16_t get_temperatura_value(struct snmp_node_instance* instance, void* value);
static int16_t get_status_value(struct snmp_node_instance* instance, void* value);
static int16_t get_bat_value(struct snmp_node_instance* instance, void* value);
static int16_t get_R_AC_value(struct snmp_node_instance* instance, void* value);
static int16_t get_S_AC_value(struct snmp_node_instance* instance, void* value);
static int16_t get_T_AC_value(struct snmp_node_instance* instance, void* value);
static int16_t get_stationname_value(struct snmp_node_instance* instance, void* value);
static int16_t get_frequency_value(struct snmp_node_instance* instance, void* value);
static int16_t get_powerstate_value(struct snmp_node_instance* instance, void* value);
static int16_t get_falha_value(struct snmp_node_instance* instance, void* value);
static int16_t get_mpx_value(struct snmp_node_instance* instance, void* value);
static int16_t get_audiol_value(struct snmp_node_instance* instance, void* value);
static int16_t get_audior_value(struct snmp_node_instance* instance, void* value);
static int16_t get_preset_value(struct snmp_node_instance* instance, void* value);
static int16_t get_audiosource_value(struct snmp_node_instance* instance, void* value);
static int16_t get_servico_value(struct snmp_node_instance* instance, void* value);
static int16_t get_ofs_value(struct snmp_node_instance* instance, void* value);
static int16_t get_alarm_mpx_value(struct snmp_node_instance* instance, void* value);
//
static snmp_err_t set_powerstate_value(struct snmp_node_instance* instance, uint16_t len, void *value);
static snmp_err_t set_preset_value(struct snmp_node_instance* instance, uint16_t len, void *value);
static snmp_err_t set_audiosource_value(struct snmp_node_instance* instance, uint16_t len, void *value);
static snmp_err_t set_servico_value(struct snmp_node_instance* instance, uint16_t len, void *value);
static snmp_err_t set_ofs_value(struct snmp_node_instance* instance, uint16_t len, void *value);

/* forward .1.3.6.1.4.1.54532.7.1.0 */
static const struct snmp_scalar_node forward_node = SNMP_SCALAR_CREATE_NODE_READONLY(1, SNMP_ASN1_TYPE_INTEGER, get_forward_value);
/* reflected .1.3.6.1.m4.1.54532.7.2.0 */
static const struct snmp_scalar_node reflected_node = SNMP_SCALAR_CREATE_NODE_READONLY(2, SNMP_ASN1_TYPE_INTEGER, get_reflected_value);
/* rfin .1.3.6.1.4.1.54532.7.3.0 */
static const struct snmp_scalar_node rfin_node = SNMP_SCALAR_CREATE_NODE_READONLY(3, SNMP_ASN1_TYPE_INTEGER, get_rfin_value);
/* temperatura .1.3.6.1.4.1.54532.7.4.0 */
static const struct snmp_scalar_node temp_node = SNMP_SCALAR_CREATE_NODE_READONLY(4, SNMP_ASN1_TYPE_INTEGER, get_temperatura_value);
/* status .1.3.6.1.4.1.54532.7.5.0 */
static const struct snmp_scalar_node status_node = SNMP_SCALAR_CREATE_NODE_READONLY(5, SNMP_ASN1_TYPE_OCTET_STRING, get_status_value);
/* status .1.3.6.1.4.1.54532.7.6.0 */
static const struct snmp_scalar_node bat_node = SNMP_SCALAR_CREATE_NODE_READONLY(6, SNMP_ASN1_TYPE_INTEGER, get_bat_value);
/* status .1.3.6.1.4.1.54532.7.7.0 */
static const struct snmp_scalar_node rac_node = SNMP_SCALAR_CREATE_NODE_READONLY(7, SNMP_ASN1_TYPE_INTEGER, get_R_AC_value);
/* status .1.3.6.1.4.1.54532.7.8.0 */
static const struct snmp_scalar_node stationname_node = SNMP_SCALAR_CREATE_NODE_READONLY(8, SNMP_ASN1_TYPE_OCTET_STRING, get_stationname_value);
/* status .1.3.6.1.4.1.54532.7.9.0 */
static const struct snmp_scalar_node frequency_node = SNMP_SCALAR_CREATE_NODE_READONLY(9, SNMP_ASN1_TYPE_INTEGER, get_frequency_value);
/* status .1.3.6.1.4.1.54532.7.10.0 */
static const struct snmp_scalar_node powerstate_node = SNMP_SCALAR_CREATE_NODE(10, SNMP_NODE_INSTANCE_READ_WRITE, SNMP_ASN1_TYPE_INTEGER, get_powerstate_value, NULL, set_powerstate_value);
/* status .1.3.6.1.4.1.54532.7.11.0 */
static const struct snmp_scalar_node falha_node = SNMP_SCALAR_CREATE_NODE_READONLY(11, SNMP_ASN1_TYPE_OCTET_STRING, get_falha_value);
/* status .1.3.6.1.4.1.54532.7.12.0 */
static const struct snmp_scalar_node preset_node = SNMP_SCALAR_CREATE_NODE(12, SNMP_NODE_INSTANCE_READ_WRITE, SNMP_ASN1_TYPE_INTEGER, get_preset_value, NULL, set_preset_value);
/* status .1.3.6.1.4.1.54532.7.13.0 */
static const struct snmp_scalar_node audiosource_node = SNMP_SCALAR_CREATE_NODE(13, SNMP_NODE_INSTANCE_READ_WRITE, SNMP_ASN1_TYPE_INTEGER, get_audiosource_value, NULL, set_audiosource_value);
/* status .1.3.6.1.4.1.54532.7.14.0 */
static const struct snmp_scalar_node audiol_node = SNMP_SCALAR_CREATE_NODE_READONLY(14, SNMP_ASN1_TYPE_INTEGER, get_audiol_value);
/* status .1.3.6.1.4.1.54532.7.15.0 */
static const struct snmp_scalar_node audior_node = SNMP_SCALAR_CREATE_NODE_READONLY(15, SNMP_ASN1_TYPE_INTEGER, get_audior_value);
/* status .1.3.6.1.4.1.54532.7.16.0 */
static const struct snmp_scalar_node mpx_node = SNMP_SCALAR_CREATE_NODE(16, SNMP_NODE_INSTANCE_READ_ONLY, SNMP_ASN1_TYPE_INTEGER, get_mpx_value, NULL, NULL);
/* status .1.3.6.1.4.1.54532.7.17.0 */
static const struct snmp_scalar_node sac_node = SNMP_SCALAR_CREATE_NODE_READONLY(17, SNMP_ASN1_TYPE_INTEGER, get_S_AC_value);
/* status .1.3.6.1.4.1.54532.7.18.0 */
static const struct snmp_scalar_node tac_node = SNMP_SCALAR_CREATE_NODE_READONLY(18, SNMP_ASN1_TYPE_INTEGER, get_T_AC_value);
/* status .1.3.6.1.4.1.54532.7.19.0 */
static const struct snmp_scalar_node servico_node = SNMP_SCALAR_CREATE_NODE(19, SNMP_NODE_INSTANCE_READ_WRITE, SNMP_ASN1_TYPE_INTEGER, get_servico_value, NULL, set_servico_value);
/* status .1.3.6.1.4.1.54532.7.20.0 */
static const struct snmp_scalar_node ofsbat_node = SNMP_SCALAR_CREATE_NODE(20, SNMP_NODE_INSTANCE_READ_WRITE, SNMP_ASN1_TYPE_INTEGER, get_ofs_value, NULL, set_ofs_value);
/* status .1.3.6.1.4.1.54532.7.21.0 */
static const struct snmp_scalar_node alarm_mpx_node = SNMP_SCALAR_CREATE_NODE_READONLY(21, SNMP_ASN1_TYPE_INTEGER, get_alarm_mpx_value);


/* all private nodes  .1.3.6.1.4.1.54532.7.. .. (.1,.2,.3..21) */
static const struct snmp_node* const xt_nodes[] = { &forward_node.node.node,
													&reflected_node.node.node,
													&rfin_node.node.node,
													&temp_node.node.node,
													&status_node.node.node,
													&bat_node.node.node,
													&rac_node.node.node,
													&stationname_node.node.node,
													&frequency_node.node.node,
													&powerstate_node.node.node,
													&falha_node.node.node,
													&preset_node.node.node,
													&audiosource_node.node.node,
													&audiol_node.node.node,
													&audior_node.node.node,
													&mpx_node.node.node,
													&servico_node.node.node,
													&sac_node.node.node,
													&tac_node.node.node,
													&ofsbat_node.node.node,
													&alarm_mpx_node.node.node
};

static const struct snmp_tree_node xt_tree_node = SNMP_CREATE_TREE_NODE(1, xt_nodes);

/* define private mib */
const uint32_t my_base_oid[] = { 1, 3, 6, 1, 4, 1, 54532, 7 };
const struct snmp_mib gpio_mib = SNMP_MIB_CREATE(my_base_oid, &xt_tree_node.node);

/* forward value .1.3.6.1.4.1.54532.7.1.0 */
static int16_t get_forward_value(struct snmp_node_instance* instance, void* value)
{
	uint32_t *uint_ptr = (uint32_t*) value;
	//*uint_ptr = (uint32_t)forward;
	return sizeof(*uint_ptr);
}

/* reflected value .1.3.6.1.4.1.54532.7.2.0 */
static int16_t get_reflected_value(struct snmp_node_instance* instance, void* value)
{
	uint32_t *uint_ptr = (uint32_t*) value;
	//*uint_ptr = (uint32_t)reflected;
	return sizeof(*uint_ptr);
}

/* rfin value .1.3.6.1.4.1.54532.7.3.0 */
static int16_t get_rfin_value(struct snmp_node_instance* instance, void* value)
{
	uint32_t *uint_ptr = (uint32_t*) value;
	//*uint_ptr = (uint32_t)rfin;
	return sizeof(*uint_ptr);
}

/* Temperature value .1.3.6.1.4.1.54532.7.4.0 */
static int16_t get_temperatura_value(struct snmp_node_instance* instance, void* value)
{
	uint32_t *uint_ptr = (uint32_t*) value;
	//*uint_ptr = (uint32_t)temperatura;
	return sizeof(*uint_ptr);
}

/* status value .1.3.6.1.4.1.54532.7.5.0 */
static int16_t get_status_value(struct snmp_node_instance* instance, void* value)
{
	//uint8_t *char_ptr = (uint8_t *) value;
	//memcpy(char_ptr, (uint8_t *)str_PowerState[RFEnable], strlen(str_PowerState[RFEnable]));
	//return strlen(str_PowerState[RFEnable]) * sizeof(*char_ptr);
	return 0;
}

/* Bateria value .1.3.6.1.4.1.54532.7.6.0 */
static int16_t get_bat_value(struct snmp_node_instance* instance, void* value)
{
	uint32_t *uint_ptr = (uint32_t*) value;
	//*uint_ptr = (uint32_t)vbat;
	return sizeof(*uint_ptr);
}

/* AC value .1.3.6.1.4.1.54532.7.7.0 */
static int16_t get_R_AC_value(struct snmp_node_instance* instance, void* value)
{
	uint32_t *uint_ptr = (uint32_t*) value;
	//*uint_ptr = (uint32_t)find_min_Vinput_Eltek(model_type);
	return sizeof(*uint_ptr);
}

/* AC value .1.3.6.1.4.1.54532.7.17.0 */
static int16_t get_S_AC_value(struct snmp_node_instance* instance, void* value)
{
	uint32_t *uint_ptr = (uint32_t*) value;
	//*uint_ptr = (uint32_t)find_min_Vinput_Eltek(model_type);
	return sizeof(*uint_ptr);
}

/* AC value .1.3.6.1.4.1.54532.7.18.0 */
static int16_t get_T_AC_value(struct snmp_node_instance* instance, void* value)
{
	uint32_t *uint_ptr = (uint32_t*) value;
	//*uint_ptr = (uint32_t)find_min_Vinput_Eltek(model_type);
	return sizeof(*uint_ptr);
}

/* Station Name value .1.3.6.1.4.1.54532.7.8.0 */
static int16_t get_stationname_value(struct snmp_node_instance* instance, void* value)
{
	return 0;
}

/* Frequency value .1.3.6.1.4.1.54532.7.9.0 */
static int16_t get_frequency_value(struct snmp_node_instance* instance, void* value)
{
	uint32_t *uint_ptr = (uint32_t*) value;
	*uint_ptr = (uint32_t)cfg.Frequencia;
	return sizeof(*uint_ptr);
}

/* PowerState value .1.3.6.1.4.1.54532.7.10.0 */
static snmp_err_t set_powerstate_value(struct snmp_node_instance* instance, uint16_t len, void *value)
{
	//uint32_t val = *((uint32_t*)value);
	return SNMP_ERR_NOERROR;
}

/* PowerState value .1.3.6.1.4.1.54532.7.10.0 */
static int16_t get_powerstate_value(struct snmp_node_instance* instance, void* value)
{
	uint32_t *uint_ptr = (uint32_t*) value;
	//*uint_ptr = (uint32_t)RFEnable;
	return sizeof(*uint_ptr);
}

/* Falha value .1.3.6.1.4.1.54532.7.11.0 */
static int16_t get_falha_value(struct snmp_node_instance* instance, void* value)
{
	char msg[20] = {0};

	//sprintf(msg, "%s", decode_falha_snmp(falha));
	uint8_t *char_ptr = (uint8_t *) value;
	//memcpy( char_ptr, (uint8_t *)msg, strlen(msg) );
	return strlen(msg) * sizeof(*char_ptr);
}

/* Preset value .1.3.6.1.4.1.54532.7.12.0 */
static int16_t get_preset_value(struct snmp_node_instance* instance, void* value)
{
	return 0;
}

static snmp_err_t set_preset_value(struct snmp_node_instance* instance, uint16_t len, void *value)
{
	//uint32_t val = *((uint32_t*)value) - 1;
	return SNMP_ERR_NOERROR;
}

/* Preset value .1.3.6.1.4.1.54532.7.13.0 */
static int16_t get_audiosource_value(struct snmp_node_instance* instance, void* value)
{
	return 0;
}

/* Preset value .1.3.6.1.4.1.54532.7.13.0 */
static snmp_err_t set_audiosource_value(struct snmp_node_instance* instance, uint16_t len, void *value)
{
	//uint32_t val = *((uint32_t*)value);
	return SNMP_ERR_NOERROR;
}

/* Audio_Left value .1.3.6.1.4.1.54532.7.14.0 */
static int16_t get_audiol_value(struct snmp_node_instance* instance, void* value)
{
	return 0;
}

/* Audio_Right value .1.3.6.1.4.1.54532.7.15.0 */
static int16_t get_audior_value(struct snmp_node_instance* instance, void* value)
{
	return 0;
}

/* MPX value .1.3.6.1.4.1.54532.7.16.0 */
static int16_t get_mpx_value(struct snmp_node_instance* instance, void* value)
{
	return 0;
}

/* PowerState value .1.3.6.1.4.1.54532.7.19.0 */
static int16_t get_servico_value(struct snmp_node_instance* instance, void* value)
{
	return 0;
}

/* PowerState value .1.3.6.1.4.1.54532.7.19.0 */
static snmp_err_t set_servico_value(struct snmp_node_instance* instance, uint16_t len, void *value)
{
	return SNMP_ERR_NOERROR;
}

/* PowerState value .1.3.6.1.4.1.54532.7.20.0 */
static int16_t get_ofs_value(struct snmp_node_instance* instance, void* value)
{
	return 0;
}

/* PowerState value .1.3.6.1.4.1.54532.7.21.0 */
static int16_t get_alarm_mpx_value(struct snmp_node_instance* instance, void* value)
{
	return 0;
}

/* PowerState value .1.3.6.1.4.1.54532.7.20.0 */
static snmp_err_t set_ofs_value(struct snmp_node_instance* instance, uint16_t len, void *value)
{
	return SNMP_ERR_NOERROR;
}
