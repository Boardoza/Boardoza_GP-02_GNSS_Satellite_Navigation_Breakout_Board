/*
GP02++ - a small GPS library for Arduino providing universal NMEA parsing
Based on work by and "distanceBetween" and "courseTo" courtesy of Maarten Lamers.
Suggestion to add satellites, courseTo(), and cardinal() by Matt Monson.
Location precision improvements suggested by Wayne Holder.
Copyright (C) 2008-2024 Mikal Hart
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "GP02.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define _RMCterm "RMC"
#define _GGAterm "GGA"

#if !defined(ARDUINO) && !defined(__AVR__)
// Alternate implementation of millis() that relies on std
unsigned long millis()
{
    static auto start_time = std::chrono::high_resolution_clock::now();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    return static_cast<unsigned long>(duration.count());
}
#endif

GP02::GP02()
  :  parity(0)
  ,  isChecksumTerm(false)
  ,  curSentenceType(GPS_SENTENCE_OTHER)
  ,  curTermNumber(0)
  ,  curTermOffset(0)
  ,  sentenceHasFix(false)
  ,  customElts(0)
  ,  customCandidates(0)
  ,  encodedCharCount(0)
  ,  sentencesWithFixCount(0)
  ,  failedChecksumCount(0)
  ,  passedChecksumCount(0)
{
  term[0] = '\0';
}

//
// public methods
//

/**
 * @brief Encodes a character for processing GPS data.
 * 
 * This function encodes a character for processing GPS data. It updates the internal state
 * of the GP02 object and returns a boolean value indicating if a valid sentence has been processed.
 * 
 * @param c The character to be encoded.
 * @return A boolean value indicating if a valid sentence has been processed.
 */
bool GP02::encode(char c)
{
  ++encodedCharCount;

  switch(c)
  {
  case ',': // term terminators
    parity ^= (uint8_t)c;
  case '\r':
  case '\n':
  case '*':
    {
      bool isValidSentence = false;
      if (curTermOffset < sizeof(term))
      {
        term[curTermOffset] = 0;
        isValidSentence = endOfTermHandler();
      }
      ++curTermNumber;
      curTermOffset = 0;
      isChecksumTerm = c == '*';
      return isValidSentence;
    }
    break;

  case '$': // sentence begin
    curTermNumber = curTermOffset = 0;
    parity = 0;
    curSentenceType = GPS_SENTENCE_OTHER;
    isChecksumTerm = false;
    sentenceHasFix = false;
    return false;

  default: // ordinary characters
    if (curTermOffset < sizeof(term) - 1)
      term[curTermOffset++] = c;
    if (!isChecksumTerm)
      parity ^= c;
    return false;
  }

  return false;
}

//
// internal utilities
//

/**
 * @brief Converts a hexadecimal character to an integer.
 * 
 * This function converts a hexadecimal character to an integer value.
 * If the character is in the range 'A' to 'F' or 'a' to 'f', it returns the corresponding
 * decimal value (10 to 15). Otherwise, it returns the decimal value of the character.
 * 
 * @param a The hexadecimal character to convert.
 * @return The integer value corresponding to the hexadecimal character.
 */
int GP02::fromHex(char a)
{
  if (a >= 'A' && a <= 'F')
    return a - 'A' + 10;
  else if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  else
    return a - '0';
}

// static
// Parse a (potentially negative) number with up to 2 decimal digits -xxxx.yy

/**
 * @brief Parses a (potentially negative) number with up to 2 decimal digits (-xxxx.yy).
 * 
 * This function parses a string representing a decimal number with up to 2 decimal digits.
 * The function converts the string to a 32-bit integer value while considering the sign and decimal point.
 * 
 * @param term The string containing the decimal number to parse.
 * @return The parsed 32-bit integer value.
 */
int32_t GP02::parseDecimal(const char *term)
{
  bool negative = *term == '-';
  if (negative) ++term;
  int32_t ret = 100 * (int32_t)atol(term);
  while (isdigit(*term)) ++term;
  if (*term == '.' && isdigit(term[1]))
  {
    ret += 10 * (term[1] - '0');
    if (isdigit(term[2]))
      ret += term[2] - '0';
  }
  return negative ? -ret : ret;
}

