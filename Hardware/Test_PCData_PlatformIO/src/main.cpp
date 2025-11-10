#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// --- Global Variables ---
struct PCData
{
  char cpuName[64];
  char gpuName[64];
  float cpuLoad;
  float cpuTemp;
  float gpuLoad;
  float gpuTemp;
  float ramLoad;
  bool valid;
};

PCData pcData;

// Serial receive buffer
#define BUFFER_SIZE 256
char inputBuffer[BUFFER_SIZE];
uint16_t bufferIndex = 0;
bool stringComplete = false;

// Display position definitions
#define TITLE_Y 10
#define RAW_DATA_Y 40
#define PARSED_DATA_Y 90
#define CURRENT_DATA_Y 160
#define LINE_HEIGHT 20
#define COL2_X 120

/**
 * @brief Initialize PCData structure
 */
void PCData_Init(PCData *pcdata)
{
  memset(pcdata, 0, sizeof(*pcdata));
  strncpy(pcdata->cpuName, "Intel Core i7-14650HX", sizeof(pcdata->cpuName) - 1);
  strncpy(pcdata->gpuName, "NVIDIA GeForce RTX 4060", sizeof(pcdata->gpuName) - 1);
  pcdata->valid = false;
}

/**
 * @brief Reset serial receive buffer
 */
void resetBuffer()
{
  bufferIndex = 0;
  inputBuffer[0] = '\0';
}

/**
 * @brief Clear screen area
 */
void clearArea(int16_t y, int16_t height)
{
  tft.fillRect(0, y, tft.width(), height, TFT_BLACK);
}

/**
 * @brief Display text at specified position
 */
void displayText(int16_t x, int16_t y, const String &text, uint16_t color = TFT_WHITE, uint8_t size = 1)
{
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(size);
  tft.setCursor(x, y);
  tft.print(text);
}

/**
 * @brief Display title and static elements
 */
void drawStaticElements()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(1);

  // Display title
  displayText(10, TITLE_Y, "Performance Monitor Test", TFT_CYAN, 2);
  displayText(10, TITLE_Y + 25, "Waiting for data...", TFT_GREEN, 1);

  // Draw separator lines
  tft.drawLine(0, RAW_DATA_Y - 5, tft.width(), RAW_DATA_Y - 5, TFT_DARKGREY);
  tft.drawLine(0, PARSED_DATA_Y - 5, tft.width(), PARSED_DATA_Y - 5, TFT_DARKGREY);
  tft.drawLine(0, CURRENT_DATA_Y - 5, tft.width(), CURRENT_DATA_Y - 5, TFT_DARKGREY);
}

/**
 * @brief Parse PC performance data from serial
 */
void parsePCData()
{
  PCData parsedValues;
  memset(&parsedValues, 0, sizeof(parsedValues));
  parsedValues.valid = false;

  char *ptr;

  // Clear parsed data display area
  clearArea(PARSED_DATA_Y, 70);

  // Display raw data
  displayText(10, RAW_DATA_Y, "Raw Data:", TFT_YELLOW, 1);
  displayText(COL2_X, RAW_DATA_Y, String(inputBuffer), TFT_WHITE, 1);

  bool dataFound = false;

  // Parse CPU load
  ptr = strstr(inputBuffer, "CCc ");
  if (ptr)
  {
    parsedValues.cpuLoad = atoi(ptr + 4);
    displayText(10, PARSED_DATA_Y, "CPU Load:", TFT_GREEN, 1);
    displayText(COL2_X, PARSED_DATA_Y, String(parsedValues.cpuLoad) + "%", TFT_GREEN, 1);
    dataFound = true;
  }

  // Parse GPU data
  ptr = strstr(inputBuffer, "G");
  if (ptr && ptr[1] >= '0' && ptr[1] <= '9')
  {
    char *cPos = strchr(ptr, 'c');
    if (cPos)
    {
      parsedValues.gpuTemp = atoi(ptr + 1);
      parsedValues.gpuLoad = atoi(cPos + 1);
      displayText(10, PARSED_DATA_Y + LINE_HEIGHT, "GPU Temp:", TFT_BLUE, 1);
      displayText(COL2_X, PARSED_DATA_Y + LINE_HEIGHT, String(parsedValues.gpuTemp) + "C", TFT_BLUE, 1);
      displayText(10, PARSED_DATA_Y + 2 * LINE_HEIGHT, "GPU Load:", TFT_BLUE, 1);
      displayText(COL2_X, PARSED_DATA_Y + 2 * LINE_HEIGHT, String(parsedValues.gpuLoad) + "%", TFT_BLUE, 1);
      dataFound = true;
    }
  }

  // Parse RAM usage
  ptr = strstr(inputBuffer, "RL");
  if (ptr)
  {
    parsedValues.ramLoad = atof(ptr + 2);
    displayText(10, PARSED_DATA_Y + 3 * LINE_HEIGHT, "RAM Usage:", TFT_RED, 1);
    displayText(COL2_X, PARSED_DATA_Y + 3 * LINE_HEIGHT, String(parsedValues.ramLoad, 1) + "%", TFT_RED, 1);
    dataFound = true;
  }

  // Update global data
  if (dataFound)
  {
    pcData = parsedValues;
    pcData.valid = true;
    displayText(10, PARSED_DATA_Y + 4 * LINE_HEIGHT, "Status: Valid Data", TFT_GREEN, 1);
  }
  else
  {
    displayText(10, PARSED_DATA_Y + 4 * LINE_HEIGHT, "Status: Invalid Data", TFT_RED, 1);
  }
}

