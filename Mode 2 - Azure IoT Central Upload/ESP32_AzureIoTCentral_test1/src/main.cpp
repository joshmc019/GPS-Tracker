/* --- Dependencies --- */
#include <Arduino.h>

// Local header files
#include "AzureIoT_SDK_C.h"
#include "AzureIoT.h"
#include "Azure_IoT_PnP_Template.h"
#include "iot_configs.h"

#include "SD_funcs.h"

// Variables to keep track of when previous data was sent to cloud
time_t now;
time_t last_send_time = 0;

// Function signature
int generate_payload(uint8_t* buff, size_t buff_size, size_t* buff_len);

// Pin definitions for SPI - used to connect to SD card
#define SPI_CLK 18
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_CS 5

SPIClass spi(VSPI);
#define SD_SPEED 80000000

String lines[100];
int lines_index = 0;
int lines_sent = 0;

// Variables to store the values read frmo the SD card
float latitude;
float longitude;
float altitude;
float velocity;

const int coord_accuracy = 6;
const int alt_accuracy = 2;
const int velo_accuracy = 2;

float rng_lat;
float rng_lon;
float rng_alt;
float rng_velo;

void setup()
{
  Serial.begin(SERIAL_LOGGER_BAUD_RATE);

  // Initialize Azure IoT Central communication -- from Microsoft Documentation/Examples
  set_logging_function(logging_function);

  connect_to_wifi();
  sync_device_clock_with_ntp_server();

  azure_pnp_init();

  configure_azure_iot();
  azure_iot_start(&azure_iot);

  LogInfo("Azure IoT client initialized (state=%d)", azure_iot.state);

  azure_iot_do_work(&azure_iot);

  delay(5000);

  // Connect to SD card
  SD.begin(SPI_CS, spi, SD_SPEED);
  SD_printType(SD);
  SD_printSize(SD);
  SD_printRootDir(SD);

  // Read all lines of a file stored on the SD card into lines array
  File file = SD.open("/data1.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open data file for cloud upload");
  } else {
    while (file.available()) {
      String line = file.readStringUntil('\n');
      Serial.println(line);

      lines[lines_index] = line;
      lines_index++;
    }

    
  }

}

void loop()
{

  /*

  // The following is used to randomly generate expected data points (used for testing purposes)

  const float lat_upperBound = 33.370605;
  const float lat_lowerBound = 33.367662;
  const float lon_upperBound = 86.853339;
  const float lon_lowerBound = 86.851778;
  const float coord_mult = 1000000.0;

  const float alt_lowerBound = 200.0;
  const float alt_upperBound = 300.0;
  const float alt_mult = 100.0;

  const float velo_lowerBound = 0.0;
  const float velo_upperBound = 50.0;
  const float velo_mult = 100.0;

  rng_lat = (random(lat_lowerBound * coord_mult, lat_upperBound * coord_mult) / coord_mult);
  rng_lon = (-1.0) * (random(lon_lowerBound * coord_mult, lon_upperBound * coord_mult) / coord_mult);
  rng_alt = (random(alt_lowerBound * alt_mult, alt_upperBound * alt_mult) / alt_mult);
  rng_velo = (random(velo_lowerBound * velo_mult, velo_upperBound * velo_mult) / velo_mult);

  latitude = rng_lat;
  longitude = rng_lon;
  altitude = rng_alt;
  velocity = rng_velo;

  */

  
  // Get data from the current line
  sscanf(lines[lines_sent].c_str(), "%f,%f,%f,%f", &latitude, &longitude, &altitude, &velocity);

  // Initialize variables for the data buffer
  size_t b_size = 1024;
  uint8_t b[b_size];
  size_t b_len;
  int result;

  if (WiFi.status() != WL_CONNECTED)
  {
    azure_iot_stop(&azure_iot);
    
    connect_to_wifi();
    
    if (!azure_initial_connect)
    {
      configure_azure_iot();
    }
    
    azure_iot_start(&azure_iot);
  }
  else
  {
    switch (azure_iot_get_status(&azure_iot))
    {
      case azure_iot_connected:
        // Send each line of data from the SD card to the Azure database
        if (lines_sent < lines_index) {
          azure_initial_connect = true;

          now = time(NULL);
          if (difftime(now, last_send_time) >= TELEMETRY_FREQUENCY_IN_SECONDS) {  // Wait for 5 seconds
            result = generate_payload(b, b_size, &b_len);
            Serial.println();
            for (int i = 0; i < b_len; i++) {
              Serial.print(char(b[i]));
            }
            Serial.println();
            Serial.println();

            // Send the data buffer to the Azure database
            if (azure_iot_send_telemetry(&azure_iot, az_span_create(b, b_len)) != 0) {
              LogError("Failed sending telemetry from main loop.");
            }

            lines_sent++;

            last_send_time = now;
          }
        }

        // if (send_device_info)
        // {
        //   (void)azure_pnp_send_device_info(&azure_iot, properties_request_id++);
        //   send_device_info = false; // Only need to send once.
        // }
        // else if (azure_pnp_send_telemetry(&azure_iot) != 0)
        // {
        //   LogError("Failed sending telemetry.");
        // }

        break;
        
      case azure_iot_error:
        LogError("Azure IoT client is in error state.");
        azure_iot_stop(&azure_iot);
        break;
        
      case azure_iot_disconnected:
        WiFi.disconnect();
        break;
        
      default:
        break;
    }

    azure_iot_do_work(&azure_iot);
  }
}

