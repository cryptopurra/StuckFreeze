#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Stutter "roll-up" stile Stucker:
//  - all'attivazione cattura l'ultima fetta di audio (la griglia)
//  - la ripete accorciandola in continuo (sliceTarget), fino a diventare un suono intonato
//  - al rilascio: Cut (stop secco), Tail (coda che sfuma), Reverse (coda al contrario)
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
        writePos = 0; loopLen = 64; readPos = 0.0;
        sliceCur = 64.0f; lastTarget = 64.0f;
        gain = 0.0f; hpX = hpY = 0.0f; wasActive = false;
    }

    // dropMode: 0 = Cut, 1 = Tail, 2 = Reverse
    float process (float in, bool active, int gridLen, float sliceTarget,
                   float smooth, float lowCutHz, int dropMode)
    {
        const int N = (int) ring.size();
        ring[(size_t) writePos] = in;
        writePos = (writePos + 1) % N;

        // cattura la fetta quando la manopola si stacca da OFF
        if (active && ! wasActive && gain < 1.0e-4f)
        {
            loopLen = std::clamp (gridLen, 64, N - 1);
            for (int i = 0; i < loopLen; ++i)
            {
                const int idx = ((writePos - loopLen + i) % N + N) % N;
                loop[(size_t) i] = ring[(size_t) idx];
            }
            readPos = 0.0;
            sliceCur = std::clamp (sliceTarget, 32.0f, (float) loopLen);
            lastTarget = sliceCur;
        }
        wasActive = active;

        if (active)
            lastTarget = std::clamp (sliceTarget, 32.0f, (float) loopLen);

        // la lunghezza della fetta scivola verso il valore richiesto (niente scatti)
        const float slew = 1.0f - std::exp (-1.0f / (0.003f * (float) sampleRate));
        sliceCur += (lastTarget - sliceCur) * slew;

        // dissolvenza in entrata / uscita
        if (active)
            gain = std::min (1.0f, gain + (float) (1.0 / (0.005 * sampleRate)));
        else
        {
            const double rel = (dropMode == 0) ? 0.005 : 0.35;
            gain = std::max (0.0f, gain - (float) (1.0 / (rel * sampleRate)));
        }
        if (gain <= 0.0f)
            return in;

        const float L = std::clamp (sliceCur, 32.0f, (float) loopLen);
        const double dir = (! active && dropMode == 2) ? -1.0 : 1.0;

        while (readPos >= (double) L) readPos -= (double) L;
        while (readPos < 0.0)         readPos += (double) L;

        const int i0 = std::min ((int) readPos, loopLen - 1);
        const int i1 = (i0 + 1 < loopLen) ? i0 + 1 : 0;
        const float frac = (float) (readPos - (double) i0);
        float s = loop[(size_t) i0] * (1.0f - frac) + loop[(size_t) i1] * frac;

        // Smooth: dissolvenza ai bordi della fetta
        const float xf = std::min (std::max (16.0f, smooth * 0.5f * L), L * 0.5f);
        const float pos = (float) readPos;
        const float env = std::max (0.0f, std::min ({ 1.0f, pos / xf, (L - pos) / xf }));
        s *= env;

        readPos += dir;

        // Low cut (passa-alto a un polo)
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
    int writePos = 0, loopLen = 64;
    double readPos = 0.0;
    float sliceCur = 64.0f, lastTarget = 64.0f;
    float gain = 0.0f, hpX = 0.0f, hpY = 0.0f;
    bool wasActive = false;
};
