#ifndef LOWPASS_H
#define LOWPASS_H

class LowPass {
public:
    explicit LowPass(const float alpha) : alpha(alpha), smoothed(0.0f), initialized(false) {}

    float update(const float input) {
        if (!initialized) {
            smoothed = input;
            initialized = true;
        } else {
            smoothed = (1.0f - alpha) * smoothed + alpha * input;
        }
        return smoothed;
    }

    float value() const {
        return smoothed;
    }

    void reset() {
        initialized = false;
        smoothed = 0.0f;
    }

private:
    float alpha;
    float smoothed;
    bool initialized;
};

#endif //LOWPASS_H
