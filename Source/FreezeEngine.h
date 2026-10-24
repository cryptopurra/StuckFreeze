#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

// Freeze spettrale: blocca lo spettro (magnitudini) e lo risintetizza
// con fasi casuali (tecnica "PaulStretch"), ottenendo un suono tenuto all'infinito.
class FreezeEngine
{
public:
    static constexpr int order = 11;
    static constexpr int N     = 1 << order;   // 2048
    static constexpr int hop   = N / 4;        // overlap 75%
    static constexpr int bins  = N / 2 + 1;

    FreezeEngine() : fft (order)
    {
        window.resize (N);
        for (int i = 0; i < N; ++i)
            window[(size_t) i] = 0.5f * (1.0f - std::cos (6.2831853f * (float) i / (float) N));
        inBuf.assign (N, 0.0f);
        outAccum.assign (N, 0.0f);
        fftData.assign (2 * N, 0.0f);
        frozenMag.assign (bins, 0.0f);
        mag.assign (bins, 0.0f);
        smoothMag.assign (bins, 0.0f);
        synthMag.assign (bins, 0.0f);
        calibrate();
    }

    void prepare (double sr)
    {
        sampleRate = sr;
        std::fill (inBuf.begin(), inBuf.end(), 0.0f);
        std::fill (outAccum.begin(), outAccum.end(), 0.0f);
        std::fill (frozenMag.begin(), frozenMag.end(), 0.0f);
        inPos = outPos = hopCounter = 0;
        gain = 0.0f;
        rng.setSeedRandomly();
    }

    float process (float in, bool freezeOn, float smear, float semitones,
                   float bits, float mix)
    {
        frozen   = freezeOn;
        smearAmt = smear;
        ratio    = std::pow (2.0f, semitones / 12.0f);

        inBuf[(size_t) inPos] = in;
        inPos = (inPos + 1) & (N - 1);
        if (++hopCounter >= hop) { hopCounter = 0; processFrame(); }

        float wet = outAccum[(size_t) outPos];
        outAccum[(size_t) outPos] = 0.0f;
        outPos = (outPos + 1) & (N - 1);

        const float step = (float) (1.0 / (0.02 * sampleRate));
        gain = freezeOn ? std::min (1.0f, gain + step) : std::max (0.0f, gain - step);
        if (gain <= 0.0f)
            return in;

        if (bits < 15.5f)   // Reducer (bit crusher)
        {
            const float lv = std::pow (2.0f, bits - 1.0f);
            wet = std::round (wet * lv) / lv;
        }

        const float g = gain * mix;
        return in * (1.0f - g) + wet * g;
    }

private:
    void processFrame()
    {
        for (int i = 0; i < N; ++i)
            fftData[(size_t) i] = inBuf[(size_t) ((inPos + i) & (N - 1))] * window[(size_t) i];
        std::fill (fftData.begin() + N, fftData.end(), 0.0f);

        fft.performRealOnlyForwardTransform (fftData.data());

        // finché non è congelato, aggiorna continuamente lo spettro "memorizzato"
        if (! frozen)
        {
            for (int k = 0; k < bins; ++k)
                frozenMag[(size_t) k] = std::hypot (fftData[(size_t) (2 * k)], fftData[(size_t) (2 * k + 1)]);
        }

        // Smear: sfoca le magnitudini tra bin vicini
        const int w = 1 + (int) (smearAmt * 12.0f);
        for (int k = 0; k < bins; ++k)
        {
            float sum = 0.0f; int c = 0;
            for (int j = -w; j <= w; ++j)
            {
                const int kk = k + j;
                if (kk >= 0 && kk < bins) { sum += frozenMag[(size_t) kk]; ++c; }
            }
            smoothMag[(size_t) k] = frozenMag[(size_t) k] * (1.0f - smearAmt) + (sum / (float) c) * smearAmt;
        }

        // Pitch: sposta i bin
        for (int k = 0; k < bins; ++k)
        {
            const float src = (float) k / ratio;
            const int i0 = (int) src;
            if (i0 + 1 < bins)
            {
                const float fr = src - (float) i0;
                synthMag[(size_t) k] = smoothMag[(size_t) i0] * (1.0f - fr) + smoothMag[(size_t) (i0 + 1)] * fr;
            }
            else synthMag[(size_t) k] = 0.0f;
        }

        // fasi casuali
        for (int k = 0; k < bins; ++k)
        {
            const float m = synthMag[(size_t) k];
            if (k == 0 || k == bins - 1)
            {
                fftData[(size_t) (2 * k)] = m;
                fftData[(size_t) (2 * k + 1)] = 0.0f;
            }
            else
            {
                const float ph = rng.nextFloat() * 6.2831853f;
                fftData[(size_t) (2 * k)]     = m * std::cos (ph);
                fftData[(size_t) (2 * k + 1)] = m * std::sin (ph);
            }
        }

        fft.performRealOnlyInverseTransform (fftData.data());

        const float scale = norm / 1.5f;   // 1.5 = somma di Hann^2 con overlap 75%
        for (int i = 0; i < N; ++i)
            outAccum[(size_t) ((outPos + i) & (N - 1))] += fftData[(size_t) i] * window[(size_t) i] * scale;
    }

    // misura la normalizzazione reale della FFT (indipendente dalla piattaforma)
    void calibrate()
    {
        std::vector<float> orig ((size_t) N), d ((size_t) (2 * N), 0.0f);
        for (int i = 0; i < N; ++i)
        {
            const float f = (float) i;
            orig[(size_t) i] = std::sin (0.1f * f) + 0.3f * std::cos (0.37f * f) + 0.2f * std::sin (1.3f * f);
            d[(size_t) i] = orig[(size_t) i];
        }
        fft.performRealOnlyForwardTransform (d.data());
        fft.performRealOnlyInverseTransform (d.data());
        double num = 0.0, den = 0.0;
        for (int i = 0; i < N; ++i)
        {
            num += (double) orig[(size_t) i] * orig[(size_t) i];
            den += (double) orig[(size_t) i] * d[(size_t) i];
        }
        norm = std::abs (den) > 1.0e-9 ? (float) (num / den) : 1.0f;
    }

    juce::dsp::FFT fft;
    juce::Random rng;
    std::vector<float> window, inBuf, outAccum, fftData, frozenMag, mag, smoothMag, synthMag;
    double sampleRate = 44100.0;
    int inPos = 0, outPos = 0, hopCounter = 0;
    float gain = 0.0f, smearAmt = 0.0f, ratio = 1.0f, norm = 1.0f;
    bool frozen = false;
};
