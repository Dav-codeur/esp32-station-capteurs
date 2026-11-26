#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht.h"

#define PIN_PIR    GPIO_NUM_5
#define PIN_LIGNE  GPIO_NUM_6
#define PIN_DHT    GPIO_NUM_7
#define CANAL_SOL  ADC_CHANNEL_3        // GPIO4 sur l'ESP32-S3 (ADC1)

#define SOL_SEC        2800             // au-dessus : sol sec (a calibrer)
#define SOL_MIN_VALIDE 100              // en dessous : alimentation coupee ou capteur HS
#define SOL_MAX_VALIDE 4000             // au-dessus : valeur impossible

static const char *TAG_SOL   = "sol";
static const char *TAG_PIR   = "pir";
static const char *TAG_LIGNE = "ligne";
static const char *TAG_DHT   = "dht";
static const char *TAG_ETAT  = "etat";

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_DEBUG);   // affiche aussi les messages Debug

    // Entrees numeriques : PIR et suiveur de ligne
    gpio_config_t entrees = {
        .pin_bit_mask = (1ULL << PIN_PIR) | (1ULL << PIN_LIGNE),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&entrees);

    // Entree analogique : capteur d'humidite du sol
    adc_oneshot_unit_handle_t adc;
    adc_oneshot_unit_init_cfg_t unite = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unite, &adc));
    adc_oneshot_chan_cfg_t canal = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, CANAL_SOL, &canal));

    ESP_LOGI(TAG_SOL, "Station demarree");

    int pir_avant = -1, ligne_avant = -1;
    int tour = 0;

    while (1) {
        // PIR et ligne : on log les CHANGEMENTS
        int pir = gpio_get_level(PIN_PIR);
        int ligne = gpio_get_level(PIN_LIGNE);
        ESP_LOGI(TAG_ETAT, "pir=%d ligne=%d", pir, ligne);

        if (pir != pir_avant) {
            if (pir) ESP_LOGW(TAG_PIR, "Mouvement detecte");
            else     ESP_LOGI(TAG_PIR, "Plus de mouvement");
            pir_avant = pir;
        }
        if (ligne != ligne_avant) {
            ESP_LOGI(TAG_LIGNE, "Etat du capteur : %d", ligne);
            ligne_avant = ligne;
        }

        // Humidite du sol : toutes les 2 s (tours 0, 4, 8...)
        if (tour % 4 == 0) {
            int brut = 0;
            esp_err_t err = adc_oneshot_read(adc, CANAL_SOL, &brut);
            if (err != ESP_OK) {
                ESP_LOGE(TAG_SOL, "Lecture ADC echouee : %s", esp_err_to_name(err));
            } else {
                ESP_LOGD(TAG_SOL, "Valeur brute : %d", brut);
                if (brut < SOL_MIN_VALIDE || brut > SOL_MAX_VALIDE) {
                    ESP_LOGE(TAG_SOL, "Valeur %d hors plage (alim coupee ou capteur HS)", brut);
                } else if (brut > SOL_SEC) {
                    ESP_LOGW(TAG_SOL, "Sol sec (%d)", brut);
                } else {
                    ESP_LOGI(TAG_SOL, "Humidite correcte (%d)", brut);
                }
            }
        }

        // Temperature et humidite de l'air : toutes les 2 s (tours 2, 6, 10...)
        if (tour % 4 == 2) {
            float humidite, temperature;
            esp_err_t err = dht_read_float_data(DHT_TYPE_DHT11, PIN_DHT,
                                                &humidite, &temperature);
            if (err == ESP_OK) {
                ESP_LOGI(TAG_DHT, "Temperature %.0f C, humidite de l'air %.0f %%",
                         temperature, humidite);
            } else if (err == ESP_ERR_TIMEOUT) {
                ESP_LOGE(TAG_DHT, "Pas de reponse : capteur debranche ou mal cable");
            } else if (err == ESP_ERR_INVALID_CRC) {
                ESP_LOGW(TAG_DHT, "Donnees corrompues (somme de controle) : mesure ignoree");
            } else {
                ESP_LOGE(TAG_DHT, "Erreur : %s", esp_err_to_name(err));
            }
        }

        tour++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}