// Following the same pattern from the Example, customize the JSON packet to include all necessary values in the correct format
int generate_payload(uint8_t* buff, size_t buff_size, size_t* buff_len) {
  // Initialize the JSON writer
  az_json_writer jw;
  az_result rc;
  az_span buff_span = az_span_create(buff, buff_size);
  az_span json_span;

  // Append each necessary element to the JSON packet
  rc = az_json_writer_init(&jw, buff_span, NULL);
  rc = az_json_writer_append_begin_object(&jw);
  rc = az_iot_hub_client_properties_writer_begin_component(NULL, &jw, AZ_SPAN_FROM_STR("Tracking"));
  // rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR("Tracking"));
  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR("lat"));
  rc = az_json_writer_append_double(&jw, latitude, coord_accuracy);
  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR("lon"));
  rc = az_json_writer_append_double(&jw, longitude, coord_accuracy);
  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR("alt"));
  rc = az_json_writer_append_double(&jw, altitude, alt_accuracy);
  rc = az_iot_hub_client_properties_writer_end_component(NULL, &jw);
  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR("velo"));
  rc = az_json_writer_append_double(&jw, velocity, velo_accuracy);
  rc = az_json_writer_append_end_object(&jw);

  // Update the data buffer
  buff_span = az_json_writer_get_bytes_used_in_destination(&jw);

  buff[az_span_size(buff_span)] = null_terminator;
  *buff_len = az_span_size(buff_span);

  return RESULT_OK;
}

// ============ Microsoft Azure IoT Central functions ============
static void sync_device_clock_with_ntp_server()
{
  LogInfo("Setting time using SNTP");

  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < UNIX_TIME_NOV_13_2017)
  {
    delay(500);
    Serial.print(".");
    now = time(NULL);
  }
  Serial.println("");
  LogInfo("Time initialized!");
}

