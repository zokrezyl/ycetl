#pragma once

#include <iostream>

template <typename T> struct RecordMeta {};

template <typename T> consteval RecordMeta<T> *record_meta_ptr() {
  return nullptr;
}

using EnumHandler = void (*)();
using RecordHandler = void (*)();
using TypedefHandler = void (*)();

template <typename T> void enum_handler() {
  std::cout << "Handler for type: " << typeid(T).name() << std::endl;
};

template <typename T>
void record_handler() {
  // std::cout << "Handler for type: " << typeid(T).name() << std::endl;
};

template <typename T> void typedef_handler() {
  std::cout << "Handler for type: " << typeid(T).name() << std::endl;
};

template <typename T> consteval const EnumHandler get_enum_handler() {
  return &enum_handler<T>;
}

template <typename T> consteval const RecordHandler get_record_handler() {
  return &record_handler<T>;
}

template <typename T> consteval const TypedefHandler get_typedef_handler() {
  return &typedef_handler<T>;
}
