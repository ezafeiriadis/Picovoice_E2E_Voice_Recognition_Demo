/*
 * turnDegrees.h
 *
 *  Created on: 05.04.2022
 *      Author: petridis
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wordToNumber.h>

#ifndef INCLUDE_TURNDEGREES_H_
#define INCLUDE_TURNDEGREES_H_


void turnDegrees(const char* degrees_string, int rotation){

	for (int i = 0; i < 100; ++i)
	  {
	    if (strcmp(wordsToNumbersArray[i], degrees_string) == 0){
			printf("\nI found the Index: '%d'\n\n", i);
	    }
	  }
}

#endif /* INCLUDE_TURNDEGREES_H_ */