// static
// Parse degrees in that funny NMEA format DDMM.MMMM

/**
 * @brief Parses a string representing degrees and minutes and stores the result in a RawDegrees object.
 * 
 * This function parses a string representing degrees and minutes and stores the parsed values
 * in a RawDegrees object containing the degrees and billionths of a degree.
 * 
 * @param term The string containing the degrees and minutes to parse.
 * @param deg The RawDegrees object to store the parsed values.
 */
void GP02::parseDegrees(const char *term, RawDegrees &deg)
{
  uint32_t leftOfDecimal = (uint32_t)atol(term);
  uint16_t minutes = (uint16_t)(leftOfDecimal % 100);
  uint32_t multiplier = 10000000UL;
  uint32_t tenMillionthsOfMinutes = minutes * multiplier;

  deg.deg = (int16_t)(leftOfDecimal / 100);

  while (isdigit(*term))
    ++term;

  if (*term == '.')
    while (isdigit(*++term))
    {
      multiplier /= 10;
      tenMillionthsOfMinutes += (*term - '0') * multiplier;
    }

  deg.billionths = (5 * tenMillionthsOfMinutes + 1) / 3;
  deg.negative = false;
}

#define COMBINE(sentence_type, term_number) (((unsigned)(sentence_type) << 5) | term_number)

// Processes a just-completed term
// Returns true if new sentence has just passed checksum test and is validated

/**
 * @brief Handles the end of a term in the GPS sentence and performs necessary actions.
 * 
 * This function is responsible for handling the end of a term in the GPS sentence and performing
 * the required actions based on the term received. It checks for checksum, parses the sentence type,
 * and sets various values like time, location, speed, course, date, fix quality, satellites used, etc.
 * 
 * @return Returns true if the checksum is valid and the data is committed; otherwise, returns false.
 */
