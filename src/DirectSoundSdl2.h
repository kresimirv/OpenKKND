#pragma once

#include <SDL2/SDL.h>
#include <cstring>
#include <cstdlib>
#include <cmath>

struct WAVEFORMATEX
{
    unsigned short wFormatTag;
    unsigned short nChannels;
    unsigned int   nSamplesPerSec;
    unsigned int   nAvgBytesPerSec;
    unsigned short nBlockAlign;
    unsigned short wBitsPerSample;
    unsigned short cbSize;
};

struct WAVEFORMATEX;
struct DSBUFFERDESC
{
    unsigned int dwSize;
    unsigned int dwFlags;
    unsigned int dwBufferBytes;
    unsigned int dwReserved;
    WAVEFORMATEX *lpwfxFormat;
};

struct DSBCAPS
{
    unsigned int dwSize;
    unsigned int dwFlags;
    unsigned int dwBufferBytes;
    unsigned int dwUnlockTransferRate;
    unsigned int dwPlayCpuOverhead;
};

struct IDirectSound; // forward decl

struct IDirectSoundBuffer
{
    SDL_AudioDeviceID device_id;
    unsigned char *buffer;
    unsigned int buffer_size;
    unsigned int write_cursor;
    WAVEFORMATEX wfx;
    bool playing;
    bool looping;
    bool restored;
    int volume_dsb; // DirectSound hundredths-of-dB, 0 = max

    unsigned int play_start_time;
    unsigned int duration_ms;

    // temp buffer for format conversion
    unsigned char *convert_buf;
    unsigned int convert_buf_size;

    IDirectSoundBuffer() : device_id(0), buffer(nullptr), buffer_size(0),
        write_cursor(0), playing(false), looping(false), restored(false),
        volume_dsb(-2000), play_start_time(0), duration_ms(0),
        convert_buf(nullptr), convert_buf_size(0)
    {
        memset(&wfx, 0, sizeof(wfx));
    }

    ~IDirectSoundBuffer()
    {
        if (convert_buf)
            free(convert_buf);
    }

    static void scale_audio(unsigned char *data, unsigned int bytes,
                            int volume_dsb, int wBitsPerSample)
    {
        if (volume_dsb >= 0) return;
        unsigned short cw, safe_cw;
        __asm__ __volatile__ ("fnstcw %0" : "=m" (cw) : : "memory");
        safe_cw = cw | 0x3F; // mask all x87 exceptions
        __asm__ __volatile__ ("fnclex" : : : "memory");
        __asm__ __volatile__ ("fldcw %0" : : "m" (safe_cw) : "memory");

        double factor = pow(10.0, volume_dsb / 2000.0);
        if (wBitsPerSample == 16)
        {
            short *samples = (short *)data;
            int count = bytes / 2;
            for (int i = 0; i < count; i++)
            {
                double v = samples[i] * factor;
                if (v > 32767.0) v = 32767.0;
                else if (v < -32768.0) v = -32768.0;
                samples[i] = (short)v;
            }
        }
        else
        {
            for (unsigned int i = 0; i < bytes; i++)
            {
                double v = (data[i] - 128) * factor + 128;
                if (v > 255.0) v = 255.0;
                else if (v < 0.0) v = 0.0;
                data[i] = (unsigned char)v;
            }
        }

        __asm__ __volatile__ ("fnclex" : : : "memory");
        __asm__ __volatile__ ("fldcw %0" : : "m" (cw) : "memory");
    }

    int Lock(unsigned int dwOffset, unsigned int dwBytes,
             void **ppvAudioPtr1, unsigned int *pdwAudioBytes1,
             void **ppvAudioPtr2, unsigned int *pdwAudioBytes2,
             unsigned int dwFlags)
    {
        if (buffer == nullptr) return -1;
        unsigned int offset = dwOffset % buffer_size;
        unsigned int available = buffer_size - offset;
        if (available >= dwBytes)
        {
            *ppvAudioPtr1 = buffer + offset;
            *pdwAudioBytes1 = dwBytes;
            *ppvAudioPtr2 = nullptr;
            *pdwAudioBytes2 = 0;
        }
        else
        {
            *ppvAudioPtr1 = buffer + offset;
            *pdwAudioBytes1 = available;
            *ppvAudioPtr2 = buffer;
            *pdwAudioBytes2 = dwBytes - available;
        }
        return 0;
    }

