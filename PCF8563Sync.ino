/*
 * 
 *  PCF8563Sync - v1.0
 *  Code by Gavin Tryzbiak
 *  Inspiration from Paul Stoffregen's DS1307RTC Library
 *  
 */

/*
 * 
 *  IMPORTANT:
 *  - The Serial Monitor should be set to "No line ending" in
 *    the dropdown menu at the bottom.
 *  - The Serial Monitor MUST match "#define BAUD_RATE XXXX"
 *    from line 40 in the dropdown menu of the Serial
 *    Monitor! By default it is 115200, but it can be set
 *    lower if your cable has a poor connection, or higher
 *    for quicker text output.
 *     
 *  NOTE:
 *  - This sketch will attempt to sync your PCF8563 to your
 *    computer's time. However, the time is pulled as soon
 *    as the sketch begins compiling. Compile time, sketch
 *    upload time, and I2C tramission time are not accounted
 *    for so the time on your PCF8563 may be a few seconds late.
 *  - You may need to change your register! Do this by
 *    changing "#define I2C_ADDRESS 0x51" from line 41 to whatever
 *    register your chip uses. This sketch does not support
 *    the serparate r/w address version of the chip.
 *  - You may think your chip has separate addresses, but
 *    use the "I2CScanner" sketch to check, as it may
 *    actually be a single-address chip.
 *    
 */

// Libraries
#include <Wire.h>

// Constants
#define BAUD_RATE 115200 // Match this with the number in the dropdown menu in the Serial Monitor
#define I2C_ADDRESS 0x51

#define SECONDS_REGISTER 0x02 // Also includes voltage status in MSB
#define MINUTES_REGISTER 0x03
#define HOURS_REGISTER 0x04
#define DAYS_REGISTER 0x05
#define WEEKDAYS_REGISTER 0x06
#define MONTHS_REGISTER 0x07 // Also includes century in MSB
#define YEARS_REGISTER 0x08

void setup()
{
  Serial.begin(BAUD_RATE);
  Wire.begin();
  autoSyncTime(); // autoSyncTime() and autoSyncDay() would be merged into one function, but __TIME__ and __DATE__ interfere when placed in the same function for some reason
  autoSyncDay();
  writeManualInfo();
  Serial.println(F("\nPCF8563 calibrated."));
}

void loop()
{
  displayDateTime();
  Serial.println(F("Enter \"M\" to set the time manually. Enter any other character to check the time again."));
  delay(3000);
  while(!Serial.available());
  if (tolower(Serial.readString()[0]) == 'm')
    manualSync();
}

byte DecToBCD(byte value) // Decimal to Binary Coded Decimal
{
  return ( (value / 10 * 16) + (value % 10) );
}

byte BCDToDec(byte value) // Binary Coded Decimal to Decimal
{
  return ( (value / 16 * 10) + (value % 16) );
}

void writeToAddress(byte I2CAddress, word registerAddress, byte data) // Writes data via I2C
{
  Wire.beginTransmission(I2CAddress);
  Wire.write(registerAddress);
  Wire.write(data);
  Wire.endTransmission();
}

byte * readFromAddress(byte I2CAddress, word beginningAddress, byte byteCount) // Note: variable pointing to this function should eventually be freed!
{
  byte * data = malloc(sizeof(byte)*byteCount);
  byte tempData;
  Wire.beginTransmission(I2CAddress);
  Wire.write(beginningAddress);
  Wire.endTransmission();
  Wire.requestFrom(I2CAddress, byteCount);
  for (byte i = 0; i < byteCount; i++)
    data[i] = Wire.read();
  return data;
}

byte getCurVStatus() // Returns the current voltage status of the PCF8563 in the MSB, without the current second
{
  byte * curSecondsRegister = readFromAddress(I2C_ADDRESS, SECONDS_REGISTER, 1);
  byte vStatus = *curSecondsRegister & 0b10000000;
  free(curSecondsRegister);
  return vStatus;
}

byte getCurSecond() // Returns the current second from the PCF8563, without the voltage status
{
  byte * curSecondsRegister = readFromAddress(I2C_ADDRESS, SECONDS_REGISTER, 1);
  byte curSecond = *curSecondsRegister & 0b01111111;
  free(curSecondsRegister);
  return curSecond;
}

