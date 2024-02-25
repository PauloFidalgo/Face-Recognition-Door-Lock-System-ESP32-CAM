
#include "esp_camera.h"
#include "img_converters.h"
#include "camera_index.h"
#include "Arduino.h"

#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"

// Number of samples per person
#define ENROLL_CONFIRM_TIMES 10

// Maximum number of enrolled people
#define FACE_ID_SAVE_NUMBER 20

#define LED_BUILTIN 4
#define RELAY_PIN 12 

extern int pessoas;
extern int enrolled;
static mtmn_config_t mtmn_config = {0};
static face_id_list id_list = {0};

static void open()
{
  digitalWrite(RELAY_PIN,LOW);
  delay(5000);
  digitalWrite(RELAY_PIN,HIGH);
}

static int run_face_recognition(dl_matrix3du_t *image_matrix, box_array_t *net_boxes){
    dl_matrix3du_t *aligned_face = NULL;
    int matched_id = 0;

    aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
    if(!aligned_face){
        Serial.println("Could not allocate face recognition buffer");
        dl_matrix3du_free(aligned_face);
        return matched_id;
    }
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK){
        if (enrolled > 0){
            enrolled--;
            int8_t left_sample_face = enroll_face(&id_list, aligned_face);

            if(left_sample_face == (ENROLL_CONFIRM_TIMES-1)){
                Serial.printf("Enrolling Face ID: %d\n", id_list.tail);
            }
            Serial.printf("Enrolling Face ID: %d sample %d\n", id_list.tail, ENROLL_CONFIRM_TIMES - left_sample_face);
            if (left_sample_face == 0){
                Serial.printf("Enrolled Face ID: %d\n", id_list.tail);
            }

        } else {
            matched_id = recognize_face(&id_list, aligned_face);
            if (matched_id >= 0) {
                Serial.printf("Match Face ID: %u\n", matched_id);
                open();
            } else {
                Serial.println("No Match Found");
                /*
                digitalWrite(LED_BUILTIN,HIGH);
                delay(500);
                digitalWrite(LED_BUILTIN,LOW);
                digitalWrite(LED_BUILTIN,HIGH);
                delay(500);
                digitalWrite(LED_BUILTIN,LOW);
                digitalWrite(LED_BUILTIN,HIGH);
                delay(500);
                digitalWrite(LED_BUILTIN,LOW);
                */
                matched_id = -1;
            }
        }
    } else {
        Serial.println("Face Not Aligned");
    }

    dl_matrix3du_free(aligned_face);
    return matched_id;
}

static void del_all()
{
  if (pessoas > 0)
  {
    while (pessoas > 0)
    {
      delete_face(&id_list);
      pessoas--;
    }
  }
}

static void del_last()
{
  if (pessoas > 0)
  {
    pessoas--;
    delete_face(&id_list);
    Serial.printf("Still %d left!\n", pessoas);
  }
}

