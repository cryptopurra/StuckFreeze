#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include <algorithm>

struct FreezeParams
{
    bool  freeze   = false;
    bool  live     = true;      // effetto live quando Freeze e' spento
    float smear    = 0.0f;      // 0..1
    float pitchSt  = 0.0f;      // semitoni
    float bits     = 16.0f;     // Reducer
    float mix      = 1.0f;
    float speed    = 0.0f;      // Freeze: velocita' di scorrimento (neg. = indietro, 0 = fermo)
                                // Live: pitch shift (1.0 = +1 ottava, -1.0 = -1 ottava)
    float grain    = 2048.0f;   // dimensione grano (campioni)
    float engine   = 1.0f;      // 0 = solo Granular, 1 = solo FFT
    int   windowLen = 44100;    // finestra di scorrimento (campioni)
    int   playMode = 1;         // 0 Normal, 1 Loop, 2 Ping-Pong
};

// Freeze stile TimeFreeze: al Freeze cattura gli ultimi 8 secondi e li "suona" con un playhead
// che scorre alla velocita' scelta (anche all'indietro). Due motori: Granular e FFT (spettrale).
class FreezeEngine
{
public:
    static constexpr int order = 11;
    static constexpr int N     = 1 << order;   // 2048
    static constexpr int hop   = N / 4;
    static constexpr int bins  = N / 2 + 1;
    static constexpr int bufSeconds = 8;
    static constexpr int maxGrains  = 8;

    FreezeEngine() : fft (order)
    {
        window.resize (N);
        for (int i = 0; i < N; ++i)
            window[(size_t) i] = 0.5f * (1.0f - std::cos (6.2831853f * (float) i / (float) N));
        fftData.assign (2 * N, 0.0f);
        outAccum.assign (N, 0.0f);
        mag.assign (bins, 0.0f);
        smoothMag.assign (bins, 0.0f);
        synthMag.assign (bins, 0.0f);
        calibrate();
    }

    void prepare (double sr)
    {
        sampleRate = sr;
        cap = (int) (sr * bufSeconds);
        ring.assign ((size_t) cap, 0.0f);
        snap.assign ((size_t) cap, 0.0f);
        std::fill (outAccum.begin(), outAccum.end(), 0.0f);
        wp = 0; outPos = 0; hopCounter = 0; gain = 0.0f;
        wasFrozen = false; wasRunning = false; tAbs = 0; pos = (double) (cap - 1); bounce = 1.0;
        for (auto& g : grains) g.on = false;
        grainTimer = 0;
        rng.setSeedRandomly();
    }

    float process (float in, const FreezeParams& p)
    {
        ring[(size_t) wp] = in;
        wp = (wp + 1) % cap;
        ++tAbs;

        if (p.freeze && ! wasFrozen) engage();
        wasFrozen = p.freeze;
        if (p.freeze) advance (p);

        // Live: se i parametri sono "neutri" il segnale passa pulito (nessun colore, nessun ritardo)
        const bool neutral = std::abs (p.speed) < 1.0e-4f && std::abs (p.pitchSt) < 1.0e-4f
                             && p.smear < 1.0e-4f && p.bits >= 15.5f;
        const bool wetWanted = p.freeze || (p.live && ! neutral);

        const float step = (float) (1.0 / (0.02 * sampleRate));
        gain = wetWanted ? std::min (1.0f, gain + step) : std::max (0.0f, gain - step);

        const bool running = p.freeze || p.live || gain > 0.0f;
        if (! running)
        {
            wasRunning = false;
            return in;
        }
        if (! wasRunning)
        {
            std::fill (outAccum.begin(), outAccum.end(), 0.0f);
            for (auto& g : grains) g.on = false;
            grainTimer = 0;
            hopCounter = hop;
            wasRunning = true;
        }

        const float e = p.engine;
        float g = 0.0f;
        if (e < 0.999f) g = granularSample (p);

        if (e > 0.001f && ++hopCounter >= hop)
        {
            hopCounter = 0;
            fftFrame (p);
        }
        const float f = outAccum[(size_t) outPos];
        outAccum[(size_t) outPos] = 0.0f;
        outPos = (outPos + 1) & (N - 1);

        // compensazione di livello (misurata con rumore): FFT x2, Granular x1.33
        float wet = (1.0f - e) * (g * 1.33f) + e * (f * 2.0f);
        wet = std::tanh (wet);

        if (p.bits < 15.5f)   // Reducer
        {
            const float lv = std::pow (2.0f, p.bits - 1.0f);
            wet = std::round (wet * lv) / lv;
        }

        const float gm = gain * p.mix;
        return in * (1.0f - gm) + wet * gm;
    }

private:
    struct Grain { bool on = false, live = false; int age = 0, len = 0; float rate = 1.0f; double start = 0.0; };

    void engage()
    {
        std::copy (ring.begin() + wp, ring.end(), snap.begin());
        std::copy (ring.begin(), ring.begin() + wp, snap.begin() + (cap - wp));
        pos = (double) (cap - 1);
        bounce = 1.0;
    }

    void advance (const FreezeParams& p)
    {
        const int win = std::clamp (p.windowLen, 4 * N, cap - 1);
        const double lo = (double) (cap - win), hi = (double) (cap - 1);

        pos += (double) p.speed * bounce;

        if (pos > hi)
        {
            if (p.playMode == 0)      pos = hi;
            else if (p.playMode == 1) pos = lo + (pos - hi);
            else                      { pos = hi - (pos - hi); bounce = -bounce; }
        }
        else if (pos < lo)
        {
            if (p.playMode == 0)      pos = lo;
            else if (p.playMode == 1) pos = hi - (lo - pos);
            else                      { pos = lo + (lo - pos); bounce = -bounce; }
        }
        pos = std::clamp (pos, lo, hi);
    }