byte monthToValue(char *str) // Heavily inspired by Paul Stoffregen's getDate() function in SetTime
{
  const char *monthNames[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  byte monthIndex;
  for (monthIndex = 0; monthIndex < 12; monthIndex++)
    if (strcmp(str, monthNames[monthIndex]) == 0)
      break;
  return ++monthIndex; 
}

byte writeManualInfo() // Weekday and PCF8563 voltage cannot easily be automatically determined
{
  Serial.println(F("What is the battery's current health status? Enter one of the following:\n - \"H\" : Healthy\n - \"L\" : Low/Unstable\n - \"X\" : Keep previous value"));
  while (!Serial.available());
  switch(Serial.readString()[0])
  {
    case 'h':
    case 'H':
      writeToAddress(I2C_ADDRESS, SECONDS_REGISTER, getCurSecond()); // Retrieves the current second with getCurSecond() and writes to the seconds register. getCurSecond() already returns the current second with the bitmask 0b01111111
      break;
    case 'l':
    case 'L':
      writeToAddress(I2C_ADDRESS, SECONDS_REGISTER, getCurSecond() | 0b10000000); // Retrieves the current second with getCurSecond(), changes the MSB to 1, and writes to the seconds register
      break;
  } // If "Keep previous value" is chosen, nothing is read or written in the seconds register
  
  Serial.println(F("\nWhat is the current weekday? Enter one of the following:\n - \"0\" : Sunday\n - \"1\" : Monday\n - \"2\" : Tuesday\n - \"3\" : Wednesday\n - \"4\" : Thursday\n - \"5\" : Friday\n - \"6\" : Saturday"));
  while (!Serial.available());
  writeToAddress(I2C_ADDRESS, WEEKDAYS_REGISTER, DecToBCD(Serial.parseInt()));
}

void autoSyncTime()
{
  byte curHour, curMinute, curSecond;

  // Acquire compile time date and time
  if (sscanf(__TIME__, "%d:%d:%d", &curHour, &curMinute, &curSecond) != 3) // Verify compiler was able to acquire computer time
  {
    Serial.println(F("Something went wrong while attempting to collect the time. Verify your Arduino is connected properly, and reupload the sketch."));
    while(true);
  }
  
  // Write time
  writeToAddress(I2C_ADDRESS, SECONDS_REGISTER, getCurVStatus() | (DecToBCD(curSecond) & 0b01111111)); // Writes the current second and the already-existing voltage status. If the voltage status is to be manually set, the seconds will have to be read and re-written
  writeToAddress(I2C_ADDRESS, MINUTES_REGISTER, DecToBCD(curMinute) & 0b01111111); // Writes the current minute
  writeToAddress(I2C_ADDRESS, HOURS_REGISTER, DecToBCD(curHour) & 0b00111111); // Writes the current hour
  
  
}

void autoSyncDay()
{
  byte curDay;
  char curMonth[4];
  word curYear;
  
  if (sscanf(__DATE__, "%s %d %d", curMonth, &curDay, &curYear) != 3) // Verify compiler was able to acquire computer date
  {
    Serial.println(F("Something went wrong while attempting to collect the date. Verify your Arduino is connected properly, and reupload the sketch."));
    while(true);
  }

  // Write date
  writeToAddress(I2C_ADDRESS, DAYS_REGISTER, DecToBCD(curDay) & 0b00111111); // Writes the current day of the month, weekdays will be written in writeManualInfo()
  writeToAddress(I2C_ADDRESS, MONTHS_REGISTER, (curYear < 2100 ? 0b00000000 : 0b10000000) | DecToBCD(monthToValue(curMonth))); // Writes the current century and month
  writeToAddress(I2C_ADDRESS, YEARS_REGISTER, DecToBCD(curYear % 100)); // Writes the current year
}

void manualSync()
{
  byte tempMonth;
  word tempYear;
  
  Serial.println(F("\nWhat is the current second?"));
  while (!Serial.available());
  writeToAddress(I2C_ADDRESS, SECONDS_REGISTER, getCurVStatus() | DecToBCD(Serial.parseInt()));

  Serial.println(F("\nWhat is the current minute?"));
  while (!Serial.available());
  writeToAddress(I2C_ADDRESS, MINUTES_REGISTER, DecToBCD(Serial.parseInt()));

  Serial.println(F("\nWhat is the current hour (in 24 hour format)?"));
  while (!Serial.available());
  writeToAddress(I2C_ADDRESS, HOURS_REGISTER, DecToBCD(Serial.parseInt()));

  Serial.println(F("\nWhat is the current day of the month?"));
  while (!Serial.available());
  writeToAddress(I2C_ADDRESS, DAYS_REGISTER, DecToBCD(Serial.parseInt()));

  Serial.println(F("\nWhat is the current month? Enter one of the following:\n - \"1\"  : January\n - \"2\"  : February\n - \"3\"  : March\n - \"4\"  : April\n - \"5\"  : May\n - \"6\"  : June\n - \"7\"  : July\n - \"8\"  : August\n - \"9\"  : September\n - \"10\" : October\n - \"11\" : November\n - \"12\" : December"));
  while (!Serial.available());
  tempMonth = Serial.parseInt(); // The month is stored along with the century, so we have to wait until we have the year before we can write to the month register

  Serial.println(F("\nWhat is the current year?"));
  while (!Serial.available());
  tempYear = Serial.parseInt(); // We need to process the year with the months, and then we'll be using the year again for the years register. We cant use Serial.parseInt() twice.

  writeToAddress(I2C_ADDRESS, MONTHS_REGISTER, (tempYear < 2100 ? 0b00000000 : 0b10000000) | DecToBCD(tempMonth)); // Writes the month with the current century
  writeToAddress(I2C_ADDRESS, YEARS_REGISTER, DecToBCD(tempYear % 100)); // Writes the last 2 digits of the current year

  Serial.println(F("\nPCF8563 calibrated."));
}

void displayDateTime()
{
  byte * data = readFromAddress(I2C_ADDRESS, SECONDS_REGISTER, 7);
  Serial.print(F("\nCurrently, the PCF8563 at address 0x"));
  Serial.print(I2C_ADDRESS, HEX);
  Serial.println(F(" reads..."));

  // Print date
  Serial.print(F("Date: "));
  Serial.print(weekdayFromValue(BCDToDec(data[4] & 0b00000111)));
  Serial.print(F(" ("));
  Serial.print(BCDToDec(data[4] & 0b00000111));
  Serial.print(F("), "));
  Serial.print(monthFromValue(BCDToDec(data[5] & 0b00011111)));
  Serial.print(F(" ("));
  Serial.print(BCDToDec(data[5] & 0b00011111));
  Serial.print(F(") "));
  Serial.print(BCDToDec(data[3] & 0b00111111));
  Serial.print(F(", "));
  Serial.print((data[5] & 0b10000000) < 0b10000000 ? F("20") : F("21"));
  if (BCDToDec(data[6]) < 10)
    Serial.print('0');
  Serial.println(BCDToDec(data[6]));

  // Print time (24 hour format)
  Serial.print(F("Time: "));
  if (BCDToDec(data[2] & 0b00111111) < 10)
    Serial.print('0');
  Serial.print(BCDToDec(data[2] & 0b00111111));
  Serial.print(':');
  if (BCDToDec(data[1] & 0b01111111) < 10)
    Serial.print('0');
  Serial.print(BCDToDec(data[1] & 0b01111111));
  Serial.print(':');
  if (BCDToDec(data[0] & 0b01111111) < 10)
    Serial.print('0');
  Serial.print(BCDToDec(data[0] & 0b01111111));
  // Print time (12 hour format)
  Serial.print(F(" ("));
  if (BCDToDec(data[2] & 0b00111111) < 10)
    Serial.print('0');
  Serial.print(BCDToDec(data[2] & 0b00111111) % 12);
  Serial.print(':');
  if (BCDToDec(data[1] & 0b01111111) < 10)
    Serial.print('0');
  Serial.print(BCDToDec(data[1] & 0b01111111));
  Serial.print(':');
  if (BCDToDec(data[0] & 0b01111111) < 10)
    Serial.print('0');
  Serial.print(BCDToDec(data[0] & 0b01111111));
  Serial.print(BCDToDec(data[2] & 0b00111111) > 12 ? F("PM") : F("AM"));
  Serial.println(')');

  // Print battery status
  Serial.print(F("Battery: "));
  Serial.println((data[0] & 0b10000000) == 0 ? F("Healthy") : F("Low/Unstable"));
  
  free(data);
}

String weekdayFromValue(byte value)
{
  switch(value)
  {
    case 0:
      return F("Sunday");
    case 1:
      return F("Monday");
    case 2:
      return F("Tuesday");
    case 3:
      return F("Wednesday");
    case 4:
      return F("Thursday");
    case 5:
      return F("Friday");
    case 6:
      return F("Saturday");
    default:
      return F("Error:UnknownWeekday");
  }
}

String monthFromValue(byte value)
{
  switch(value)
  {
    case 1:
      return F("January");
    case 2:
      return F("February");
    case 3:
      return F("March");
    case 4:
      return F("April");
    case 5:
      return F("May");
    case 6:
      return F("June");
    case 7:
      return F("July");
    case 8:
      return F("August");
    case 9:
      return F("September");
    case 10:
      return F("October");
    case 11:
      return F("November");
    case 12:
      return F("December");
    default:
      return F("Error:UnknownMonth");
  }
}
