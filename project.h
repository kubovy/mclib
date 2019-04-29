/* 
 * File:   project.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * Includes all libraries, modules, components and generated MC files.
 * 
 * This file should be included only by the project's main.c file.
 */
#ifndef PROJECT_H
#define	PROJECT_H

#ifdef	__cplusplus
extern "C" {
#endif

// Common
#include "../config.h"
#include "lib/ascii.h"
#include "lib/common.h"
// Modules
#include "modules/bm78.h"
#include "modules/dht11.h"
#include "modules/i2c.h"
#include "modules/lcd.h"
#include "modules/mcp23017.h"
#include "modules/memory.h"
#include "modules/state_machine.h"
#include "modules/ws281x.h"
// Components
#include "components/bm78_pairing.h"
#include "components/bm78_communication.h"
#include "components/poc.h"
#include "components/state_machine_interaction.h"
#include "components/state_machine_transfer.h"
#include "components/setup_mode.h"
// MCC
#include "../mcc_generated_files/mcc.h"

#ifdef	__cplusplus
}
#endif

#endif	/* PROJECT_H */

