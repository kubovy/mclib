/* 
 * File:   requirements.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */

#ifndef REQUIREMENTS_H
#define	REQUIREMENTS_H

#ifdef	__cplusplus
extern "C" {
#endif


#include "../../mcc_generated_files/mcc.h"

#ifndef BM78_DISABLE
#if defined BM78_SW_BTN_PORT && defined BM78_RST_N_PORT && defined BM78_P2_0_PORT && defined BM78_P2_4_PORT && defined BM78_EAN_PORT
#define BM78_ENABLED
#endif
#endif

#include "../../config.h"

#if (defined MCP_ENABLED || defined LCD_ENABLED) && !defined I2C_ENABLED
#define I2C_ENABLED
#endif

#ifndef RGB_DISABLE
#if defined RGB_R_DUTY_CYCLE || defined RGB_G_DUTY_CYCLE || defined RGB_B_DUTY_CYCLE
#define RGB_ENABLED
#endif
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* REQUIREMENTS_H */

