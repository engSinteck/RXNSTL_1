/*
 * defines.h
 *
 *  Created on: Aug 25, 2025
 *      Author: rinaldo.santos
 */

#ifndef SRC_DEFINES_H_
#define SRC_DEFINES_H_

#define		PRG_REVISION		4
#define		MODELO				0

// Enderecos EEPROM
#define ADDR_ID					0
#define ADDR_REV				2
#define ADDR_MODEL				6
#define ADDR_TABREV				7

#define ADDR_SERV				0x1000
#define ADDR_SN					0x2000		// Endereço Serial Number
#define ADDR_LIC				0x2018		// Endereço Licença
#define ADDR_LIC_SEQ			0x2030
#define ADDR_TMR_LIC			0x2040

// Variaveis Configurações
#define ADDR_STEREO				16			// 1
#define ADDR_PROCESSOR			17			// 1
#define ADDR_CLIPPER			18			// 1
#define ADDR_EMPHASE			19			// 1
#define ADDR_AUDIO				20			// 1
#define ADDR_VOLMPX1			21			// 1
#define ADDR_VOLMPX2			22			// 1
#define ADDR_VOLMPX3			23			// 1
#define ADDR_VOLSCA				24			// 1
#define ADDR_VOLLEFT			25			// 1
#define ADDR_VOLRIGHT			26			// 1
#define ADDR_TOSKLINK			27			// 1
#define ADDR_IMPEDANCE			28			// 1
#define ADDR_AES192				29			// 1
#define ADDR_LEVELON			30			// 1
#define ADDR_LEVELOFF			31			// 1
#define	ADDR_SWITCHMOD			32			// 1
#define ADDR_TIMERON			33			// 4
#define ADDR_VAGO_1				37			// 1
#define ADDR_VAGO_2				38			// 1
#define ADDR_VAGO_3				39			// 1
#define ADDR_TIMEROFF			40			// 4
#define ADDR_FREQ				44			// 4
//
#define ADDR_MODO				48			// 1
#define ADDR_RFENABLE			49			// 1
//
#define	ADDR_TIMEZONE			50			// 1
#define ADDR_SNMP				51			// 1
#define ADDR_IPADDR				52			// 4
#define ADDR_MASKADDR			56			// 4
#define ADDR_GWADDR				60			// 4
#define ADDR_DNSADDR			64			// 4
#define ADDR_PORTWEB			68			// 2
#define ADDR_VSWRNULL			70			// 1
#define ADDR_VAGO_4				71			// 1
#define ADDR_PASSADMIN			72			// 4
#define ADDR_PASSUSER			76			// 4
#define ADDR_VSWRVALUE			80			// 4
#define ADDR_FWDNULL			84			// 4
#define ADDR_ADV_RSSI1			88			// 4
#define ADDR_ADV_RSSI2			92			// 4
#define ADDR_ADV_MPX			96			// 4
#define ADDR_ADV_LEFT			100			// 4
#define ADDR_ADV_RIGHT			104			// 4
#define ADDR_FOLD_IPA			108			// 4
#define ADDR_FOLD_VSWR			112			// 4
#define ADDR_FOLD_TEMP			116			// 4
#define ADDR_RDS_REMOTE			120			// 1
#define ADDR_RDS_PORTRDS		121			// 2
#define ADDR_RDS_ENABLE			123			// 1
#define ADDR_RDS_EN_TEXT		124			// 1
#define ADDR_RDS_EN_STATION		125			// 1
#define ADDR_RDS_DATE			126			// 1
#define ADDR_RDS_TYPE			127			// 1
#define ADDR_RDS_PI				128			// 2
#define ADDR_RDS_TP				130			// 1
#define ADDR_RDS_CT				131			// 1
#define ADDR_RDS_PTY			132			// 1
#define ADDR_RDS_MS				133			// 1
#define ADDR_VAGO_5				134			// 1
#define ADDR_VAGO_6				135			// 1
#define ADDR_RDS_PS				136			// 16
#define ADDR_RDS_PTYN			152			// 16
#define ADDR_RDS_TPS			168			// 16
#define ADDR_RDS_AF				184			// 4
#define ADDR_VAGO_7				185			// 1
#define ADDR_VAGO_8				186			// 1
#define ADDR_VAGO_9				187			// 1
#define ADDR_VAGO_10			188			// 1
#define ADDR_VAGO_11			189			// 1
#define ADDR_VAGO_12			190			// 1
#define ADDR_VAGO_13			191			// 1
#define ADDR_RDS_RT				192			// 80
#define ADDR_RDS_DYN			320			// 80

