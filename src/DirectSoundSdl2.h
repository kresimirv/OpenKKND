#pragma once

#include <SDL2/SDL.h>
#include <cstring>
#include <cstdlib>

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

    IDirectSoundBuffer() : device_id(0), buffer(nullptr), buffer_size(0),
        write_cursor(0), playing(false), looping(false), restored(false)
    {
        memset(&wfx, 0, sizeof(wfx));
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
        if (device_id && playing)
        {
            if (pvAudioPtr1 && dwAudioBytes1 > 0)
            {
                SDL_QueueAudio(device_id, pvAudioPtr1, dwAudioBytes1);
            }
            if (pvAudioPtr2 && dwAudioBytes2 > 0)
            {
                SDL_QueueAudio(device_id, pvAudioPtr2, dwAudioBytes2);
            }
        }
        return 0;
    }

    int Play(unsigned int dwReserved1, unsigned int dwReserved2, unsigned int dwFlags)
    {
        playing = true;
        looping = (dwFlags != 0);
        if (device_id)
        {
            SDL_PauseAudioDevice(device_id, 0);
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
        if (device_id)
        {
            SDL_CloseAudioDevice(device_id);
            device_id = 0;
        }
        if (buffer)
        {
            free(buffer);
            buffer = nullptr;
        }
        buffer_size = 0;
        return 0;
    }

    int SetVolume(int volume)
    {
        return 0;
    }

    int GetVolume(int *volume)
    {
        if (volume) *volume = 0;
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
        unsigned int played_bytes = buffer_size > queued ? buffer_size - queued : 0;
        if (pdwCurrentPlayCursor)
            *pdwCurrentPlayCursor = played_bytes % buffer_size;
        if (pdwCurrentWriteCursor)
            *pdwCurrentWriteCursor = write_cursor;
        return 0;
    }

    int GetStatus(unsigned int *pdwStatus)
    {
        if (pdwStatus)
        {
            *pdwStatus = (playing && device_id && SDL_GetQueuedAudioSize(device_id) > 0) ? 1 : 0;
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
    IDirectSound() {}

    int SetCooperativeLevel(void *hwnd, unsigned int dwLevel)
    {
        return 0;
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
                SDL_AudioSpec desired;
                memset(&desired, 0, sizeof(desired));
                desired.freq = buf->wfx.nSamplesPerSec;
                desired.format = buf->wfx.wBitsPerSample == 16 ? AUDIO_S16SYS : AUDIO_U8;
                desired.channels = buf->wfx.nChannels;
                desired.samples = 4096;
                buf->device_id = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
            }
            *ppDSBuffer = buf;
        }
        return 0;
    }

    int Release()
    {
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
