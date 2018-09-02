//part of the code is from https://github.com/maximemoreillon/scale/; credits to the creator.

float get_weight() {
  return scale.get_units();
}

float get_mean_weight() {
  return scale.get_units(10);
}

void add_to_buffer(float in) {
  // Shifting all buffer elements
  for (int i = 1; i < WEIGHT_BUFFER_SIZE; i++) {
    weight_buffer[i - 1] = weight_buffer[i];
  }

  // Adding new element in the buffer
  weight_buffer[WEIGHT_BUFFER_SIZE - 1] = in;
}

float get_range() {
  float buffer_min = 200.00;
  float buffer_max = 0.00;

  for (int i = 0; i < WEIGHT_BUFFER_SIZE; i++) {
    if (weight_buffer[i] > buffer_max) {
      buffer_max = weight_buffer[i];
    }
    if (weight_buffer[i] < buffer_min) {
      buffer_min = weight_buffer[i];
    }
  }
  return  buffer_max - buffer_min;
}

float get_mean() {
  float buffer_sum = 0.00;
  for (int i = 0; i < WEIGHT_BUFFER_SIZE; i++)  buffer_sum += weight_buffer[i];
  return buffer_sum / WEIGHT_BUFFER_SIZE;
}