static void connect_to_wifi()
{
  LogInfo("Connecting to WIFI wifi_ssid %s", wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");

  LogInfo("WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
}

static esp_err_t esp_mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  switch (event->event_id)
  {
    int i, r;

    case MQTT_EVENT_ERROR:
      LogError("MQTT client in ERROR state.");
      LogError(
          "esp_tls_stack_err=%d; "
          "esp_tls_cert_verify_flags=%d;esp_transport_sock_errno=%d;error_type=%d;connect_return_"
          "code=%d",
          event->error_handle->esp_tls_stack_err,
          event->error_handle->esp_tls_cert_verify_flags,
          event->error_handle->esp_transport_sock_errno,
          event->error_handle->error_type,
          event->error_handle->connect_return_code);

      switch (event->error_handle->connect_return_code)
      {
        case MQTT_CONNECTION_ACCEPTED:
          LogError("connect_return_code=MQTT_CONNECTION_ACCEPTED");
          break;
        case MQTT_CONNECTION_REFUSE_PROTOCOL:
          LogError("connect_return_code=MQTT_CONNECTION_REFUSE_PROTOCOL");
          break;
        case MQTT_CONNECTION_REFUSE_ID_REJECTED:
          LogError("connect_return_code=MQTT_CONNECTION_REFUSE_ID_REJECTED");
          break;
        case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
          LogError("connect_return_code=MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE");
          break;
        case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
          LogError("connect_return_code=MQTT_CONNECTION_REFUSE_BAD_USERNAME");
          break;
        case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
          LogError("connect_return_code=MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED");
          break;
        default:
          LogError("connect_return_code=unknown (%d)", event->error_handle->connect_return_code);
          break;
      };

      break;
    case MQTT_EVENT_CONNECTED:
      LogInfo("MQTT client connected (session_present=%d).", event->session_present);

      if (azure_iot_mqtt_client_connected(&azure_iot) != 0)
      {
        LogError("azure_iot_mqtt_client_connected failed.");
      }

      break;
    case MQTT_EVENT_DISCONNECTED:
      LogInfo("MQTT client disconnected.");

      if (azure_iot_mqtt_client_disconnected(&azure_iot) != 0)
      {
        LogError("azure_iot_mqtt_client_disconnected failed.");
      }

      break;
    case MQTT_EVENT_SUBSCRIBED:
      LogInfo("MQTT topic subscribed (message id=%d).", event->msg_id);

      if (azure_iot_mqtt_client_subscribe_completed(&azure_iot, event->msg_id) != 0)
      {
        LogError("azure_iot_mqtt_client_subscribe_completed failed.");
      }

      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      LogInfo("MQTT topic unsubscribed.");
      break;
    case MQTT_EVENT_PUBLISHED:
      LogInfo("MQTT event MQTT_EVENT_PUBLISHED");

      if (azure_iot_mqtt_client_publish_completed(&azure_iot, event->msg_id) != 0)
      {
        LogError("azure_iot_mqtt_client_publish_completed failed (message id=%d).", event->msg_id);
      }

      break;
    case MQTT_EVENT_DATA:
      LogInfo("MQTT message received.");

      mqtt_message_t mqtt_message;
      mqtt_message.topic = az_span_create((uint8_t*)event->topic, event->topic_len);
      mqtt_message.payload = az_span_create((uint8_t*)event->data, event->data_len);
      mqtt_message.qos
          = mqtt_qos_at_most_once; // QoS is unused by azure_iot_mqtt_client_message_received.

      if (azure_iot_mqtt_client_message_received(&azure_iot, &mqtt_message) != 0)
      {
        LogError(
            "azure_iot_mqtt_client_message_received failed (topic=%.*s).",
            event->topic_len,
            event->topic);
      }

      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      LogInfo("MQTT client connecting.");
      break;
    default:
      LogError("MQTT event UNKNOWN.");
      break;
  }

  return ESP_OK;
}

static void logging_function(log_level_t log_level, char const* const format, ...)
{
  struct tm* ptm;
  time_t now = time(NULL);

  ptm = gmtime(&now);

  Serial.print(ptm->tm_year + UNIX_EPOCH_START_YEAR);
  Serial.print("/");
  Serial.print(ptm->tm_mon + 1);
  Serial.print("/");
  Serial.print(ptm->tm_mday);
  Serial.print(" ");

  if (ptm->tm_hour < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_hour);
  Serial.print(":");

  if (ptm->tm_min < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_min);
  Serial.print(":");

  if (ptm->tm_sec < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_sec);

  Serial.print(log_level == log_level_info ? " [INFO] " : " [ERROR] ");

  char message[256];
  va_list ap;
  va_start(ap, format);
  int message_length = vsnprintf(message, 256, format, ap);
  va_end(ap);

  if (message_length < 0)
  {
    Serial.println("Failed encoding log message (!)");
  }
  else
  {
    Serial.println(message);
  }
}