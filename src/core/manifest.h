/*
 * component.h
 *
 *  Created on: 12 May 2016
 *      Author: rad
 */

#ifndef MANIFEST_H_
#define MANIFEST_H_

#include "endpoint.h"
#include "array.h"
#include "json.h"

/* some sort of modest singleton struct */

typedef struct _COMPONENT{
	char *name;
	JSON *metadata;
}COMPONENT;

/* manifest length levels */
#define MANIFEST_SIMPLE		1 /* just md about cpt added with cpt_set_md */
#define MANIFEST_SHORT		2 /* md about cpt and app defined eps */
#define MANIFEST_FULL		3 /* all md including default eps */

/*
 * updates the component's manifest with a new json.
 */
int manifest_update(JSON* json);

/*
 * returns the JSON md.
 */
JSON* manifest_get(int lvl);

/*
 * short manifest
 */
JSON* manigest_get_short();

#endif /* MANIFEST_H_ */