bool GP02::endOfTermHandler()
{
  // If it's the checksum term, and the checksum checks out, commit
  if (isChecksumTerm)
  {
    byte checksum = 16 * fromHex(term[0]) + fromHex(term[1]);
    if (checksum == parity)
    {
      passedChecksumCount++;
      if (sentenceHasFix)
        ++sentencesWithFixCount;

      switch(curSentenceType)
      {
      case GPS_SENTENCE_RMC:
        date.commit();
        time.commit();
        if (sentenceHasFix)
        {
           location.commit();
           speed.commit();
           course.commit();
        }
        break;
      case GPS_SENTENCE_GGA:
        time.commit();
        if (sentenceHasFix)
        {
          location.commit();
          altitude.commit();
        }
        satellites.commit();
        hdop.commit();
        break;
      }

      // Commit all custom listeners of this sentence type
      for (GP02Custom *p = customCandidates; p != NULL && strcmp(p->sentenceName, customCandidates->sentenceName) == 0; p = p->next)
         p->commit();
      return true;
    }

    else
    {
      ++failedChecksumCount;
    }

    return false;
  }

  // the first term determines the sentence type
  if (curTermNumber == 0)
  {
    if (term[0] == 'G' && strchr("PNABL", term[1]) != NULL && !strcmp(term + 2, _RMCterm))
      curSentenceType = GPS_SENTENCE_RMC;
    else if (term[0] == 'G' && strchr("PNABL", term[1]) != NULL && !strcmp(term + 2, _GGAterm))
      curSentenceType = GPS_SENTENCE_GGA;
    else
      curSentenceType = GPS_SENTENCE_OTHER;

    // Any custom candidates of this sentence type?
    for (customCandidates = customElts; customCandidates != NULL && strcmp(customCandidates->sentenceName, term) < 0; customCandidates = customCandidates->next);
    if (customCandidates != NULL && strcmp(customCandidates->sentenceName, term) > 0)
       customCandidates = NULL;

    return false;
  }

  if (curSentenceType != GPS_SENTENCE_OTHER && term[0])
    switch(COMBINE(curSentenceType, curTermNumber))
  {
    case COMBINE(GPS_SENTENCE_RMC, 1): // Time in both sentences
    case COMBINE(GPS_SENTENCE_GGA, 1):
      time.setTime(term);
      break;
    case COMBINE(GPS_SENTENCE_RMC, 2): // RMC validity
      sentenceHasFix = term[0] == 'A';
      break;
    case COMBINE(GPS_SENTENCE_RMC, 3): // Latitude
    case COMBINE(GPS_SENTENCE_GGA, 2):
      location.setLatitude(term);
      break;
    case COMBINE(GPS_SENTENCE_RMC, 4): // N/S
    case COMBINE(GPS_SENTENCE_GGA, 3):
      location.rawNewLatData.negative = term[0] == 'S';
      break;
    case COMBINE(GPS_SENTENCE_RMC, 5): // Longitude
    case COMBINE(GPS_SENTENCE_GGA, 4):
      location.setLongitude(term);
      break;
    case COMBINE(GPS_SENTENCE_RMC, 6): // E/W
    case COMBINE(GPS_SENTENCE_GGA, 5):
      location.rawNewLngData.negative = term[0] == 'W';
      break;
    case COMBINE(GPS_SENTENCE_RMC, 7): // Speed (RMC)
      speed.set(term);
      break;
    case COMBINE(GPS_SENTENCE_RMC, 8): // Course (RMC)
      course.set(term);
      break;
    case COMBINE(GPS_SENTENCE_RMC, 9): // Date (RMC)
      date.setDate(term);
      break;
    case COMBINE(GPS_SENTENCE_GGA, 6): // Fix data (GGA)
      sentenceHasFix = term[0] > '0';
      location.newFixQuality = (GP02Location::Quality)term[0];
      break;
    case COMBINE(GPS_SENTENCE_GGA, 7): // Satellites used (GGA)
      satellites.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GGA, 8): // HDOP
      hdop.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GGA, 9): // Altitude (GGA)
      altitude.set(term);
      break;
    case COMBINE(GPS_SENTENCE_RMC, 12):
      location.newFixMode = (GP02Location::Mode)term[0];
      break;
  }

  // Set custom values as needed
  for (GP02Custom *p = customCandidates; p != NULL && strcmp(p->sentenceName, customCandidates->sentenceName) == 0 && p->termNumber <= curTermNumber; p = p->next)
    if (p->termNumber == curTermNumber)
         p->set(term);

  return false;
}

/* static */

/**
 * @brief Calculates the distance between two positions on Earth in meters.
 * 
 * This function calculates the distance in meters between two positions specified
 * as signed decimal-degrees latitude and longitude. It uses the great-circle distance
 * computation formula for a hypothetical sphere of radius 6371009 meters. Please note
 * that due to Earth not being a perfect sphere, rounding errors may be up to 0.5%.
 * 
 * @param lat1 Latitude of the first position in decimal-degrees.
 * @param long1 Longitude of the first position in decimal-degrees.
 * @param lat2 Latitude of the second position in decimal-degrees.
 * @param long2 Longitude of the second position in decimal-degrees.
 * @return The distance between the two positions in meters.
 */
double GP02::distanceBetween(double lat1, double long1, double lat2, double long2)
{
  // returns distance in meters between two positions, both specified
  // as signed decimal-degrees latitude and longitude. Uses great-circle
  // distance computation for hypothetical sphere of radius 6371009 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  double delta = radians(long1-long2);
  double sdlong = sin(delta);
  double cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double slat1 = sin(lat1);
  double clat1 = cos(lat1);
  double slat2 = sin(lat2);
  double clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta = sq(delta);
  delta += sq(clat2 * sdlong);
  delta = sqrt(delta);
  double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  return delta * _GPS_EARTH_MEAN_RADIUS;
}

