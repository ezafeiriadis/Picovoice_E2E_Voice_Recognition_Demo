/*
    Copyright 2021 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
    file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
    specific language governing permissions and limitations under the License.
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cy_rgb_led.h"
#include "cy_pdl.h"
#include "cybsp.h"
#include "cyhal.h"

#include "pv_audio_rec.h"
#include "pv_keywords.h"
#include "pv_picovoice.h"
#include "pv_psoc6.h"
#include "faceMe.h"
#include "turnDegrees.h"
#include "comeCloser.h"
#include "degreesToDutyCycle.h"

#define MEMORY_BUFFER_SIZE (70 * 1024)

static const char* ACCESS_KEY = "euUgfv3MUOlJk9OvEw8ihc5eMMhoX9OcgTZvx3L5fXh5f6oqhTB3Nw=="; // AccessKey string obtained from Picovoice Console (https://picovoice.ai/console/)
static const char* LEFT = "left";

static int8_t memory_buffer[MEMORY_BUFFER_SIZE] __attribute__((aligned(16)));

static const float PORCUPINE_SENSITIVITY = 0.75f;
static const float RHINO_SENSITIVITY = 0.5f;

// This is the PWM initialization end
cy_rslt_t result;
cyhal_pwm_t pwm_obj_d9;
cyhal_pwm_t pwm_obj_d10;
cyhal_pwm_t pwm_obj_d11;

int current_position;

static void wake_word_callback(void) {
    printf("[wake word]\r\n");
    cy_rgb_led_on(CY_RGB_LED_COLOR_GREEN, CY_RGB_LED_MAX_BRIGHTNESS);
}

float convertDegreesToDutyCycle(float degrees){
    return degrees / 180 * 100;
}

void pwm_init(){
	/* Initialize PWM on the supplied pin and assign a new clock */
	result = cyhal_pwm_init(&pwm_obj_d9, CYBSP_D9, NULL);
	result = cyhal_pwm_init(&pwm_obj_d10, CYBSP_D10, NULL);
	result = cyhal_pwm_init(&pwm_obj_d11, CYBSP_D11, NULL);
}

static void inference_callback(pv_inference_t *inference) {
    cy_rgb_led_on(CY_RGB_LED_COLOR_BLUE, CY_RGB_LED_MAX_BRIGHTNESS);

    printf("{\r\n");
    printf("\tis_understood: '%s',\r\n", (inference->is_understood ? "true" : "false"));

    int rotation = 0;

    if (inference->is_understood) {

        if (strcmp(inference->intent, "goHome") == 0){
        	// for smaller turning angles the TV wall mount would have to be fully folded

        	// TV motor
			result = cyhal_pwm_start(&pwm_obj_d9);
			result = cyhal_pwm_set_duty_cycle(&pwm_obj_d9, 7, 50);
			// Bottom motor
			result = cyhal_pwm_start(&pwm_obj_d11);
			result = cyhal_pwm_set_duty_cycle(&pwm_obj_d11, 5, 50);
			// Top motor
			result = cyhal_pwm_start(&pwm_obj_d10);
			result = cyhal_pwm_set_duty_cycle(&pwm_obj_d10, 7, 50);
        }
        else if (strcmp(inference->intent, "comeCloser") == 0){
        	// Bottom motor
			result = cyhal_pwm_start(&pwm_obj_d11);
			result = cyhal_pwm_set_duty_cycle(&pwm_obj_d11, 2, 50);
			// Top motor
			result = cyhal_pwm_start(&pwm_obj_d10);
			result = cyhal_pwm_set_duty_cycle(&pwm_obj_d10, 5, 50);
		}
        else if (strcmp(inference->intent, "turnPositionRot") == 0){

			// If rotation is 'left', then direction is 1, if it is 'right', then direction is -1
			// so that the servo moves the current position left or right
			if (strcmp(inference->values[1], LEFT)){
				rotation = 1;
			}
			else{
				rotation = -1;
			}
			// Picovoice returns a string value that we have to convert to an integer (that's what turnDegrees does)
			int number_of_positions = turnDegrees(inference->values[0]);

			current_position = current_position + rotation * number_of_positions;

			if (current_position > 12){
				// If the current position is 12 and someone says move 2 positions right,
				// then the current position would be 14 which is not a legal move
				current_position = 12;
			}
			else if (current_position <= 0){
				// If the current position is 1 and someone says move 2 positions left,
				// then the current position would be -1 which is not a legal move
				current_position = 1;
			}

			// This is the PWM start
			/* Start the PWM output */

			result = cyhal_pwm_start(&pwm_obj_d9);

			result = cyhal_pwm_set_duty_cycle(&pwm_obj_d9, current_position, 50);

			// This is the PWM end
        }
        else if (strcmp(inference->intent, "turnToPosition") == 0){

			int position = turnDegrees(inference->values[0]);

			// If the user wants the TV to move to an illegal position > 12 then we move
			// the TV to the largest legal position which is position 12
			if (position > 11){
				position = 11;
			}

			// This is the PWM start
			/* Start the PWM output */
			if (position >= 9 || position <= 3){
				// for large turning angles the TV wall mount would have to be fully unfolded
				// in order for the TV not to hit the wall
				// Bottom motor
				result = cyhal_pwm_start(&pwm_obj_d11);
				result = cyhal_pwm_set_duty_cycle(&pwm_obj_d11, 3, 50);
				// Top motor
				result = cyhal_pwm_start(&pwm_obj_d10);
				result = cyhal_pwm_set_duty_cycle(&pwm_obj_d10, 5, 50);
			}
			else{
				// for smaller turning angles the TV wall mount would have to be partially unfolded
				// in order for the TV not to hit the wall
				// Bottom motor
				result = cyhal_pwm_start(&pwm_obj_d11);
				result = cyhal_pwm_set_duty_cycle(&pwm_obj_d11, 5, 50);
				// Top motor
				result = cyhal_pwm_start(&pwm_obj_d10);
				result = cyhal_pwm_set_duty_cycle(&pwm_obj_d10, 7, 50);
			}

			// The Servo we chose can only move in steps from Duty Cycle = 2-12, so for us
			// min position 1--> = Duty Cycle 2 and max position 11 --> Duty Cycle = 12
			position = position + 1;

			current_position = position;

			result = cyhal_pwm_start(&pwm_obj_d9);

			result = cyhal_pwm_set_duty_cycle(&pwm_obj_d9, position, 50);
			// This is the PWM end

            }

        }

    for (int32_t i = 0; i < 10; i++) {
        if (cy_rgb_led_get_brightness() == 0) {
            cy_rgb_led_set_brightness(CY_RGB_LED_MAX_BRIGHTNESS);
        } else {
            cy_rgb_led_set_brightness(0);
        }
        Cy_SysLib_Delay(30);
    }
    cy_rgb_led_off();

    pv_inference_delete(inference);
}

