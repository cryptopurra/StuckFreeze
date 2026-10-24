#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Effetto "stutter": cattura gli ultimi N campioni e li ripete in loop.
class StutterEngine
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        ring.assign ((size_t) (sr * 10.0), 0.0f);
        loop.assign (ring.size(), 0.0f);
        reset();
    }

    void reset()
    {
        std::fill (ring.begin(), ring.end(), 0.0f);
        std::fill (loop.begin(), loop.end(), 0.0f);
        writePos = 0; loopLen = 1; capturedLen = 0; readPos = 0.0; gain = 0.0f;
        hpX = hpY = 0.0f; wasOn = false;
    }

    float process (float in, bool on, int lenSamples, float smooth,
                   float semitones, bool reverse, float lowCutHz)
    {
        const int N = (int) ring.size();
        ring[(size_t) writePos] = in;
        writePos = (writePos + 1) % N;

        const int len = std::clamp (lenSamples, 64, N - 1);
        auto isStale = [&] { return std::abs (len - capturedLen) > std::max (8, capturedLen / 50); };

        // (Ri)cattura: all'accensione, oppure quando cambia la lunghezza/nota (con gain a zero, senza click)
        if (on && gain < 1.0e-4f && (isStale() || ! wasOn))
        {
            loopLen = len;
            capturedLen = len;
            for (int i = 0; i < loopLen; ++i)
            {
                const int idx = ((writePos - loopLen + i) % N + N) % N;
                loop[(size_t) i] = ring[(size_t) idx];
            }
            readPos = reverse ? (double) (loopLen - 1) : 0.0;
        }
        const bool wantPlay = on && ! isStale();
        wasOn = on;

        const float step = (float) (1.0 / (0.005 * sampleRate));
        gain = wantPlay ? std::min (1.0f, gain + step) : std::max (0.0f, gain - step);
        if (gain <= 0.0f)
            return in;

        // lettura del loop con interpolazione lineare
        const double rate = std::pow (2.0, (double) semitones / 12.0);
        int i0 = std::min ((int) readPos, loopLen - 1);
        const int i1 = (i0 + 1) % loopLen;
        const float frac = (float) (readPos - (double) i0);
        float s = loop[(size_t) i0] * (1.0f - frac) + loop[(size_t) i1] * frac;

        // dissolvenza ai bordi del loop (Smooth)
        const float xf = std::max (16.0f, smooth * 0.5f * (float) loopLen);
        const float pos = (float) readPos;
        float env = std::min ({ 1.0f, pos / xf, ((float) loopLen - pos) / xf });
        s *= std::max (0.0f, env);

        readPos += reverse ? -rate : rate;
        while (readPos >= (double) loopLen) readPos -= (double) loopLen;
        while (readPos < 0.0)               readPos += (double) loopLen;

        // filtro passa-alto (Low Cut)
        if (lowCutHz > 21.0f)
        {
            const float a = std::exp (-6.2831853f * lowCutHz / (float) sampleRate);
            const float y = a * (hpY + s - hpX);
            hpX = s; hpY = y; s = y;
        }
        else { hpX = s; hpY = 0.0f; }

        return in * (1.0f - gain) + s * gain;
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> ring, loop;
    int writePos = 0, loopLen = 1, capturedLen = 0;
    double readPos = 0.0;
    float gain = 0.0f, hpX = 0.0f, hpY = 0.0f;
    bool wasOn = false;
};
