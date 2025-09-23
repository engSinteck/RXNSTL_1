/*
 * certificado.h
 *
 *  Created on: Mar 3, 2021
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#ifndef TCP_CERTIFICADO_H_
#define TCP_CERTIFICADO_H_

/*
static const char mbedtls_root2_certificate[] = 						\
"-----BEGIN CERTIFICATE-----\r\n"										\
"MIIEDzCCAvegAwIBAgIUZTNod6uoiKUzxEgAOsH6S9jZ4BswDQYJKoZIhvcNAQEL\r\n"	\
"BQAwgZYxCzAJBgNVBAYTAkJSMRIwEAYDVQQIDAlTYW8gUGF1bG8xEjAQBgNVBAcM\r\n"	\
"CVNhbyBQYXVsbzEVMBMGA1UECgwMU2ludGVjayBOZXh0MQswCQYDVQQLDAJYVDEX\r\n"	\
"MBUGA1UEAwwOMTkyLjE2OC4xMC4xNjUxIjAgBgkqhkiG9w0BCQEWE3JpbmFsZG9A\r\n"	\
"c2ludGVjay5jb20wHhcNMjAxMjAyMTY0NzU3WhcNMjUxMjAxMTY0NzU3WjCBljEL\r\n"	\
"MAkGA1UEBhMCQlIxEjAQBgNVBAgMCVNhbyBQYXVsbzESMBAGA1UEBwwJU2FvIFBh\r\n"	\
"dWxvMRUwEwYDVQQKDAxTaW50ZWNrIE5leHQxCzAJBgNVBAsMAlhUMRcwFQYDVQQD\r\n"	\
"DA4xOTIuMTY4LjEwLjE2NTEiMCAGCSqGSIb3DQEJARYTcmluYWxkb0BzaW50ZWNr\r\n"	\
"LmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAOZmobrBEXdjng5q\r\n"	\
"JnAruPSMVxR5WRntO1eyMwMAWm77COPyENk3/4JzEKF93I82qUepuktvVz+xhNES\r\n"	\
"v5ORjTz86p9yeeW223+k4sWA3lCci4bSX2M6bOCpBozEPp0MubPp2F4k7jswFV+Y\r\n"	\
"8aMc3GwHgK6woAWFgo0+id+t/JQEW4Hs5+4CL1tSzbpXgpy/R1K9syhdni9S6kpX\r\n"	\
"Ry/5GNopEWP7tF5fo4OAQPZxEW6ALp6xe3M8nssKS01CyqZHUzOpou+vi6fVhVhx\r\n"	\
"ntaDMIkvnG3lzqLBsiJO8ouV716aDBSEZERaEmcaXyKEJXiTIUIsgu7Y4iq2UmNY\r\n"	\
"1V5dn4kCAwEAAaNTMFEwHQYDVR0OBBYEFBY4DnsX0rWVEriP9TFl/CQGAJa3MB8G\r\n"	\
"A1UdIwQYMBaAFBY4DnsX0rWVEriP9TFl/CQGAJa3MA8GA1UdEwEB/wQFMAMBAf8w\r\n"	\
"DQYJKoZIhvcNAQELBQADggEBANKekomCslRUGunN4iDXNvI27bVOTW/OAyLHbbA1\r\n"	\
"5KmYTPu2UbcSk3uihpLecA1XBLTOHR66K2uTCy4eaEFXi9IvVNIRSJ8m1WcRYq2O\r\n"	\
"br4sFO6a5zcnkFCtiRlxp5MqVyDjzaJtf0xAXLW2/ozy1NKMMAvpd6w2BZ99//uL\r\n"	\
"frXBuhTom4rioGSbBDMlXF/djZOWJVXEWxV8c74JhmEMjlUgZHVLzoIo957qUi3l\r\n"	\
"/Dolku2vWyziP8Lj51nWEbVZI/z5Bm5/CPo5OPgLdhOcCXi98RTb4/X6ixcJG51H\r\n"	\
"Rb3nsR0WQe+gWqY5TnUvfppN2tCdVALBppX9y0GkRenp2R8=\r\n"					\
"-----END CERTIFICATE-----\r\n";
*/

static const char mbedtls_root2_certificate[] = 									\
"-----BEGIN CERTIFICATE-----\r\n"													\
"MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4GA1UECxMXR2xv\r\n"	\
"YmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNpZ24xEzARBgNVBAMTCkdsb2Jh\r\n"	\
"bFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxT\r\n"	\
"aWduIFJvb3QgQ0EgLSBSMjETMBEGA1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2ln\r\n"	\
"bjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6\r\n"	\
"ErPLv4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8eoLrvozp\r\n"	\
"s6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklqtTleiDTsvHgMCJiEbKjN\r\n"	\
"S7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzdC9XZzPnqJworc5HGnRusyMvo4KD0L5CL\r\n"	\
"TfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pazq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6C\r\n"	\
"ygPCm48CAwEAAaOBnDCBmTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\r\n"	\
"FgQUm+IHV2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5nbG9i\r\n"	\
"YWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG3lm0mi3f3BmGLjAN\r\n"	\
"BgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4GsJ0/WwbgcQ3izDJr86iw8bmEbTUsp\r\n"	\
"9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu\r\n"	\
"01yiPqFbQfXf5WRDLenVOavSot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG7\r\n"	\
"9G+dwfCMNYxdAfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\r\n"	\
"TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\r\n"				\
"-----END CERTIFICATE-----\r\n";

static const size_t mbedtls_root_certificate_len = sizeof(mbedtls_root2_certificate);

/* use static allocation to keep the heap size as low as possible */
//#ifdef MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MAX_MEM_SIZE      24 * 1024
extern uint8_t mbedtls_memory_buf[MAX_MEM_SIZE];
extern uint8_t mbedtls_memory_mqtt_buf[MAX_MEM_SIZE];
//#endif

#endif /* TCP_CERTIFICADO_H_ */
