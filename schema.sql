CREATE TABLE sensor_readings (
  id BIGSERIAL PRIMARY KEY,
  created_at TIMESTAMPTZ DEFAULT NOW(),
  temperature FLOAT,
  humidity FLOAT,
  sensor_ok BOOLEAN
);