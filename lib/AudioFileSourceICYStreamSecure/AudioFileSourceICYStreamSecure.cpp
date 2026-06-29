/*
  AudioFileSourceICYStreamSecure
  Drop-in replacement for AudioFileSourceICYStream with https:// support.
*/

#if defined(ESP32)

#ifdef _GNU_SOURCE
#undef _GNU_SOURCE
#endif
#define _GNU_SOURCE

#include "AudioFileSourceICYStreamSecure.h"
#include <string.h>

// ============================================================
// AudioFileSourceHTTPStreamSecure
// ============================================================

AudioFileSourceHTTPStreamSecure::AudioFileSourceHTTPStreamSecure()
{
  pos = 0;
  reconnectTries = 0;
  reconnectDelayMs = 0;
  saveURL[0] = 0;
}

AudioFileSourceHTTPStreamSecure::AudioFileSourceHTTPStreamSecure(const char *url)
{
  saveURL[0] = 0;
  reconnectTries = 0;
  reconnectDelayMs = 0;
  open(url);
}

AudioFileSourceHTTPStreamSecure::~AudioFileSourceHTTPStreamSecure()
{
  http.end();
}

bool AudioFileSourceHTTPStreamSecure::open(const char *url)
{
  pos = 0;
  client.setInsecure();  // skip certificate verification — fine for radio streams
  http.begin(client, url);
  http.setReuse(true);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    cb.st(STATUS_HTTPFAIL, PSTR("Can't open HTTP request"));
    return false;
  }
  size = http.getSize();
  strncpy(saveURL, url, sizeof(saveURL));
  saveURL[sizeof(saveURL) - 1] = 0;
  return true;
}

uint32_t AudioFileSourceHTTPStreamSecure::read(void *data, uint32_t len)
{
  if (data == NULL) {
    audioLogger->printf_P(PSTR("ERROR! AudioFileSourceHTTPStreamSecure::read passed NULL data\n"));
    return 0;
  }
  return readInternal(data, len, false);
}

uint32_t AudioFileSourceHTTPStreamSecure::readNonBlock(void *data, uint32_t len)
{
  if (data == NULL) {
    audioLogger->printf_P(PSTR("ERROR! AudioFileSourceHTTPStreamSecure::readNonBlock passed NULL data\n"));
    return 0;
  }
  return readInternal(data, len, true);
}

uint32_t AudioFileSourceHTTPStreamSecure::readInternal(void *data, uint32_t len, bool nonBlock)
{
retry:
  if (!http.connected()) {
    cb.st(STATUS_DISCONNECTED, PSTR("Stream disconnected"));
    http.end();
    for (int i = 0; i < reconnectTries; i++) {
      char buff[64];
      sprintf_P(buff, PSTR("Attempting to reconnect, try %d"), i);
      cb.st(STATUS_RECONNECTING, buff);
      delay(reconnectDelayMs);
      if (open(saveURL)) {
        cb.st(STATUS_RECONNECTED, PSTR("Stream reconnected"));
        break;
      }
    }
    if (!http.connected()) {
      cb.st(STATUS_DISCONNECTED, PSTR("Unable to reconnect"));
      return 0;
    }
  }
  if ((size > 0) && (pos >= size)) return 0;

  WiFiClient *stream = http.getStreamPtr();

  if ((size > 0) && (len > (uint32_t)(pos - size))) len = pos - size;

  if (!nonBlock) {
    int start = millis();
    while ((stream->available() < (int)len) && (millis() - start < 500)) yield();
  }

  size_t avail = stream->available();
  if (!nonBlock && !avail) {
    cb.st(STATUS_NODATA, PSTR("No stream data available"));
    http.end();
    goto retry;
  }
  if (avail == 0) return 0;
  if (avail < len) len = avail;

  int read = stream->read(reinterpret_cast<uint8_t *>(data), len);
  pos += read;
  return read;
}

bool AudioFileSourceHTTPStreamSecure::seek(int32_t pos, int dir)
{
  audioLogger->printf_P(PSTR("ERROR! AudioFileSourceHTTPStreamSecure::seek not implemented!"));
  (void)pos; (void)dir;
  return false;
}

bool AudioFileSourceHTTPStreamSecure::close()
{
  http.end();
  return true;
}

bool AudioFileSourceHTTPStreamSecure::isOpen()
{
  return http.connected();
}

uint32_t AudioFileSourceHTTPStreamSecure::getSize()  { return size; }
uint32_t AudioFileSourceHTTPStreamSecure::getPos()   { return pos; }

// ============================================================
// AudioFileSourceICYStreamSecure
// ============================================================

AudioFileSourceICYStreamSecure::AudioFileSourceICYStreamSecure()
{
  icyMetaInt = 0;
  icyByteCount = 0;
}

AudioFileSourceICYStreamSecure::AudioFileSourceICYStreamSecure(const char *url)
{
  icyMetaInt = 0;
  icyByteCount = 0;
  open(url);
}

AudioFileSourceICYStreamSecure::~AudioFileSourceICYStreamSecure()
{
  http.end();
}