/**
 * @brief Display current parsed data
 */
void displayCurrentData()
{
  static unsigned long lastDisplay = 0;

  if (millis() - lastDisplay >= 1000)
  { // Update every 1 second
// Clear current data display area
    clearArea(CURRENT_DATA_Y, 80);

    displayText(10, CURRENT_DATA_Y, "Current Data:", TFT_CYAN, 1);

    if (pcData.valid)
    {
      displayText(10, CURRENT_DATA_Y + LINE_HEIGHT, "CPU: " + String(pcData.cpuLoad) + "%", TFT_GREEN, 1);
      displayText(10, CURRENT_DATA_Y + 2 * LINE_HEIGHT, "GPU: " + String(pcData.gpuLoad) + "%", TFT_BLUE, 1);
      displayText(COL2_X, CURRENT_DATA_Y + LINE_HEIGHT, "GPU Temp: " + String(pcData.gpuTemp) + "C", TFT_BLUE, 1);
      displayText(COL2_X, CURRENT_DATA_Y + 2 * LINE_HEIGHT, "RAM: " + String(pcData.ramLoad, 1) + "%", TFT_RED, 1);
    }
    else
    {
      displayText(10, CURRENT_DATA_Y + LINE_HEIGHT, "No valid data", TFT_ORANGE, 1);
    }

    // Display update time
    displayText(10, CURRENT_DATA_Y + 4 * LINE_HEIGHT, "Update: " + String(millis() / 1000) + "s", TFT_WHITE, 1);

    lastDisplay = millis();
  }
}

/**
 * @brief Handle serial data reception
 */
void serialReceiveTask()
{
  static unsigned long lastCharTime = 0;

  if (Serial.available())
  {
    char inChar = (char) Serial.read();

    if (bufferIndex < BUFFER_SIZE - 1)
    {
      inputBuffer[bufferIndex++] = inChar;
      inputBuffer[bufferIndex] = '\0';
    }
    lastCharTime = millis();

    if (inChar == '\n' || inChar == '\r')
    {
      stringComplete = true;
    }
  }

  // 50ms timeout detection
  if (!stringComplete && bufferIndex > 0 && (millis() - lastCharTime > 50))
  {
    stringComplete = true;
  }

  if (stringComplete)
  {
    // Clear raw data area
    clearArea(RAW_DATA_Y, 40);

    parsePCData();

    stringComplete = false;
    resetBuffer();
  }
}

/**
 * @brief Arduino setup function
 */
void setup()
{
  Serial.begin(115200);

  // Initialize TFT
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  PCData_Init(&pcData);
  drawStaticElements();

  Serial.println("Performance Monitor Test Started");
  Serial.println("Send data in format: CCc XX GYYcZZ RLWW.W");
}

/**
 * @brief Arduino loop function
 */
void loop()
{
  // Handle serial data reception
  serialReceiveTask();

  // Display current parsed data
  displayCurrentData();

  delay(10);
}