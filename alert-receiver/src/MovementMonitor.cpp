#include "MovementMonitor.h"

// ------------------------------------------------------------------------------------------------

MovementMonitor::MovementMonitor(uint8_t trigger_pin, uint8_t echo_pin, uint8_t temperature, bool verbose_logging)
: verbose{ verbose_logging }, ultrasonic_sensor{ trigger_pin, echo_pin, temperature, 255 }, temperature{ temperature }
{
}

// ------------------------------------------------------------------------------------------------

void MovementMonitor::setup(std::function<void(bool)> callback)
{
    Serial.println(F("initializing movement monitor:"));
    Serial.printf("threshold:                     %d [cm]\n", detection_threshold_mm);
    Serial.printf("minimum sequential detections: %d times\n", min_sequential_detections);
    Serial.printf("temperature:                   %d [°C]\n", temperature);
    Serial.printf("sample scan separation:        %d [ms]\n", sample_scan_separation_ms);
    Serial.printf("samples at once:               %d times\n", num_samples_at_once);

    this->callback = callback;
    ultrasonic_sensor.begin();
}

// ------------------------------------------------------------------------------------------------

void MovementMonitor::process()
{
    auto detected_state = detect();

    if(callback != nullptr && detected_state != DetectionState::Unknown)
    {
        callback(current_detection_state == DetectionState::Movement);
    }
}

// ------------------------------------------------------------------------------------------------

void MovementMonitor::skipSamples(uint8_t n) { skip_samples = n; }

// ------------------------------------------------------------------------------------------------

bool MovementMonitor::probe()
{
    const float distance = ultrasonic_sensor.getDistance();

    // --- skip requested N samples but measure to sample out VCC
    skip_samples = (skip_samples > 0) ? skip_samples - 1 : skip_samples;
    if(skip_samples > 0)
    {
        Serial.print('.');
        return false;
    }

    current_distance_mm = 10u * static_cast<uint16_t>(std::roundf(distance));

    if(HCSR04_OUT_OF_RANGE == distance)
    {
        if(verbose)
            Serial.println(F("out of range"));
        return false;
    }

    // ----- track running mean of samples
    distance_samples_mean.addSample(current_distance_mm);
    if(distance_samples_mean.isSaturated())
    {
        average_distance_mm = distance_samples_mean.computeMean();

        // ----- track running mean of deviation
        uint32_t min{ current_distance_mm }, max{ average_distance_mm };
        if(min > max)
        {
            min = max;
            max = current_distance_mm;
        }
        current_deviation_mm = max - min;
        deviation_samples_mean.addSample(current_deviation_mm);

        if(deviation_samples_mean.isSaturated())
        {
            average_deviation_mm = deviation_samples_mean.computeMean();
            return true;
        }
    }

    return false;
}

// ------------------------------------------------------------------------------------------------

MovementMonitor::DetectionState MovementMonitor::detect()
{
    for(int i{ num_samples_at_once }; i > 0; i--)
    {
        if(probe())
        {
            if(verbose)
            {
                Serial.printf("dst(%4u,~%4u)", current_distance_mm, average_distance_mm);
                Serial.printf(" dev(%4u,~%4u) thr(%4u)", current_deviation_mm, average_deviation_mm,
                              detection_threshold_mm);
            }

            // ----- accumulate hits above/below threshold
            if(average_deviation_mm >= detection_threshold_mm && current_sequential_detections < min_sequential_detections)
            {
                current_sequential_detections++;
                if(verbose)
                    Serial.printf(" ++");
            }
            else if(average_deviation_mm <= detection_threshold_mm && current_sequential_detections > 0)
            {
                current_sequential_detections--;
                if(verbose)
                    Serial.printf(" --");
            }
            else
            {
                if(verbose)
                    Serial.printf("   ");
            }

            if(verbose)
                Serial.printf("seq(%2d)", current_sequential_detections);

            // ----- evaluate limits
            if(current_sequential_detections >= min_sequential_detections)
            {
                previous_detection_state = current_detection_state;
                current_detection_state = DetectionState::Movement;
                if(verbose)
                    Serial.printf(" hit   ");
            }
            else if(current_sequential_detections <= 0)
            {
                previous_detection_state = current_detection_state;
                current_detection_state = DetectionState::NoDetection;
                if(verbose)
                    Serial.printf(" noise ");
            }
            else if(verbose)
            {
                Serial.printf(" update ");
            }

            if(current_detection_state != previous_detection_state)
            {
                if(current_detection_state == DetectionState::Movement)
                {
                    if(verbose)
                        Serial.printf(" -> movement\n");
                    return MovementMonitor::DetectionState::Movement;
                }
                else
                {
                    if(verbose)
                        Serial.printf(" -> stopped\n");
                    return MovementMonitor::DetectionState::NoDetection;
                }
            }

            if(verbose)
                if(i > 1)
                    Serial.println();
        }
        else // ouf of range probes do not constitute a sample
            i++;

        delay(sample_scan_separation_ms);
    }

    if(verbose)
        Serial.printf(" -> unchanged\n");
    return MovementMonitor::DetectionState::NoDetection;
}