#define ADDR_ADV_MAXIPA			448			// 4
#define ADDR_ADV_MAXVSWR		452			// 4
#define ADDR_ADV_MAXTEMP		456			// 4
#define ADDR_ADV_GAINFWD		460			// 4
#define ADDR_ADV_GAINREF		464			// 4
#define ADDR_ADV_GAINIPA		468			// 4
#define ADDR_ADV_GAINVPA		472			// 4
#define ADDR_ADV_GAINTEMP		476			// 4
#define ADDR_CONFIGHOLD			480			// 1
#define ADDR_CONFIGNTP			481			// 1
#define ADDR_PROF_STATION		512			// 64
#define ADDR_PROF_CITY			576			// 64
#define ADDR_PROF_STATE			640			// 64
#define ADDR_PROF_COUNTRY		704			// 64
#define ADDR_CFG_BW				768			// 1
#define ADDR_CFG_ATN			769			// 4

// Mapeamento de Falhas
#define FAIL_PLLLOCK            0           // Falha PLL Nao LOCK
#define FAIL_10MHZ				1			// Falha Oscilador 10MHz
#define FAIL_FAN1               2           // Falha Rotacao FAN 1
#define FAIL_FAN2               3           // Falha Rotacao FAN 2
#define FAIL_FAN3				4			// Falha Rotacao FAN 3
#define FAIL_SWR                5           // Falha de RF Sem Antenna
#define FAIL_TEMPERATURE        6           // Temperatura Anormal
#define FAIL_VPA                7           // Tensao VPA Anormal
#define FAIL_IPA                8           // Corrente de IPA acima do programado
#define FAIL_FOLD               9           // Estado de Foldback
#define FAIL_FOLDIPA            10          // Estado Foldback IPA
#define FAIL_RFL                11          // Refletida Alta
#define FAIL_REMOTE				12			// Entrada Remote
#define FAIL_USB_CONNECT		13			// Entrada USB Sem Pendrive
#define FAIL_USB_MOUNT			14			// USB No Mount
#define FAIL_SYNCRDS			15			// SYNC RDS
#define FAIL_RDSCARD			16			// RDS Card Nao Instalado
#define FAIL_VINPUT				17			// Falha Tensao de Entrada
#define FAIL_PSTEMP				18			// Temperatura Fonte Alta
#define FAIL_RFDRIVER			19			// Falha RF Driver
#define FAIL_IPADRIVER			20			// Falha Corrente Driver Acima Estabelecido
#define FAIL_NOANTENNA			21			// Falha RF OFF sem Antenna
#define FAIL_ALLFAN				22			// Falha Simultanea FAN'S
#define FAIL_MECSENSE			23			// Falha Sensor Mecanico
#define FAIL_BATTLOW			24			// Falha Bateria Baixa
#define FAIL_PSU				25			// Falha Comunicacao PSU
#define FAIL_PSUWALK			26			// ELTEK em Modo WALK
#define FAIL_LICENSE			27			// Licenca Vencida
#define FAIL_UPLINK				28			// Falha UPLink (RUS)
#define	FAIL_BATCHARGE			29			// Bateria em Modo Carga
#define FAIL_BATMAX				30			// Max Discharge Battery
#define FAIL_PWR				31			// Falha Potencia Direta
#define FAIL_TEMPDRV			32			// Temperatura Driver
#define FAIL_IPA_BURN			33			// IPA > 26.6 A
#define FAIL_PALLET				34			// Falha Pallet
#define FAIL_ELTEK				35			// Alarme ELTEK
#define FAIL_FAULTIPA			36			// Alarme Falta IPA em determinado Pallet
#define FAIL_STACK				37			// Falha Stack Overflow
#define FAIL_AUDIOMPX			38			// Falha Audio Modulacao


#endif /* SRC_DEFINES_H_ */
