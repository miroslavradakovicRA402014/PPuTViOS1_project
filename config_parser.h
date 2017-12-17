#ifndef __CONFIG_PARSER_H__
#define __CONFIG_PARSER_H__

#include <stdio.h>

#define CONFIG_LINE_LEN 50
#define CONFIG_VAL_LEN 8

/**
 * @brief Enumeration of possible config parser error codes
 */
typedef enum _ConfigErrorCode
{
    CONFIG_PARSE_ERROR = 0,                         /* CONFIG_PARSE_ERROR */
	CONFIG_PARSE_OK = 1                             /* CONFIG_PARSE_OK */
}ConfigErrorCode;

/**
 * @brief Parse config file
 *
 * @param [in] configFile - path to config file
 * @param [out] config - config structure where config will be written in
 * @return config parser error
 */
ConfigErrorCode parseConfigFile(char* configFile, InitConfig* config);


#endif

