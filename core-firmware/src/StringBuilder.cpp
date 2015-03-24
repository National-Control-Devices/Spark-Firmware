// Copyright Benoit Blanchon 2014-2015
// MIT License
//
// Arduino JSON library
// https://github.com/bblanchon/ArduinoJson

#include "StringBuilder.hpp"

using namespace ArduinoJson::Internals;

size_t StringBuilder::write(uint8_t c) {
  if (length >= capacity) return 0;

  buffer[length++] = c;
  buffer[length] = '\0';
  return 1;
}