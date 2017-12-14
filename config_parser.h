#ifndef __CONFIG_PARSER_H__
#define __CONFIG_PARSER_H__

#include <stdio.h>

/**
 * @brief Enumeration of possible config parser error codes
 */
typedef enum _ConfigErrorCode
{
    CONFIG_PARSE_ERROR = 0,                         /* CONFIG_PARSE_ERROR */
	CONFIG_PARSE_OK = 1                             /* CONFIG_PARSE_OK */
}ConfigErrorCode;

/**
 * @brief Open config file
 *
 * @param [in] configFile - path to config file
 * @return file config descriptor or NULL
 */
FILE* configOpen(char* configFile);

/**
 * @brief Parse config file
 *
 * @param [in] configFile - path to config file
 * @param [out] config - config structure where config has been written in
 * @return config parser error
 */
ConfigErrorCode parseConfigFile(char* configFile, InitConfig* config);


#endif