bool AudioFileSourceICYStreamSecure::open(const char *url)
{
  static const char *hdr[] = { "icy-metaint", "icy-name", "icy-genre", "icy-br" };
  pos = 0;
  client.setInsecure();
  http.begin(client, url);
  http.addHeader("Icy-MetaData", "1");
  http.collectHeaders(hdr, 4);
  http.setReuse(true);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    cb.st(STATUS_HTTPFAIL, PSTR("Can't open HTTP request"));
    return false;
  }
  if (http.hasHeader(hdr[0])) {
    icyMetaInt = http.header(hdr[0]).toInt();
  } else {
    icyMetaInt = 0;
  }
  icyByteCount = 0;
  size = http.getSize();
  strncpy(saveURL, url, sizeof(saveURL));
  saveURL[sizeof(saveURL) - 1] = 0;
  return true;
}

uint32_t AudioFileSourceICYStreamSecure::readInternal(void *data, uint32_t len, bool nonBlock)
{
  if (icyMetaInt > 1) {
    len = std::min((int)(icyMetaInt >> 1), (int)len);
  }
retry:
  if (!http.connected()) {
    cb.st(STATUS_DISCONNECTED, PSTR("Stream disconnected"));
    http.end();
    for (int i = 0; i < reconnectTries; i++) {
      char buff[64];
      sprintf_P(buff, PSTR("Attempting to reconnect, try %d"), i);
      cb.st(STATUS_RECONNECTING, buff);
      delay(reconnectDelayMs);
      if (open(saveURL)) {
        cb.st(STATUS_RECONNECTED, PSTR("Stream reconnected"));
        break;
      }
    }
    if (!http.connected()) {
      cb.st(STATUS_DISCONNECTED, PSTR("Unable to reconnect"));
      return 0;
    }
  }
  if ((size > 0) && (pos >= size)) return 0;

  WiFiClient *stream = http.getStreamPtr();

  if ((size > 0) && (len > (uint32_t)(pos - size))) len = pos - size;

  if (!nonBlock) {
    int start = millis();
    while ((stream->available() < (int)len) && (millis() - start < 500)) yield();
  }

  size_t avail = stream->available();
  if (!nonBlock && !avail) {
    cb.st(STATUS_NODATA, PSTR("No stream data available"));
    http.end();
    goto retry;
  }
  if (avail == 0) return 0;
  if (avail < len) len = avail;

  int read = 0;
  int ret  = 0;

  if (((int)(icyByteCount + len) > (int)icyMetaInt) && (icyMetaInt > 0)) {
    int beforeIcy = icyMetaInt - icyByteCount;
    if (beforeIcy > 0) {
      ret = stream->read(reinterpret_cast<uint8_t *>(data), beforeIcy);
      if (ret < 0) ret = 0;
      read += ret;
      pos  += ret;
      len  -= ret;
      data  = (void *)(reinterpret_cast<char *>(data) + ret);
      icyByteCount += ret;
      if (ret != beforeIcy) return read;
    }

    int mdSize;
    uint8_t c;
    int mdret = stream->read(&c, 1);
    if (mdret == 0) return read;
    mdSize = c * 16;
    if ((mdret == 1) && (mdSize > 0)) {
      char icyBuff[256 + 16 + 1];
      char *readInto = icyBuff + 16;
      memset(icyBuff, 0, 16);
      while (mdSize) {
        int toRead = mdSize > 256 ? 256 : mdSize;
        int ret = stream->read((uint8_t *)readInto, toRead);
        if (ret < 0) return read;
        if (ret == 0) { delay(1); continue; }
        mdSize -= ret;
        int end = 16 + ret;
        char *header = (char *)memmem((void *)icyBuff, end, (void *)"StreamTitle=", 12);
        if (!header) {
          memmove(icyBuff, icyBuff + end - 16, 16);
          delay(1);
          continue;
        }
        int lastValidByte = end - (header - icyBuff) + 1;
        memmove(icyBuff, header, lastValidByte);
        while (mdSize && lastValidByte < 255) {
          int toRead = mdSize > (256 - lastValidByte) ? (256 - lastValidByte) : mdSize;
          ret = stream->read((uint8_t *)icyBuff + lastValidByte, toRead);
          if (ret == -1) return read;
          if (ret == 0) { delay(1); continue; }
          mdSize -= ret;
          lastValidByte += ret;
        }
        char *p = icyBuff + 12;
        if (*p == '\'' || *p == '"') {
          char closing[] = { *p, ';', '\0' };
          char *psz = strstr(p + 1, closing);
          if (!psz) psz = strchr(&icyBuff[13], ';');
          if (psz) *psz = '\0';
          p++;
        } else {
          char *psz = strchr(p, ';');
          if (psz) *psz = '\0';
        }
        cb.md("StreamTitle", false, p);
        while (mdSize) {
          int toRead = mdSize > 256 ? 256 : mdSize;
          ret = stream->read((uint8_t *)icyBuff, toRead);
          if (ret < 0) return read;
          if (ret == 0) { delay(1); continue; }
          mdSize -= ret;
        }
      }
    }
    icyByteCount = 0;
  }

  ret = stream->read(reinterpret_cast<uint8_t *>(data), len);
  if (ret < 0) ret = 0;
  read += ret;
  pos  += ret;
  icyByteCount += ret;
  return read;
}

#endif // ESP32
