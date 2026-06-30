/**
 * @file ev_dash.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "ev_dash.h"
#include "user_code/speedometer_needle.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ev_dash_init(const char * asset_path)
{
    ev_dash_init_gen(asset_path);
    speedometer_needle_init();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/