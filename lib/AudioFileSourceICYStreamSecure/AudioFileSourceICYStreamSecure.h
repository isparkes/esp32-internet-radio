/*
  AudioFileSourceICYStreamSecure
  Drop-in replacement for AudioFileSourceICYStream with https:// support.
  Uses WiFiClientSecure with setInsecure() — no CA certificate required.
  Works identically for http:// streams.
*/

#pragma once

#if defined(ESP32)

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <AudioFileSource.h>

class AudioFileSourceHTTPStreamSecure : public AudioFileSource {
  friend class AudioFileSourceICYStreamSecure;
public:
  AudioFileSourceHTTPStreamSecure();
  AudioFileSourceHTTPStreamSecure(const char *url);
  virtual ~AudioFileSourceHTTPStreamSecure() override;

  virtual bool open(const char *url) override;
  virtual uint32_t read(void *data, uint32_t len) override;
  virtual uint32_t readNonBlock(void *data, uint32_t len) override;
  virtual bool seek(int32_t pos, int dir) override;
  virtual bool close() override;
  virtual bool isOpen() override;
  virtual uint32_t getSize() override;
  virtual uint32_t getPos() override;

  bool SetReconnect(int tries, int delayms) { reconnectTries = tries; reconnectDelayMs = delayms; return true; }
  void useHTTP10() { http.useHTTP10(true); }

  enum { STATUS_HTTPFAIL=2, STATUS_DISCONNECTED, STATUS_RECONNECTING, STATUS_RECONNECTED, STATUS_NODATA };

private:
  virtual uint32_t readInternal(void *data, uint32_t len, bool nonBlock);
  WiFiClientSecure client;  // handles both http and https; setInsecure() called in open()
  HTTPClient http;
  int pos;
  int size;
  int reconnectTries;
  int reconnectDelayMs;
  char saveURL[128];
};

class AudioFileSourceICYStreamSecure : public AudioFileSourceHTTPStreamSecure {
public:
  AudioFileSourceICYStreamSecure();
  AudioFileSourceICYStreamSecure(const char *url);
  virtual ~AudioFileSourceICYStreamSecure() override;

  virtual bool open(const char *url) override;

private:
  virtual uint32_t readInternal(void *data, uint32_t len, bool nonBlock) override;
  int icyMetaInt;
  int icyByteCount;
};

#endif // ESP32
