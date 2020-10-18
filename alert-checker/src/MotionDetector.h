#pragma once

#include <Arduino.h>
#include <HCSR04.h>
#include <inttypes.h>
#include <string.h>

// ------------------------------------------------------------------------------------------------
namespace
{
// ------------------------------------------------------------------------------------------------

constexpr uint16_t clog2(uint16_t n) { return ((n < 2) ? 1 : 1 + clog2(n / 2)); }

// ------------------------------------------------------------------------------------------------

constexpr bool cispowerof2(uint16_t n) { return (n > 1) & !(n & (n - 1)); }

} // namespace

// ------------------------------------------------------------------------------------------------

template <uint16_t MAX_SAMPLES = 16> // must be power of 2
struct MeanUint16
{
    static_assert(2 <= MAX_SAMPLES && MAX_SAMPLES <= 256, "Template parameter must be <= 256");
    static_assert(cispowerof2(MAX_SAMPLES), "Template parameter must be a power of two.");

    void addSample(uint16_t sample)
    {
        total = total + sample - samples[current_sample_index];
        samples[current_sample_index] = sample;
        current_sample_index++;

        if(samples_count < MAX_SAMPLES)
            samples_count++;
    }

    uint32_t computeMean() { return total / samples_count; }

    bool isSaturated() { return samples_count >= MAX_SAMPLES; }

    uint8_t getSamplesCount() { return samples_count; }

    void reset()
    {
        samples_count = 0;
        current_sample_index = 0;
        memset(&samples, 0, MAX_SAMPLES * sizeof(samples));
    }

protected:
    uint8_t samples_count;
    uint8_t current_sample_index : clog2(MAX_SAMPLES) - 1;

    uint16_t samples[MAX_SAMPLES]{ 0 };
    uint32_t total{ 0 };

} __attribute__((packed));

// ------------------------------------------------------------------------------------------------

struct MotionDetector
{
    enum class DetectionState : uint8_t {
        MotionDetected,
        MotionEndDetected,
        StateUnchanged,
        Unknown,
    };

    MotionDetector(uint8_t trigger_pin, uint8_t echo_pin, uint8_t temperature=20, bool verbose_logging=false);
    void setup(std::function<void(bool)> callback = nullptr);
    void process();
    void skipSamples(uint8_t n);
    void setTemperatureC(int16_t temp_celsius);

protected:
    const bool verbose;
    MeanUint16<16> distance_samples_mean;
    MeanUint16<8> deviation_samples_mean;

    uint32_t current_distance_mm{0};
    uint32_t average_distance_mm{0};
    uint32_t current_deviation_mm{0};
    uint32_t average_deviation_mm{0};

    uint8_t skip_samples;

    HCSR04 ultrasonic_sensor;
    int16_t temperature;
    std::function<void(bool)> callback;

    const uint8_t detection_threshold_mm {50};
    const uint8_t min_sequential_detections {3};
    const uint16_t sample_scan_separation_ms{100};
    const uint16_t num_samples_at_once{3};

    uint8_t current_sequential_detections {0};
    DetectionState current_detection_state{DetectionState::Unknown};
    DetectionState previous_detection_state{DetectionState::Unknown};

    bool probe();
    DetectionState detect();
};