    int Unlock(void *pvAudioPtr1, unsigned int dwAudioBytes1,
               void *pvAudioPtr2, unsigned int dwAudioBytes2)
    {
        if (!device_id)
            return 0;

        for (int pass = 0; pass < 2; pass++)
        {
            void *ptr = (pass == 0) ? pvAudioPtr1 : pvAudioPtr2;
            unsigned int bytes = (pass == 0) ? dwAudioBytes1 : dwAudioBytes2;
            if (!ptr || bytes == 0)
                continue;

            scale_audio((unsigned char *)ptr, bytes, volume_dsb, wfx.wBitsPerSample);

            // Convert to device format (22050/16/stereo) if needed
            unsigned int out_bytes, out_rate;
            unsigned char *out_data = convert_to_device(
                (unsigned char *)ptr, bytes, &out_bytes, &out_rate);

            SDL_QueueAudio(device_id, out_data, out_bytes);
        }
        return 0;
    }

    // Convert audio from buffer's native format to device format (22050 Hz, 16-bit, stereo)
    // Handles 8-bit->16-bit, sample rate conversion, mono->stereo
    unsigned char *convert_to_device(unsigned char *data, unsigned int bytes,
                                     unsigned int *out_bytes, unsigned int *out_rate)
    {
        int src_bps = wfx.wBitsPerSample;
        int src_ch = wfx.nChannels;
        int src_rate = wfx.nSamplesPerSec;
        int dst_bps = 16;
        int dst_ch = 2;
        int dst_rate = 22050;

        // Safety: invalid format, passthrough
        if (src_bps == 0 || src_ch == 0)
        {
            *out_bytes = bytes;
            *out_rate = dst_rate;
            return data;
        }

        // Fast path: already matches device format
        if (src_bps == dst_bps && src_ch == dst_ch && src_rate == dst_rate)
        {
            *out_bytes = bytes;
            *out_rate = dst_rate;
            return data;
        }

        unsigned int src_samples = bytes / (src_bps / 8);
        unsigned int src_frames = src_samples / src_ch;
        unsigned int dst_frames = src_frames;
        if (src_rate != dst_rate && src_frames > 0)
        {
            dst_frames = (unsigned int)((unsigned long long)src_frames * dst_rate / src_rate);
            if (dst_frames < 1) dst_frames = 1;
        }

        unsigned int total_out_bytes = dst_frames * dst_ch * (dst_bps / 8);

        if (total_out_bytes > convert_buf_size)
        {
            convert_buf = (unsigned char *)realloc(convert_buf, total_out_bytes);
            convert_buf_size = total_out_bytes;
        }

        short *out = (short *)convert_buf;
        for (unsigned int f = 0; f < dst_frames; f++)
        {
            // Find nearest source frame (sample rate conversion)
            unsigned int sf = (src_rate == dst_rate) ? f
                : (unsigned int)((unsigned long long)f * src_rate / dst_rate);
            if (sf >= src_frames) sf = src_frames - 1;

            // Read source sample(s)
            int l, r;
            if (src_bps == 8)
            {
                unsigned char *u8 = data;
                if (src_ch == 2)
                {
                    l = ((int)u8[sf * 2] - 128) * 256;
                    r = ((int)u8[sf * 2 + 1] - 128) * 256;
                }
                else
                {
                    l = r = ((int)u8[sf] - 128) * 256;
                }
            }
            else
            {
                short *s16 = (short *)data;
                if (src_ch == 2)
                {
                    l = s16[sf * 2];
                    r = s16[sf * 2 + 1];
                }
                else
                {
                    l = r = s16[sf];
                }
            }

            out[f * 2] = (short)l;
            out[f * 2 + 1] = (short)r;
        }

        *out_bytes = total_out_bytes;
        *out_rate = dst_rate;
        return convert_buf;
    }