/**
 * @brief Calculates the course from one position to another on Earth in degrees.
 * 
 * This function calculates the course in degrees (North=0, West=270) from the first position
 * to the second position, both specified as signed decimal-degrees latitude and longitude. Please
 * note that due to Earth not being a perfect sphere, the calculated course may be off by a tiny fraction.
 * 
 * @param lat1 Latitude of the first position in decimal-degrees.
 * @param long1 Longitude of the first position in decimal-degrees.
 * @param lat2 Latitude of the second position in decimal-degrees.
 * @param long2 Longitude of the second position in decimal-degrees.
 * @return The course from the first position to the second position in degrees.
 */
double GP02::courseTo(double lat1, double long1, double lat2, double long2)
{
  // returns course in degrees (North=0, West=270) from position 1 to position 2,
  // both specified as signed decimal-degrees latitude and longitude.
  // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
  // Courtesy of Maarten Lamers
  double dlon = radians(long2-long1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double a1 = sin(dlon) * cos(lat2);
  double a2 = sin(lat1) * cos(lat2) * cos(dlon);
  a2 = cos(lat1) * sin(lat2) - a2;
  a2 = atan2(a1, a2);
  if (a2 < 0.0)
  {
    a2 += TWO_PI;
  }
  return degrees(a2);
}

/**
 * @brief Determines the cardinal direction based on the given course in degrees.
 * 
 * This function determines the cardinal direction (e.g., North, South, East, West) based on the
 * given course in degrees. It uses a predefined array of cardinal directions to map the course to
 * the corresponding cardinal direction.
 * 
 * @param course The course in degrees.
 * @return A string representing the cardinal direction (N, NNE, NE, E, etc.).
 */
const char *GP02::cardinal(double course)
{
  static const char* directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  int direction = (int)((course + 11.25f) / 22.5f);
  return directions[direction % 16];
}

/**
 * @brief Commits the new location data to the GP02Location object.
 * 
 * This function updates the GP02Location object with the new latitude, longitude, fix quality,
 * and fix mode data. It also sets the last commit time to the current millis() value and marks
 * the location data as valid and updated.
 */
void GP02Location::commit()
{
   rawLatData = rawNewLatData;
   rawLngData = rawNewLngData;
   fixQuality = newFixQuality;
   fixMode = newFixMode;
   lastCommitTime = millis();
   valid = updated = true;
}

/**
 * @brief Sets the latitude of the GP02Location object using the provided term.
 * 
 * This function sets the latitude of the GP02Location object by parsing the provided term
 * and converting it to the raw latitude data.
 * 
 * @param term The term representing the latitude value to be set.
 */
void GP02Location::setLatitude(const char *term)
{
   GP02::parseDegrees(term, rawNewLatData);
}

/**
 * @brief Sets the longitude of the GP02Location object using the provided term.
 * 
 * This function sets the longitude of the GP02Location object by parsing the provided term
 * and converting it to the raw longitude data.
 * 
 * @param term The term representing the longitude value to be set.
 */
void GP02Location::setLongitude(const char *term)
{
   GP02::parseDegrees(term, rawNewLngData);
}

/**
 * @brief Returns the latitude value of the GP02Location object.
 * 
 * This function returns the latitude value of the GP02Location object. It combines the
 * degree and billionths parts of the latitude data to calculate the actual latitude value.
 * 
 * @return The latitude value in decimal-degrees.
 */
double GP02Location::lat()
{
   updated = false;
   double ret = rawLatData.deg + rawLatData.billionths / 1000000000.0;
   return rawLatData.negative ? -ret : ret;
}

/**
 * @brief Returns the longitude value of the GP02Location object.
 * 
 * This function returns the longitude value of the GP02Location object. It combines the
 * degree and billionths parts of the longitude data to calculate the actual longitude value.
 * 
 * @return The longitude value in decimal-degrees.
 */
double GP02Location::lng()
{
   updated = false;
   double ret = rawLngData.deg + rawLngData.billionths / 1000000000.0;
   return rawLngData.negative ? -ret : ret;
}

/**
 * @brief Commits the new date to the GP02Date object.
 * 
 * This function updates the GP02Date object with the new date value. It also sets the last commit
 * time to the current millis() value and marks the date data as valid and updated.
 */
void GP02Date::commit()
{
   date = newDate;
   lastCommitTime = millis();
   valid = updated = true;
}

/**
 * @brief Commits the new time to the GP02Time object.
 * 
 * This function updates the GP02Time object with the new time value. It also sets the last commit
 * time to the current millis() value and marks the time data as valid and updated.
 */
void GP02Time::commit()
{
   time = newTime;
   lastCommitTime = millis();
   valid = updated = true;
}

/**
 * @brief Sets the time of the GP02Time object using the provided term.
 * 
 * This function sets the time of the GP02Time object by parsing the provided term
 * as a decimal value and converting it to a uint32_t data type.
 * 
 * @param term The term representing the time value to be set.
 */
void GP02Time::setTime(const char *term)
{
   newTime = (uint32_t)GP02::parseDecimal(term);
}

/**
 * @brief Sets the date of the GP02Date object using the provided term.
 * 
 * This function sets the date of the GP02Date object by converting the provided term
 * to a long integer using the atol function.
 * 
 * @param term The term representing the date value to be set.
 */
void GP02Date::setDate(const char *term)
{
   newDate = atol(term);
}

/**
 * @brief Returns the year value of the GP02Date object.
 * 
 * This function calculates and returns the year value of the GP02Date object by extracting
 * the last two digits of the date and adding 2000 to it.
 * 
 * @return The year value.
 */
uint16_t GP02Date::year()
{
   updated = false;
   uint16_t year = date % 100;
   return year + 2000;
}

/**
 * @brief Returns the month value of the GP02Date object.
 * 
 * This function calculates and returns the month value of the GP02Date object by extracting
 * the middle two digits of the date.
 * 
 * @return The month value.
 */
uint8_t GP02Date::month()
{
   updated = false;
   return (date / 100) % 100;
}

/**
 * @brief Returns the day value of the GP02Date object.
 * 
 * This function calculates and returns the day value of the GP02Date object by extracting
 * the first two digits of the date.
 * 
 * @return The day value.
 */
uint8_t GP02Date::day()
{
   updated = false;
   return date / 10000;
}

/**
 * @brief Returns the hour value of the GP02Time object.
 * 
 * This function calculates and returns the hour value of the GP02Time object by extracting
 * the first two digits of the time.
 * 
 * @return The hour value.
 */
uint8_t GP02Time::hour()
{
   updated = false;
   return time / 1000000;
}

/**
 * @brief Returns the minute value of the GP02Time object.
 * 
 * This function calculates and returns the minute value of the GP02Time object by extracting
 * the middle two digits of the time.
 * 
 * @return The minute value.
 */
uint8_t GP02Time::minute()
{
   updated = false;
   return (time / 10000) % 100;
}

/**
 * @brief Returns the second value of the GP02Time object.
 * 
 * This function calculates and returns the second value of the GP02Time object by extracting
 * the last two digits of the time.
 * 
 * @return The second value.
 */
uint8_t GP02Time::second()
{
   updated = false;
   return (time / 100) % 100;
}

/**
 * @brief Returns the centisecond value of the GP02Time object.
 * 
 * This function calculates and returns the centisecond value of the GP02Time object by extracting
 * the last two digits of the time.
 * 
 * @return The centisecond value.
 */
uint8_t GP02Time::centisecond()
{
   updated = false;
   return time % 100;
}

/**
 * @brief Commits the new value to the GP02Decimal object.
 * 
 * This function updates the value of the GP02Decimal object with the new value, sets the last commit time
 * to the current time, and marks the object as valid and updated.
 */
void GP02Decimal::commit()
{
   val = newval;
   lastCommitTime = millis();
   valid = updated = true;
}

/**
 * @brief Sets the new value of the GP02Decimal object using the given character array.
 * 
 * This function parses the character array to a decimal value and sets it as the new value of the GP02Decimal object.
 * 
 * @param term A character array representing the new value.
 */
void GP02Decimal::set(const char *term)
{
   newval = GP02::parseDecimal(term);
}

/**
 * @brief Commits the new value to the GP02Integer object.
 * 
 * This function updates the value of the GP02Integer object with the new value, sets the last commit time
 * to the current time, and marks the object as valid and updated.
 */
void GP02Integer::commit()
{
   val = newval;
   lastCommitTime = millis();
   valid = updated = true;
}

/**
 * @brief Sets the new value of the GP02Integer object using the given character array.
 * 
 * This function converts the character array to a long integer value using the atol function
 * and sets it as the new value of the GP02Integer object.
 * 
 * @param term A character array representing the new value.
 */
void GP02Integer::set(const char *term)
{
   newval = atol(term);
}

/**
 * @brief Constructs a GP02Custom object with the given parameters.
 * 
 * This constructor initializes a GP02Custom object by calling the begin function with the provided GPS object,
 * sentence name, and term number.
 * 
 * @param gps The GPS object to associate with the custom sentence.
 * @param _sentenceName The name of the custom sentence.
 * @param _termNumber The term number of the custom sentence.
 */
GP02Custom::GP02Custom(GP02 &gps, const char *_sentenceName, int _termNumber)
{
   begin(gps, _sentenceName, _termNumber);
}

/**
 * @brief Initializes a GP02Custom object with the given parameters.
 * 
 * This function initializes a GP02Custom object by setting the last commit time to 0,
 * marking the object as not updated and not valid, setting the sentence name and term number,
 * clearing the stagingBuffer and buffer arrays, and inserting the custom item into the GPS tree.
 * 
 * @param gps The GPS object to associate with the custom sentence.
 * @param _sentenceName The name of the custom sentence.
 * @param _termNumber The term number of the custom sentence.
 */
void GP02Custom::begin(GP02 &gps, const char *_sentenceName, int _termNumber)
{
   lastCommitTime = 0;
   updated = valid = false;
   sentenceName = _sentenceName;
   termNumber = _termNumber;
   memset(stagingBuffer, '\0', sizeof(stagingBuffer));
   memset(buffer, '\0', sizeof(buffer));

   // Insert this item into the GPS tree
   gps.insertCustom(this, _sentenceName, _termNumber);
}

/**
 * @brief Commits the staging buffer content to the buffer of the GP02Custom object.
 * 
 * This function copies the content of the staging buffer to the buffer of the GP02Custom object,
 * sets the last commit time to the current time, and marks the object as valid and updated.
 */
void GP02Custom::commit()
{
   strcpy(this->buffer, this->stagingBuffer);
   lastCommitTime = millis();
   valid = updated = true;
}

/**
 * @brief Sets the staging buffer of the GP02Custom object with the given character array.
 * 
 * This function copies the content of the given term to the staging buffer of the GP02Custom object,
 * ensuring that it does not exceed the size of the staging buffer.
 * 
 * @param term A character array representing the content to be set in the staging buffer.
 */
void GP02Custom::set(const char *term)
{
   strncpy(this->stagingBuffer, term, sizeof(this->stagingBuffer) - 1);
}

/**
 * @brief Inserts a custom element into the GP02 object's custom elements list.
 * 
 * This function inserts a custom element into the custom elements list of the GP02 object,
 * based on the provided sentence name and term number order.
 * 
 * @param pElt Pointer to the custom element to be inserted.
 * @param sentenceName Name of the custom sentence.
 * @param termNumber Term number of the custom sentence.
 */
void GP02::insertCustom(GP02Custom *pElt, const char *sentenceName, int termNumber)
{
   GP02Custom **ppelt;

   for (ppelt = &this->customElts; *ppelt != NULL; ppelt = &(*ppelt)->next)
   {
      int cmp = strcmp(sentenceName, (*ppelt)->sentenceName);
      if (cmp < 0 || (cmp == 0 && termNumber < (*ppelt)->termNumber))
         break;
   }

   pElt->next = *ppelt;
   *ppelt = pElt;
}