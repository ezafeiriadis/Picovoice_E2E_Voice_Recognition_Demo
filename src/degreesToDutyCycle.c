/*
 * degreesToDutyCycle.c
 *
 *  Created on: 29.04.2022
 *      Author: petridis
 */

// This function converts degrees (0-180 degrees) to the duty cycle (0-100%)
int convertDegreesToPercentage(int degrees){
    int percentage = degrees / 180.0 * 100;
    return percentage;
}
