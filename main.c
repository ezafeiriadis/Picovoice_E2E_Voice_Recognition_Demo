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
#include "cybsp.h"

#include "pv_audio_rec.h"
#include "pv_keywords.h"
#include "pv_picovoice.h"
#include "pv_psoc6.h"
#include "faceMe.h"
#include "turnDegrees.h"
#include "comeCloser.h"

#define MEMORY_BUFFER_SIZE (70 * 1024)

static const char* ACCESS_KEY = "euUgfv3MUOlJk9OvEw8ihc5eMMhoX9OcgTZvx3L5fXh5f6oqhTB3Nw=="; // AccessKey string obtained from Picovoice Console (https://picovoice.ai/console/)
static const char* LEFT = "left";

static int8_t memory_buffer[MEMORY_BUFFER_SIZE] __attribute__((aligned(16)));

static const float PORCUPINE_SENSITIVITY = 0.75f;
static const float RHINO_SENSITIVITY = 0.5f;

static void wake_word_callback(void) {
    printf("[wake word]\r\n");
    cy_rgb_led_on(CY_RGB_LED_COLOR_GREEN, CY_RGB_LED_MAX_BRIGHTNESS);
}

static void inference_callback(pv_inference_t *inference) {
    cy_rgb_led_on(CY_RGB_LED_COLOR_BLUE, CY_RGB_LED_MAX_BRIGHTNESS);

    printf("{\r\n");
    printf("\tis_understood: '%s',\r\n", (inference->is_understood ? "true" : "false"));

    int rotation = 0;

    if (inference->is_understood) {
        printf("\tintent : '%s',\r\n", inference->intent);
        if (strcmp(inference->intent, "faceMe") == 0){
        	faceMe();
        }
        else if (strcmp(inference->intent, "comeCloser") == 0){
			comeCloser();
		}
        else if (strcmp(inference->intent, "turnDegrees") == 0){

            printf("\tslots : {\r\n");
			printf("\t\t'%s' : '%s',\r\n", inference->slots[0], inference->values[0]);
			printf("\t\t'%s' : '%s',\r\n", inference->slots[1], inference->values[1]);

			// If rotation is 'left', then direction is 1, if it is 'right', then direction is 0
			if (strcmp(inference->values[1], LEFT)){
				rotation = 0;
			}
			else{
				rotation = 1;
			}

			turnDegrees(inference->values[0], rotation);

            }
            printf("\t}\r\n");
        }
    printf("}\r\n\n");

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
