#ifndef GP02_h
#define GP02_h

#include <inttypes.h>
#include "Arduino.h"
#include <limits.h>

#define _GPS_VERSION "1.1.0" // software version of this library
#define _GPS_MPH_PER_KNOT 1.15077945
#define _GPS_MPS_PER_KNOT 0.51444444
#define _GPS_KMPH_PER_KNOT 1.852
#define _GPS_MILES_PER_METER 0.00062137112
#define _GPS_KM_PER_METER 0.001
#define _GPS_FEET_PER_METER 3.2808399
#define _GPS_MAX_FIELD_SIZE 15
#define _GPS_EARTH_MEAN_RADIUS 6371009 // old: 6372795

struct RawDegrees
{
   uint16_t deg;
   uint32_t billionths;
   bool negative;
public:
   RawDegrees() : deg(0), billionths(0), negative(false)
   {}
};

struct GP02Location
{
   friend class GP02;
public:
   enum Quality { Invalid = '0', GPS = '1', DGPS = '2', PPS = '3', RTK = '4', FloatRTK = '5', Estimated = '6', Manual = '7', Simulated = '8' };
   enum Mode { N = 'N', A = 'A', D = 'D', E = 'E'};

   bool isValid() const    { return valid; }
   bool isUpdated() const  { return updated; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
   const RawDegrees &rawLat()     { updated = false; return rawLatData; }
   const RawDegrees &rawLng()     { updated = false; return rawLngData; }
   double lat();
   double lng();
   Quality FixQuality()           { updated = false; return fixQuality; }
   Mode FixMode()                 { updated = false; return fixMode; }

   GP02Location() : valid(false), updated(false), fixQuality(Invalid), fixMode(N)
   {}

private:
   bool valid, updated;
   RawDegrees rawLatData, rawLngData, rawNewLatData, rawNewLngData;
   Quality fixQuality, newFixQuality;
   Mode fixMode, newFixMode;
   uint32_t lastCommitTime;
   void commit();
   void setLatitude(const char *term);
   void setLongitude(const char *term);
};

struct GP02Date
{
   friend class GP02;
public:
   bool isValid() const       { return valid; }
   bool isUpdated() const     { return updated; }
   uint32_t age() const       { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }

   uint32_t value()           { updated = false; return date; }
   uint16_t year();
   uint8_t month();
   uint8_t day();

   GP02Date() : valid(false), updated(false), date(0)
   {}

private:
   bool valid, updated;
   uint32_t date, newDate;
   uint32_t lastCommitTime;
   void commit();
   void setDate(const char *term);
};

struct GP02Time
{
   friend class GP02;
public:
   bool isValid() const       { return valid; }
   bool isUpdated() const     { return updated; }
   uint32_t age() const       { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }

   uint32_t value()           { updated = false; return time; }
   uint8_t hour();
   uint8_t minute();
   uint8_t second();
   uint8_t centisecond();

   GP02Time() : valid(false), updated(false), time(0)
   {}

private:
   bool valid, updated;
   uint32_t time, newTime;
   uint32_t lastCommitTime;
   void commit();
   void setTime(const char *term);
};

struct GP02Decimal
{
   friend class GP02;
public:
   bool isValid() const    { return valid; }
   bool isUpdated() const  { return updated; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
   int32_t value()         { updated = false; return val; }

   GP02Decimal() : valid(false), updated(false), val(0)
   {}

private:
   bool valid, updated;
   uint32_t lastCommitTime;
   int32_t val, newval;
   void commit();
   void set(const char *term);
};

struct GP02Integer
{
   friend class GP02;
public:
   bool isValid() const    { return valid; }
   bool isUpdated() const  { return updated; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
   uint32_t value()        { updated = false; return val; }

   GP02Integer() : valid(false), updated(false), val(0)
   {}

private:
   bool valid, updated;
   uint32_t lastCommitTime;
   uint32_t val, newval;
   void commit();
   void set(const char *term);
};

struct GP02Speed : GP02Decimal
{
   double knots()    { return value() / 100.0; }
   double mph()      { return _GPS_MPH_PER_KNOT * value() / 100.0; }
   double mps()      { return _GPS_MPS_PER_KNOT * value() / 100.0; }
   double kmph()     { return _GPS_KMPH_PER_KNOT * value() / 100.0; }
};

struct GP02Course : public GP02Decimal
{
   double deg()      { return value() / 100.0; }
};

struct GP02Altitude : GP02Decimal
{
   double meters()       { return value() / 100.0; }
   double miles()        { return _GPS_MILES_PER_METER * value() / 100.0; }
   double kilometers()   { return _GPS_KM_PER_METER * value() / 100.0; }
   double feet()         { return _GPS_FEET_PER_METER * value() / 100.0; }
};

struct GP02HDOP : GP02Decimal
{
   double hdop() { return value() / 100.0; }
};

class GP02;
class GP02Custom
{
public:
   GP02Custom() {};
   GP02Custom(GP02 &gps, const char *sentenceName, int termNumber);
   void begin(GP02 &gps, const char *_sentenceName, int _termNumber);

   bool isUpdated() const  { return updated; }
   bool isValid() const    { return valid; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
   const char *value()     { updated = false; return buffer; }

private:
   void commit();
   void set(const char *term);

   char stagingBuffer[_GPS_MAX_FIELD_SIZE + 1];
   char buffer[_GPS_MAX_FIELD_SIZE + 1];
   unsigned long lastCommitTime;
   bool valid, updated;
   const char *sentenceName;
   int termNumber;
   friend class GP02;
   GP02Custom *next;
};

class GP02
{
public:
  GP02();
  bool encode(char c); // process one character received from GPS
  GP02 &operator << (char c) {encode(c); return *this;}

  GP02Location location;
  GP02Date date;
  GP02Time time;
  GP02Speed speed;
  GP02Course course;
  GP02Altitude altitude;
  GP02Integer satellites;
  GP02HDOP hdop;

  static const char *libraryVersion() { return _GPS_VERSION; }

  static double distanceBetween(double lat1, double long1, double lat2, double long2);
  static double courseTo(double lat1, double long1, double lat2, double long2);
  static const char *cardinal(double course);

  static int32_t parseDecimal(const char *term);
  static void parseDegrees(const char *term, RawDegrees &deg);

  uint32_t charsProcessed()   const { return encodedCharCount; }
  uint32_t sentencesWithFix() const { return sentencesWithFixCount; }
  uint32_t failedChecksum()   const { return failedChecksumCount; }
  uint32_t passedChecksum()   const { return passedChecksumCount; }

private:
  enum {GPS_SENTENCE_GGA, GPS_SENTENCE_RMC, GPS_SENTENCE_OTHER};

  // parsing state variables
  uint8_t parity;
  bool isChecksumTerm;
  char term[_GPS_MAX_FIELD_SIZE];
  uint8_t curSentenceType;
  uint8_t curTermNumber;
  uint8_t curTermOffset;
  bool sentenceHasFix;

  // custom element support
  friend class GP02Custom;
  GP02Custom *customElts;
  GP02Custom *customCandidates;
  void insertCustom(GP02Custom *pElt, const char *sentenceName, int index);

  // statistics
  uint32_t encodedCharCount;
  uint32_t sentencesWithFixCount;
  uint32_t failedChecksumCount;
  uint32_t passedChecksumCount;

  // internal utilities
  int fromHex(char a);
  bool endOfTermHandler();
};

#endif // def(__GP02_h)