    float sampleAt (double idx) const
    {
        if (idx <= 0.0) return snap[0];
        if (idx >= (double) (cap - 1)) return snap[(size_t) (cap - 1)];
        const int i0 = (int) idx;
        const float fr = (float) (idx - (double) i0);
        return snap[(size_t) i0] * (1.0f - fr) + snap[(size_t) (i0 + 1)] * fr;
    }

    // legge il flusso live: a = indice assoluto di campione (0 = primo campione mai ricevuto)
    float readLive (double a) const
    {
        const double newest = (double) (tAbs - 1);
        if (a < 0.0) return 0.0f;
        if (a >= newest) return ring[(size_t) ((tAbs - 1) % cap)];
        if (a < newest - (double) (cap - 4)) a = newest - (double) (cap - 4);
        const long long i0 = (long long) a;
        const float fr = (float) (a - (double) i0);
        return ring[(size_t) (i0 % cap)] * (1.0f - fr) + ring[(size_t) ((i0 + 1) % cap)] * fr;
    }

    // ---- motore Granular ----
    float granularSample (const FreezeParams& p)
    {
        const int len = std::clamp ((int) p.grain, 256, 8192);

        if (--grainTimer <= 0)
        {
            for (auto& g : grains)
            {
                if (! g.on)
                {
                    g.on = true; g.age = 0; g.len = len;
                    g.live = ! p.freeze;
                    if (g.live)
                    {
                        // Live: Speed = pitch shift aggiuntivo; il grano parte "indietro" quanto basta per non superare il presente
                        const double semis = (double) p.pitchSt + 12.0 * (double) p.speed;
                        const double rate = std::pow (2.0, semis / 12.0);
                        const double lag = (double) len * std::max (1.0, rate) + 64.0
                                           + (double) rng.nextFloat() * (double) p.smear * (double) len * 2.0;
                        g.rate = (float) rate;
                        g.start = (double) (tAbs - 1) - lag;
                    }
                    else
                    {
                        const float rate = std::pow (2.0f, p.pitchSt / 12.0f);
                        const double jitter = ((double) rng.nextFloat() * 2.0 - 1.0) * (double) p.smear * (double) len * 2.0;
                        g.rate = rate;
                        g.start = pos - 0.5 * (double) len * (double) rate + jitter;
                    }
                    break;
                }
            }
            grainTimer = std::max (1, len / 4);
        }

        float sum = 0.0f;
        for (auto& g : grains)
        {
            if (! g.on) continue;
            const double idx = g.start + (double) g.age * (double) g.rate;
            const float w = window[(size_t) (g.age * N / g.len)];
            sum += (g.live ? readLive (idx) : sampleAt (idx)) * w;
            if (++g.age >= g.len) g.on = false;
        }
        return sum * 0.5f;
    }

    // ---- motore FFT (magnitudini + fasi casuali) ----
    void fftFrame (const FreezeParams& p)
    {
        if (p.freeze)
        {
            const int startIdx = (int) pos - N;
            for (int i = 0; i < N; ++i)
            {
                const int idx = std::clamp (startIdx + i, 0, cap - 1);
                fftData[(size_t) i] = snap[(size_t) idx] * window[(size_t) i];
            }
        }
        else
        {
            for (int i = 0; i < N; ++i)
            {
                const long long a = tAbs - N + i;
                const float v = a >= 0 ? ring[(size_t) (a % cap)] : 0.0f;
                fftData[(size_t) i] = v * window[(size_t) i];
            }
        }
        std::fill (fftData.begin() + N, fftData.end(), 0.0f);

        fft.performRealOnlyForwardTransform (fftData.data());

        for (int k = 0; k < bins; ++k)
            mag[(size_t) k] = std::hypot (fftData[(size_t) (2 * k)], fftData[(size_t) (2 * k + 1)]);

        // Smear
        const float sm = p.smear;
        const int w = 1 + (int) (sm * 12.0f);
        for (int k = 0; k < bins; ++k)
        {
            float sum = 0.0f; int c = 0;
            for (int j = -w; j <= w; ++j)
            {
                const int kk = k + j;
                if (kk >= 0 && kk < bins) { sum += mag[(size_t) kk]; ++c; }
            }
            smoothMag[(size_t) k] = mag[(size_t) k] * (1.0f - sm) + (sum / (float) c) * sm;
        }

        // Pitch
        const float semis = p.freeze ? p.pitchSt : p.pitchSt + 12.0f * p.speed;
        const float ratio = std::pow (2.0f, semis / 12.0f);
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

        const float scale = norm / 1.5f;
        for (int i = 0; i < N; ++i)
            outAccum[(size_t) ((outPos + i) & (N - 1))] += fftData[(size_t) i] * window[(size_t) i] * scale;
    }

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
    std::vector<float> window, fftData, outAccum, mag, smoothMag, synthMag, ring, snap;
    Grain grains[maxGrains];
    double sampleRate = 44100.0, pos = 0.0, bounce = 1.0;
    long long tAbs = 0;
    int cap = 1, wp = 0, outPos = 0, hopCounter = 0, grainTimer = 0;
    float gain = 0.0f, norm = 1.0f;
    bool wasFrozen = false, wasRunning = false;
};