static void error_handler(void) {
    while(true);
}

int main(void) {
    pv_status_t status = pv_board_init();
    pwm_init();
    if (status != PV_STATUS_SUCCESS) {
        error_handler();
    }

    status = pv_message_init();
    if (status != PV_STATUS_SUCCESS) {
        error_handler();
    }

    const uint8_t *board_uuid = pv_get_uuid();
    printf("UUID: ");
    for (uint32_t i = 0; i < pv_get_uuid_size(); i++) {
        printf(" %.2x", board_uuid[i]);
    }
    printf("\r\n");

    status = pv_audio_rec_init();
    if (status != PV_STATUS_SUCCESS) {
        printf("Audio init failed with '%s'\r\n", pv_status_to_string(status));
        error_handler();
    }

    status = pv_audio_rec_start();
    if (status != PV_STATUS_SUCCESS) {
        printf("Recording audio failed with '%s'\r\n", pv_status_to_string(status));
        error_handler();
    }

    pv_picovoice_t *handle = NULL;

    status = pv_picovoice_init(
            ACCESS_KEY,
            MEMORY_BUFFER_SIZE,
            memory_buffer,
            sizeof(KEYWORD_ARRAY),
            KEYWORD_ARRAY,
            PORCUPINE_SENSITIVITY,
            wake_word_callback,
            sizeof(CONTEXT_ARRAY),
            CONTEXT_ARRAY,
            RHINO_SENSITIVITY,
            true,
            inference_callback,
            &handle);
    if (status != PV_STATUS_SUCCESS) {
        printf("Picovoice init failed with '%s'\r\n", pv_status_to_string(status));
        error_handler();
    }

    while (true) {
        const int16_t *buffer = pv_audio_rec_get_new_buffer();
        if (buffer) {
            const pv_status_t status = pv_picovoice_process(handle, buffer);
            if (status != PV_STATUS_SUCCESS) {
                printf("Picovoice process failed with '%s'\r\n", pv_status_to_string(status));
                error_handler();
            }
        }

    }
    pv_board_deinit();
    pv_audio_rec_deinit();
    pv_picovoice_delete(handle);
}
