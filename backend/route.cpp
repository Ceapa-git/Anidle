#include "route.h"

bool hasMethod(const Route& route, Method method) {
  return route.method & method;
}

