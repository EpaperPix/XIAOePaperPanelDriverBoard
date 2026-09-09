#pragma once
#include <HTTPClient.h>
// A raw SPI stream requires a known Content-Length; reject chunked/unknown and partial bodies.
// No display refresh takes place until every plane has been read.
template<typename Sender> bool readFramePlane(HTTPClient& request, long remaining, Sender send) {
  auto* stream=request.getStreamPtr();
  uint8_t buffer[128];
  unsigned long started=millis(), lastProgress=started;
  while(remaining>0) {
    if(millis()-lastProgress>=15000UL || millis()-started>=180000UL) return false;
    int available=stream->available();
    if(available>0) {
      size_t amount=min((size_t)available,min(sizeof(buffer),(size_t)remaining));
      int count=stream->readBytes(buffer,amount);
      if(count>0) { send(buffer,count); remaining-=count; lastProgress=millis(); }
    } else if(!request.connected()) return false;
    delay(1);
  }
  return true;
}