    int Play(unsigned int dwReserved1, unsigned int dwReserved2, unsigned int dwFlags)
    {
        playing = true;
        looping = (dwFlags != 0);
        if (device_id)
        {
            SDL_PauseAudioDevice(device_id, 0);
            if (!looping && wfx.nAvgBytesPerSec > 0)
            {
                unsigned int existing = SDL_GetQueuedAudioSize(device_id);
                unsigned int device_bps = 22050 * 2 * 2; // device is fixed at 22050/16/stereo
                unsigned int offset_ms = (unsigned int)((double)existing / device_bps * 1000.0);
                unsigned int our_ms = (unsigned int)((double)buffer_size / wfx.nAvgBytesPerSec * 1000.0);
                play_start_time = SDL_GetTicks() - offset_ms;
                duration_ms = offset_ms + our_ms;
                fprintf(stderr, "Play: buf_size=%d avgBps=%d existing=%d offset_ms=%d our_ms=%d duration_ms=%d\n",
                    buffer_size, wfx.nAvgBytesPerSec, existing, offset_ms, our_ms, duration_ms);
            }
        }
        return 0;
    }

    int Stop()
    {
        playing = false;
        if (device_id)
        {
            SDL_PauseAudioDevice(device_id, 1);
            SDL_ClearQueuedAudio(device_id);
        }
        return 0;
    }

    int Release()
    {
        device_id = 0;
        if (buffer)
        {
            free(buffer);
            buffer = nullptr;
        }
        if (convert_buf)
        {
            free(convert_buf);
            convert_buf = nullptr;
            convert_buf_size = 0;
        }
        buffer_size = 0;
        return 0;
    }

    int SetVolume(int volume)
    {
        volume_dsb = volume;
        return 0;
    }

    int GetVolume(int *volume)
    {
        if (volume) *volume = volume_dsb;
        return 0;
    }

    int SetPan(int pan)
    {
        return 0;
    }

    int GetPan(int *pan)
    {
        if (pan) *pan = 0;
        return 0;
    }

    int GetCurrentPosition(unsigned int *pdwCurrentPlayCursor, unsigned int *pdwCurrentWriteCursor)
    {
        if (!device_id)
            return -1;
        unsigned int queued = SDL_GetQueuedAudioSize(device_id);
        unsigned int device_frame_size = 2 * 2; // device is 22050/16/stereo
        unsigned int buf_frame_size = wfx.nChannels * (wfx.wBitsPerSample / 8);
        unsigned int cursor = 0;
        if (device_frame_size > 0 && buf_frame_size > 0 && buffer_size > 0)
        {
            unsigned long long queued_frames = (unsigned long long)queued / device_frame_size;
            unsigned long long queued_buf_bytes = queued_frames * buf_frame_size;
            unsigned int consumed = (unsigned int)((queued_buf_bytes < buffer_size) ? (buffer_size - queued_buf_bytes) : 0);
            cursor = consumed % buffer_size;
        }
        if (pdwCurrentPlayCursor)
            *pdwCurrentPlayCursor = cursor;
        if (pdwCurrentWriteCursor)
            *pdwCurrentWriteCursor = write_cursor;
        return 0;
    }

    int GetStatus(unsigned int *pdwStatus)
    {
        if (pdwStatus)
        {
            if (playing && device_id)
            {
                if (looping)
                {
                    *pdwStatus = 1;
                }
                else if (duration_ms == 0)
                {
                    *pdwStatus = 1;
                }
                else
                {
                    unsigned int elapsed = SDL_GetTicks() - play_start_time;
                    *pdwStatus = (elapsed < duration_ms) ? 1 : 0;
                }
            }
            else
            {
                *pdwStatus = 0;
            }
        }
        return 0;
    }

    int GetCaps(DSBCAPS *pdsbcaps)
    {
        if (pdsbcaps)
        {
            pdsbcaps->dwBufferBytes = buffer_size;
        }
        return 0;
    }

    int Restore()
    {
        restored = true;
        return 0;
    }

    int SetCurrentPosition(unsigned int dwNewPosition)
    {
        if (device_id)
        {
            SDL_ClearQueuedAudio(device_id);
        }
        return 0;
    }
};

struct IDirectSound
{
    static const int MAX_SFX_DEVICES = 8;
    SDL_AudioDeviceID m_stream_device;   // shared device for BGM (streaming music)
    SDL_AudioDeviceID m_sfx_devices[MAX_SFX_DEVICES]; // pool for SFX
    int m_next_sfx_device;

    IDirectSound() : m_stream_device(0), m_next_sfx_device(0)
    {
        for (int i = 0; i < MAX_SFX_DEVICES; i++)
            m_sfx_devices[i] = 0;
    }

