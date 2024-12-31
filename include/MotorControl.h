#pragma once

#include <KeyValueStore.h>

#include <memory>

void bldc_init(std::shared_ptr<KeyValueStore> kvs);

void bldc_run(void*);

void update_bldc_speed(float duty);
