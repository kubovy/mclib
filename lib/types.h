/* 
 * File:   types.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */

#ifndef TYPES_H
#define	TYPES_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef void (*Procedure_t)(void);
typedef uint8_t (*Provider_t)();
typedef void (*Consumer_t)(uint8_t);
typedef uint8_t (*Function_t)(uint8_t);
typedef bool (*Condition_t)();

#ifdef	__cplusplus
}
#endif

#endif	/* TYPES_H */