    int SetCooperativeLevel(void *hwnd, unsigned int dwLevel)
    {
        return 0;
    }

    void InitDevices()
    {
        SDL_AudioSpec desired;
        memset(&desired, 0, sizeof(desired));
        desired.freq = 22050;
        desired.format = AUDIO_S16SYS;
        desired.channels = 2;
        desired.samples = 4096;

        for (int i = 0; i < MAX_SFX_DEVICES; i++)
        {
            m_sfx_devices[i] = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
            fprintf(stderr, "InitDevices SFX[%d]: dev_id=%d\n", i, m_sfx_devices[i]);
        }
        m_stream_device = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
        fprintf(stderr, "InitDevices STREAM: dev_id=%d\n", m_stream_device);
    }

    // Pause/resume all audio devices (for menu pause)
    // Only pauses SFX devices — stream (BGM) keeps playing
    void PauseAll()
    {
        for (int i = 0; i < MAX_SFX_DEVICES; i++)
        {
            if (m_sfx_devices[i])
                SDL_PauseAudioDevice(m_sfx_devices[i], 1);
        }
    }

    void ResumeAll()
    {
        for (int i = 0; i < MAX_SFX_DEVICES; i++)
        {
            if (m_sfx_devices[i])
                SDL_PauseAudioDevice(m_sfx_devices[i], 0);
        }
    }

    int CreateSoundBuffer(DSBUFFERDESC *pcDSBufferDesc,
                          IDirectSoundBuffer **ppDSBuffer,
                          void *pUnkOuter)
    {
        IDirectSoundBuffer *buf = new IDirectSoundBuffer();
        if (pcDSBufferDesc)
        {
            buf->buffer_size = pcDSBufferDesc->dwBufferBytes;
            buf->buffer = (unsigned char *)malloc(buf->buffer_size);
            if (pcDSBufferDesc->lpwfxFormat)
            {
                memcpy(&buf->wfx, pcDSBufferDesc->lpwfxFormat, sizeof(WAVEFORMATEX));
            }
            if (buf->wfx.nSamplesPerSec > 0)
            {
                // Streaming (BGM, large 3s buffer) uses shared device
                // One-shot SFX use pre-opened device pool
                bool is_streaming = (pcDSBufferDesc->dwBufferBytes >= 110000);

                if (is_streaming)
                {
                    buf->device_id = m_stream_device;
                }
                else
                {
                    int idx = m_next_sfx_device % MAX_SFX_DEVICES;
                    m_next_sfx_device++;
                    buf->device_id = m_sfx_devices[idx];
                }
            }
            fprintf(stderr, "CreateSoundBuffer: buf_size=%d wfx.sr=%d wfx.ch=%d wfx.bps=%d dev_id=%d\n",
                buf->buffer_size, buf->wfx.nSamplesPerSec, buf->wfx.nChannels,
                buf->wfx.wBitsPerSample, buf->device_id);
            *ppDSBuffer = buf;
        }
        return 0;
    }

    int Release()
    {
        for (int i = 0; i < MAX_SFX_DEVICES; i++)
        {
            if (m_sfx_devices[i])
            {
                SDL_CloseAudioDevice(m_sfx_devices[i]);
                m_sfx_devices[i] = 0;
            }
        }
        if (m_stream_device)
        {
            SDL_CloseAudioDevice(m_stream_device);
            m_stream_device = 0;
        }
        return 0;
    }
};

inline int DirectSoundCreate(void *guid, IDirectSound **ppDS, void *pUnkOuter)
{
    *ppDS = new IDirectSound();
    return 0;
}

#define S_OK 0
#define DSERR_BUFFERLOST -1

// Types needed only if not provided by other headers
#ifndef BOOL
typedef int BOOL;
#endif
#ifndef SIZE_T
typedef size_t SIZE_T;
#endif
#ifndef LPDWORD
typedef unsigned int *LPDWORD;
#endif
#ifndef LPVOID
typedef void *LPVOID;
#endif
#ifndef HLOCAL
typedef void *HLOCAL;
#endif
#ifndef HWND
typedef void *HWND;
#endif
#ifndef LPLONG
typedef int *LPLONG;
#endif
#ifndef LPDIRECTSOUND
typedef IDirectSound *LPDIRECTSOUND;
#endif
