#include <SDL2/SDL.h>

#include "src/Video.h"
#include "src/DirectSoundSdl2.h"

#include "src/_unsorted_data.h"
#include "src/_unsorted_functions.h"
#include "src/Render.h"
#include "src/Sound.h"
#include "src/Infrastructure/PlatformSpecific/OsTools.h"

static SDL_AudioDeviceID sdl_video_audio_dev = 0;
static Uint32 sdl_video_audio_play_start = 0;

DetailedDrawHandler_VideoPlayer stru_477D90; // weak
int dword_477DB8; // weak
int dword_477DBC; // weak
DrawJob *video_477DC0;
int video_477DC4_current_sound_position; // weak
VideoFile *video;
int video_477DEC_is_sound_playing; // weak
int video_477DF0; // weak
int dword_477DF4; // weak
char video_477DF8_subtitles[756];
int video_4780EC; // weak
int video_4780F0; // weak
int video_4780F4; // weak
RenderString *video_4780F8_subtitles;
VideoFileFrame *video_47F434_frame;
DetailedDrawHandler_VideoPlayer _477948_video_draw_details;
int _477970_video_header_field_6; // weak
int _477974_video_header_field_8; // weak
DrawJob *video_477978_draw_job;
int dword_477940; // weak
Palette _477990_video_palette; // idb

// 64 pattern masks for VBC op 3 (sub_45A556 through sub_45B498)
// 16-bit mask: bit (y*4+x) = 1 means pixel uses c1 (high byte of color word)
// Extracted by simulating the original pattern functions
static const unsigned short vbc_pattern_masks[64] = {
    0x0660, 0xFF00, 0xCCCC, 0xF000, 0x8888, 0x000F, 0x1111, 0xFEC8,
    0x8CEF, 0x137F, 0xF731, 0xC800, 0x008C, 0x0013, 0x3100, 0xCC00,
    0x00CC, 0x0033, 0x3300, 0x0FF0, 0x6666, 0x00F0, 0x0F00, 0x2222,
    0x4444, 0xF600, 0x8CC8, 0x006F, 0x1331, 0x318C, 0xC813, 0x33CC,
    0x6600, 0x0CC0, 0x0066, 0x0330, 0xF900, 0xC88C, 0x009F, 0x3113,
    0x6000, 0x0880, 0x0006, 0x0110, 0xCC88, 0xFC00, 0x00CF, 0x88CC,
    0x003F, 0x1133, 0x3311, 0xF300, 0x6FF6, 0x0603, 0x08C6, 0x8C63,
    0xC631, 0x6310, 0xC060, 0x0136, 0x136C, 0x36C8, 0x6C80, 0x324C,
};

//----- (0040CAE0) --------------------------------------------------------
int VIDEO_IsAllocated()
{
    int result; // eax@2

    if (video)
    {
        result = 0;
    }
    else
    {
        video_477DF0 = 0;
        result = 1;
    }
    return result;
}

//----- (0040CB00) --------------------------------------------------------
int VIDEO_ReadAndAllocDrawJob(const char *a1, int x, int y, int z_index)
{
    int v4; // esi@1
    int result; // eax@1
    DrawJob *v6; // eax@2

    v4 = x;
    result = (int)VIDEO_ReadFile(a1);
    video = (VideoFile *)result;
    if (result)
    {
        v6 = draw_list_add(0, 0);
        video_477978_draw_job = v6;
        if (v6)
        {
            v6->job_details.x = v4;
            video_477978_draw_job->job_details.y = y;
            video_477978_draw_job->job_details.z_index = z_index;
            video_477978_draw_job->job_details.image = &_477948_video_draw_details;
            _477970_video_header_field_6 = video->header.field_6;
            _477974_video_header_field_8 = video->header.field_8;
            _477948_video_draw_details.width = video->header.width;
            result = 1;
            _477948_video_draw_details.height = video->header.height;
            _477948_video_draw_details.handler = render_video_draw_handler;
            _477948_video_draw_details.field_14 = 1;
            video_477DC0 = 0;
            video_477DF0 = 0;
        }
        else
        {
            VIDEO_Clean(video);
            video = 0;
            result = 0;
        }
    }
    return result;
}

//----- (0040CBD0) --------------------------------------------------------
int VIDEO_DoFrame()
{
    signed __int64 v2;
    int v5;
    int result;
    int v7;
    VideoFile *v8;
    Uint32 v13;
    VideoFile *v14;
    signed __int64 v15;
    int v18;
    VideoFile *v19;
    int v20;
    int v21;
    char v23;
    int v24;

    if (!video)
        return 0;
    if (!dword_477DF4)
    {
        if (video->header.current_frame >= video->header.num_frames - 1)
        {
            v2 = sdl_video_audio_dev ? (signed __int64)(SDL_GetTicks() - sdl_video_audio_play_start) : (signed __int64)SDL_GetTicks();
            v5 = (unsigned int)v2 < dword_477940;
            if (!v5 && video)
            {
                if (video_477DEC_is_sound_playing)
                {
                    if (sdl_video_audio_dev)
                    {
                        SDL_PauseAudioDevice(sdl_video_audio_dev, 1);
                        SDL_CloseAudioDevice(sdl_video_audio_dev);
                        sdl_video_audio_dev = 0;
                    }
                    video_477DEC_is_sound_playing = 0;
                }
                VIDEO_Clean(video);
                video = 0;
                video_477978_draw_job->job_details.image = 0;
                video_477978_draw_job->flags |= 0x80000000;
                video_477978_draw_job = 0;
                if (video_477DC0)
                {
                    video_477DC0->job_details.image = 0;
                    video_477DC0->flags |= 0x80000000;
                    video_477DC0 = 0;
                    return 0;
                }
            }
            return 0;
        }
        VIDEO_ReadNextFrame(video);
        v7 = video->header.num_sound_bytes;
        if (v7)
        {
            v8 = video;
            if (video_477DEC_is_sound_playing && sdl_video_audio_dev)
            {
                SDL_QueueAudio(sdl_video_audio_dev, (const void *)v8->header.ptr_20, v7);
            }
            else if (!video_477DEC_is_sound_playing)
            {
                VIDEO_40D090(video);
            }
        }
        if (video->header.current_frame)
        {
            video_477DF0 = 1;
        }
        else if (video_477DEC_is_sound_playing)
        {
            dword_477940 = 0;
        }
        else
        {
            v13 = SDL_GetTicks();
            dword_477940 = v13;
        }
        dword_477DF4 = 1;
    }
    v14 = video;
    if (video_477DEC_is_sound_playing && sdl_video_audio_dev)
    {
        LODWORD(v15) = (signed __int64)(SDL_GetTicks() - sdl_video_audio_play_start);
    }
    else
    {
        LODWORD(v15) = SDL_GetTicks();
    }
    v18 = (unsigned int)v15 < dword_477940;
    if (v18)
    {
        video_477DF0 = 0;
        return 0;
    }
    v19 = video;
    dword_477DF4 = 0;
    dword_477940 += video->header.field_E;
    if (video->field_330 & 0x40)
    {
        VIDEO_40D450((char *)video->header.field_28, video->header.field_24);
        v19 = video;
    }
    if (*(_WORD *)&v19->gap2C[0])
    {
        v20 = *(_WORD *)&v19->gap2C[2];
        v21 = 3 * v20;
        PaletteEntry *pal_entry = &_477990_video_palette.entires[v20];
        do
        {
            v23 = v19->gap2C[v21 + 4];
            v21 += 3;
            pal_entry->peRed = v23;
            pal_entry->peGreen = v19->gap2C[v21 + 2];
            pal_entry->peBlue = v19->gap2C[v21 + 3];
            --*(_WORD *)&v19->gap2C[0];
            v19 = video;
            ++pal_entry;
        } while (*(_WORD *)&video->gap2C[0]);
        _40E400_set_palette(&_477990_video_palette);
        v19 = video;
    }
    if (!video_477DF0)
    {
        _477948_video_draw_details._18_img_data = (void *)v19->header.field_10;
        v24 = v19->header.field_10;
        result = 1;
        if (v24)
            stru_477D90._18_img_data = (void *)(v24 + _477948_video_draw_details.height * _477948_video_draw_details.width);
        else
            stru_477D90._18_img_data = NULL;
        return result;
    }
    return 0;
}
// 477940: using guessed type int dword_477940;
// 477D90: using guessed type DetailedDrawHandler_VideoPlayer stru_477D90;
// 477DEC: using guessed type int video_477DEC_is_sound_playing;
// 477DF0: using guessed type int video_477DF0;
// 477DF4: using guessed type int dword_477DF4;

//----- (0040D090) --------------------------------------------------------
void VIDEO_40D090(VideoFile *video)
{
    int bits_per_sample = (video->header.field_18 & 0xFF) != 8 ? 16 : 8;
    int channels = (video->header.field_18 & 0x100) ? 2 : 1;

    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = video->header._14_looks_like_fps;
    desired.format = (bits_per_sample == 8) ? AUDIO_U8 : AUDIO_S16SYS;
    desired.channels = channels;
    desired.samples = 4096;

    sdl_video_audio_dev = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (!sdl_video_audio_dev)
    {
        fprintf(stderr, "VIDEO_40D090: SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return;
    }

    SDL_QueueAudio(sdl_video_audio_dev, (const void *)video->header.ptr_20, video->header.num_sound_bytes);
    sdl_video_audio_play_start = SDL_GetTicks();
    SDL_PauseAudioDevice(sdl_video_audio_dev, 0);
    video_477DEC_is_sound_playing = 1;
}

//----- (0040D230) --------------------------------------------------------
void VIDEO_free()
{
    if (video)
    {
        if (video_477DEC_is_sound_playing)
        {
            if (sdl_video_audio_dev)
            {
                SDL_PauseAudioDevice(sdl_video_audio_dev, 1);
                SDL_CloseAudioDevice(sdl_video_audio_dev);
                sdl_video_audio_dev = 0;
            }
            video_477DEC_is_sound_playing = 0;
        }
        VIDEO_Clean(video);
        video = 0;
        video_477978_draw_job->job_details.image = 0;
        video_477978_draw_job->flags |= 0x80000000;
        video_477978_draw_job = 0;
        if (video_477DC0)
        {
            video_477DC0->job_details.image = 0;
            video_477DC0->flags |= 0x80000000;
            video_477DC0 = 0;
        }
    }
}

//----- (0040D2C0) --------------------------------------------------------
bool VIDEO_IsVideoInvalid()
{
    return video == 0;
}

//----- (0040D2D0) --------------------------------------------------------
bool VIDEO_ReadAndAllocDrawJob_2(const char *a1, stru1_draw_params *a2, stru1_draw_params *a3, RenderString *a4, int width, int height, int z_index)
{
    stru1_draw_params *v7; // ebx@1
    BOOL result; // eax@2
    DrawJob *v9; // eax@5
    int v10; // ecx@8

    v7 = a2;
    video = VIDEO_ReadFile(a1);
    if (video)
    {
        if (a3)
            video_477DC0 = draw_list_add(0, 0);
        v9 = draw_list_add(0, 0);
        video_477978_draw_job = v9;
        if (v9)
        {
            v9->job_details.params = v7;
            video_477978_draw_job->job_details.x = 0;
            video_477978_draw_job->job_details.y = 0;
            video_477978_draw_job->job_details.z_index = z_index;
            video_477978_draw_job->job_details.image = &_477948_video_draw_details;
            _477970_video_header_field_6 = video->header.field_6;
            _477974_video_header_field_8 = video->header.field_8;
            _477948_video_draw_details.width = width;
            _477948_video_draw_details.height = height;
            _477948_video_draw_details.handler = render_video_draw_handler;
            _477948_video_draw_details.field_14 = 0;
            if (a3)
            {
                video_477DC0->job_details.params = a3;
                video_477DC0->job_details.x = 0;
                video_477DC0->job_details.y = 0;
                video_477DC0->job_details.z_index = z_index;
                video_477DC0->job_details.image = &stru_477D90;
                dword_477DB8 = 0;
                dword_477DBC = 0;
                stru_477D90.width = video->header.width;
                v10 = video->header.height - height;
                stru_477D90.handler = render_video_draw_handler;
                stru_477D90.height = v10;
                stru_477D90.field_14 = 0;
            }
            else
            {
                video_477DC0 = 0;
            }
            if (a4)
                VIDEO_40D430(a4);
            video_477DF0 = 0;
            result = 1;
        }
        else
        {
            VIDEO_Clean(video);
            video = 0;
            result = 0;
        }
    }
    else
    {
        result = 0;
    }
    return result;
}

//----- (0040D430) --------------------------------------------------------
void VIDEO_40D430(RenderString *a1)
{
    video_4780F8_subtitles = a1;
    video_4780F0 = 0;
    video_4780F4 = 0;
    video_4780EC = 0;
}

//----- (0040D450) --------------------------------------------------------
void VIDEO_40D450(char *a1, int a2)
{
    char *v2; // esi@1
    int v3; // edi@3
    char v4; // al@4
    bool v5; // sf@6
    unsigned __int8 v6; // of@6
    char *v7; // eax@10

    v2 = a1;
    if (video_4780F8_subtitles && a2 > 0)
    {
        v3 = a2;
        do
        {
            v4 = *v2++;
            if (v4 >= 32 && v4 < 127)
            {
                video_477DF8_subtitles[video_4780F0++] = v4;
                v6 = __OFSUB__(video_4780F4 + 1, 23);
                v5 = video_4780F4++ - 22 < 0;
                if (!(v5 ^ v6))
                    v4 = '\n';
            }
            if (v4 == '\n')
            {
                ++video_4780EC;
                video_477DF8_subtitles[video_4780F0++] = '\n';
                video_4780F4 = 0;
                if (video_4780EC >= 26)
                {
                    v7 = strchr(video_477DF8_subtitles, '\n') + 1;
                    video_4780F0 -= v7 - video_477DF8_subtitles;
                    --video_4780EC;
                    memmove(video_477DF8_subtitles, v7, 675 - (v7 - video_477DF8_subtitles));
                    render_string_445AE0(video_4780F8_subtitles);
                }
            }
            video_477DF8_subtitles[video_4780F0] = 0;
            video_4780F8_subtitles->field_18 = 0;
            video_4780F8_subtitles->num_lines = 0;
            render_string_445770(video_4780F8_subtitles, video_477DF8_subtitles, 0);
            --v3;
        } while (v3);
    }
}


//----- (00459E80) --------------------------------------------------------
VideoFile *VIDEO_ReadFile(const char *video_name)
{
    VideoFile *result; // eax@1
    VideoFile *v2; // ebx@1
    VideoFile *v3; // esi@2
    __int32 v4; // eax@4
    char v5; // dl@4
    SIZE_T v6; // edi@4
    HLOCAL v7; // eax@4
    void *v8; // ecx@4
    __int16 v9; // bp@6
    int v10; // edi@8
    int v11; // eax@9
    int v12; // ebx@10
    int v13; // edx@10

    result = (VideoFile *)j_fopen_weirdmode_3(video_name);
    v2 = result;
    if (result)
    {
        v3 = (VideoFile *)malloc(0x20748u);
        if (v3)
        {
            memset(v3, 0, 0x748u);
            video_47F434_frame = &v3->_744_frame;
            fread(&v3->_744_frame, 0x3Cu, 1u, (FILE *)v2);
            v3->header._0__first_4_bits_eq_bpp = video_47F434_frame->_14__first_4_bits_eq_bpp;
            v3->header.width = video_47F434_frame->width;
            v3->header.height = video_47F434_frame->height;
            v3->header.num_frames = video_47F434_frame->num_frames;
            v3->header.field_6 = video_47F434_frame->field_1A;
            v3->header.field_8 = video_47F434_frame->field_1C;
            v3->header._14_looks_like_fps = video_47F434_frame->_22_looks_like_fps;
            v3->header.field_18 = video_47F434_frame->field_20;
            v3->header.num_sound_bytes = 0;
            v3->header.current_frame = -1;
            v3->file = (FILE *)v2;
            v4 = ftell((FILE *)v2);
            v5 = v3->header._0__first_4_bits_eq_bpp;
            v3->data_offset = v4;
            v6 = v3->header.height * v3->header.width * (v5 & 0xF);
            v3->field_332 = 1;
            v3->frame_front_buffer = calloc(1, v6);
            v7 = calloc(1, v6);
            v8 = v3->frame_front_buffer;
            v3->frame_back_buffer = v7;
            v9 = !v8 || !v7;
            v10 = -8;
            do
            {
                v11 = -8;
                do
                {
                    v12 = (v3->header._0__first_4_bits_eq_bpp & 0xF) * (v11 + v3->header.width * v10);
                    v13 = 16 * (v10 & 0xF) | v11++ & 0xF;
                    v3->_344_prolly_palette[v13] = v12;
                } while (v11 < 8);
                ++v10;
            } while (v10 < 8);
            if (v9)
            {
                VIDEO_Clean(v3);
                result = 0;
            }
            else
            {
                result = v3;
            }
        }
        else
        {
            free(0);
            fclose((FILE *)v2);
            result = 0;
        }
    }
    return result;
}

//----- (0045A030) --------------------------------------------------------
void VIDEO_Clean(VideoFile *video)
{
    FILE *v1; // eax@2

    if (video)
    {
        v1 = video->file;
        if (v1)
            fclose(v1);
        free(video->frame_back_buffer);
        free(video->frame_front_buffer);
        free(video);
    }
}

//----- (0045A070) --------------------------------------------------------
int VIDEO_ReadNextFrame(VideoFile *a1)
{
    VideoFileFrame *v1; // edi@3
    int v2; // edx@6
    int result; // eax@8

    if (++a1->header.current_frame == a1->header.num_frames)
    {
        fseek(a1->file, a1->data_offset, 0);
        a1->header.current_frame = 0;
    }
    v1 = &a1->_744_frame;
    if (a1->header.current_frame)
        v1->frame_size = *(int *)((char *)&a1->_744_frame.frame_size + v1->frame_size);
    else
        fread(&a1->_744_frame, 4u, 1u, a1->file);
    v2 = v1->frame_size;
    if (a1->header.current_frame == a1->header.num_frames - 1)
        v2 -= 4;
    fread(&a1->_744_frame.field_4, v2, 1u, a1->file);
    video_45A110(a1, &a1->_744_frame);
    LOWORD_HEXRAYS(result) = 0;
    return result;
}

//----- (0045A48E) --------------------------------------------------------
// 8-bit VBC decoder
// Each control byte encodes 4 operations for 4 consecutive 4x4 tiles.
_BYTE *sub_45A48E(unsigned __int16 tile_cols, __int16 tile_rows, _BYTE *data, int front_buf, int back_buf, int offset, int width, int palette_ptr)
{
    dword_476AC4 = palette_ptr;
    dword_476AC0 = offset + back_buf - front_buf;

    // Pre-copy back buffer to front buffer with global motion (matching C# ApplyFrame)
    int frame_size_8 = width * tile_rows * 4;
    if (offset >= 0)
    {
        int copy_size = frame_size_8 - offset;
        if (copy_size > 0)
            memcpy((void *)front_buf, (void *)(back_buf + offset), copy_size);
    }
    else
    {
        int copy_size = frame_size_8 + offset;
        if (copy_size > 0)
            memcpy((void *)(front_buf - offset), (void *)back_buf, copy_size);
    }

    _BYTE *in = data;
    _BYTE *out = (_BYTE *)front_buf;
    int stride = width;
    int group_adv = 4 * 4;  // 4 tiles × 4 bytes
    int row_adv = 12 * tile_cols;
    int pair_cnt = tile_cols >> 1;
    _BYTE *mid;

    while (1)
    {
        unsigned char ctl = *in++;

        // tile 0: op = (ctl >> 6) & 3
        switch ((ctl >> 6) & 3)
        {
        case 0:
        {
            break;
        }
        case 1:
        {
            unsigned char flag = *in++;
            if (flag == 0)
            {
                *(unsigned int *)out = *(unsigned int *)in; in += 4;
                *(unsigned int *)(out + stride) = *(unsigned int *)in; in += 4;
                *(unsigned int *)(out + 2 * stride) = *(unsigned int *)in; in += 4;
                *(unsigned int *)(out + 3 * stride) = *(unsigned int *)in; in += 4;
            }
            else
            {
                int adj = ((int *)palette_ptr)[flag];
                _BYTE *ref_tile = out + dword_476AC0 + adj;
                *(unsigned int *)out = *(unsigned int *)ref_tile;
                *(unsigned int *)(out + stride) = *(unsigned int *)(ref_tile + stride);
                *(unsigned int *)(out + 2 * stride) = *(unsigned int *)(ref_tile + 2 * stride);
                *(unsigned int *)(out + 3 * stride) = *(unsigned int *)(ref_tile + 3 * stride);
            }
            break;
        }
        case 2:
        {
            unsigned char color = *in++;
            unsigned int d = color | (color << 8) | (color << 16) | (color << 24);
            *(unsigned int *)out = d;
            *(unsigned int *)(out + stride) = d;
            *(unsigned int *)(out + 2 * stride) = d;
            *(unsigned int *)(out + 3 * stride) = d;
            break;
        }
        case 3:
        {
            unsigned char sel = *in++;
            unsigned short mask = vbc_pattern_masks[sel & 0x3F];
            switch (sel >> 6)
            {
            case 0:
            {
                unsigned short val = *(unsigned short *)in; in += 2;
                unsigned char c0 = val & 0xFF;
                unsigned char c1 = (val >> 8) & 0xFF;
                out[0] = (mask & 0x0001) ? c1 : c0;
                out[1] = (mask & 0x0002) ? c1 : c0;
                out[2] = (mask & 0x0004) ? c1 : c0;
                out[3] = (mask & 0x0008) ? c1 : c0;
                out[stride] = (mask & 0x0010) ? c1 : c0;
                out[stride + 1] = (mask & 0x0020) ? c1 : c0;
                out[stride + 2] = (mask & 0x0040) ? c1 : c0;
                out[stride + 3] = (mask & 0x0080) ? c1 : c0;
                out[2 * stride] = (mask & 0x0100) ? c1 : c0;
                out[2 * stride + 1] = (mask & 0x0200) ? c1 : c0;
                out[2 * stride + 2] = (mask & 0x0400) ? c1 : c0;
                out[2 * stride + 3] = (mask & 0x0800) ? c1 : c0;
                out[3 * stride] = (mask & 0x1000) ? c1 : c0;
                out[3 * stride + 1] = (mask & 0x2000) ? c1 : c0;
                out[3 * stride + 2] = (mask & 0x4000) ? c1 : c0;
                out[3 * stride + 3] = (mask & 0x8000) ? c1 : c0;
                break;
            }
            case 1:
            {
                unsigned char color = *in++;
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++)
                        if (mask & (1 << (y * 4 + x)))
                            out[y * stride + x] = color;
                break;
            }
            case 2:
            {
                unsigned char color = *in++;
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++)
                        if (!(mask & (1 << (y * 4 + x))))
                            out[y * stride + x] = color;
                break;
            }
            case 3:
            {
                unsigned char color = *in++;
                unsigned int d = color | (color << 8) | (color << 16) | (color << 24);
                *(unsigned int *)out = d;
                *(unsigned int *)(out + stride) = d;
                *(unsigned int *)(out + 2 * stride) = d;
                *(unsigned int *)(out + 3 * stride) = d;
                break;
            }
            }
            break;
        }
        }

        // tile 1: op = (ctl >> 4) & 3
        switch ((ctl >> 4) & 3)
        {
        case 0:
        {
            break;
        }
        case 1:
        {
            unsigned char flag = *in++;
            if (flag == 0)
            {
                *(unsigned int *)(out + 4) = *(unsigned int *)in; in += 4;
                *(unsigned int *)(out + 4 + stride) = *(unsigned int *)in; in += 4;
                *(unsigned int *)(out + 4 + 2 * stride) = *(unsigned int *)in; in += 4;
                *(unsigned int *)(out + 4 + 3 * stride) = *(unsigned int *)in; in += 4;
            }
            else
            {
                int adj = ((int *)palette_ptr)[flag];
                _BYTE *ref_tile = out + 4 + dword_476AC0 + adj;
                *(unsigned int *)(out + 4) = *(unsigned int *)ref_tile;
                *(unsigned int *)(out + 4 + stride) = *(unsigned int *)(ref_tile + stride);
                *(unsigned int *)(out + 4 + 2 * stride) = *(unsigned int *)(ref_tile + 2 * stride);
                *(unsigned int *)(out + 4 + 3 * stride) = *(unsigned int *)(ref_tile + 3 * stride);
            }
            break;
        }
        case 2:
        {
            unsigned char color = *in++;
            unsigned int d = color | (color << 8) | (color << 16) | (color << 24);
            *(unsigned int *)(out + 4) = d;
            *(unsigned int *)(out + 4 + stride) = d;
            *(unsigned int *)(out + 4 + 2 * stride) = d;
            *(unsigned int *)(out + 4 + 3 * stride) = d;
            break;
        }
        case 3:
        {
            unsigned char sel = *in++;
            unsigned short mask = vbc_pattern_masks[sel & 0x3F];
            _BYTE *t = out + 4;
            switch (sel >> 6)
            {
            case 0:
            {
                unsigned short val = *(unsigned short *)in; in += 2;
                unsigned char c0 = val & 0xFF;
                unsigned char c1 = (val >> 8) & 0xFF;
                t[0] = (mask & 0x0001) ? c1 : c0;
                t[1] = (mask & 0x0002) ? c1 : c0;
                t[2] = (mask & 0x0004) ? c1 : c0;
                t[3] = (mask & 0x0008) ? c1 : c0;
                t[stride] = (mask & 0x0010) ? c1 : c0;
                t[stride + 1] = (mask & 0x0020) ? c1 : c0;
                t[stride + 2] = (mask & 0x0040) ? c1 : c0;
                t[stride + 3] = (mask & 0x0080) ? c1 : c0;
                t[2 * stride] = (mask & 0x0100) ? c1 : c0;
                t[2 * stride + 1] = (mask & 0x0200) ? c1 : c0;
                t[2 * stride + 2] = (mask & 0x0400) ? c1 : c0;
                t[2 * stride + 3] = (mask & 0x0800) ? c1 : c0;
                t[3 * stride] = (mask & 0x1000) ? c1 : c0;
                t[3 * stride + 1] = (mask & 0x2000) ? c1 : c0;
                t[3 * stride + 2] = (mask & 0x4000) ? c1 : c0;
                t[3 * stride + 3] = (mask & 0x8000) ? c1 : c0;
                break;
            }
            case 1:
            {
                unsigned char color = *in++;
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++)
                        if (mask & (1 << (y * 4 + x)))
                            t[y * stride + x] = color;
                break;
            }
            case 2:
            {
                unsigned char color = *in++;
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++)
                        if (!(mask & (1 << (y * 4 + x))))
                            t[y * stride + x] = color;
                break;
            }
            case 3:
            {
                unsigned char color = *in++;
                unsigned int d = color | (color << 8) | (color << 16) | (color << 24);
                *(unsigned int *)t = d;
                *(unsigned int *)(t + stride) = d;
                *(unsigned int *)(t + 2 * stride) = d;
                *(unsigned int *)(t + 3 * stride) = d;
                break;
            }
            }
            break;
        }
        }

        mid = out + 8;
        int v20 = pair_cnt - 1;
        if (!v20)
        {
            v20 = tile_cols >> 1;
            mid += row_adv;
            if (!--tile_rows) return in;
        }

        // tile 2: op = (ctl >> 2) & 3
        switch ((ctl >> 2) & 3)
        {
        case 0:
        {
            break;
        }
        case 1:
        {
            unsigned char flag = *in++;
            if (flag == 0)
            {
                *(unsigned int *)mid = *(unsigned int *)in; in += 4;
                *(unsigned int *)(mid + stride) = *(unsigned int *)in; in += 4;
                *(unsigned int *)(mid + 2 * stride) = *(unsigned int *)in; in += 4;
                *(unsigned int *)(mid + 3 * stride) = *(unsigned int *)in; in += 4;
            }
            else
            {
                int adj = ((int *)palette_ptr)[flag];
                _BYTE *ref_tile = mid + dword_476AC0 + adj;
                *(unsigned int *)mid = *(unsigned int *)ref_tile;
                *(unsigned int *)(mid + stride) = *(unsigned int *)(ref_tile + stride);
                *(unsigned int *)(mid + 2 * stride) = *(unsigned int *)(ref_tile + 2 * stride);
                *(unsigned int *)(mid + 3 * stride) = *(unsigned int *)(ref_tile + 3 * stride);
            }
            break;
        }
        case 2:
        {
            unsigned char color = *in++;
            unsigned int d = color | (color << 8) | (color << 16) | (color << 24);
            *(unsigned int *)mid = d;
            *(unsigned int *)(mid + stride) = d;
            *(unsigned int *)(mid + 2 * stride) = d;
            *(unsigned int *)(mid + 3 * stride) = d;
            break;
        }
        case 3:
        {
            unsigned char sel = *in++;
            unsigned short mask = vbc_pattern_masks[sel & 0x3F];
            switch (sel >> 6)
            {
            case 0:
            {
                unsigned short val = *(unsigned short *)in; in += 2;
                unsigned char c0 = val & 0xFF;
                unsigned char c1 = (val >> 8) & 0xFF;
                mid[0] = (mask & 0x0001) ? c1 : c0;
                mid[1] = (mask & 0x0002) ? c1 : c0;
                mid[2] = (mask & 0x0004) ? c1 : c0;
                mid[3] = (mask & 0x0008) ? c1 : c0;
                mid[stride] = (mask & 0x0010) ? c1 : c0;
                mid[stride + 1] = (mask & 0x0020) ? c1 : c0;
                mid[stride + 2] = (mask & 0x0040) ? c1 : c0;
                mid[stride + 3] = (mask & 0x0080) ? c1 : c0;
                mid[2 * stride] = (mask & 0x0100) ? c1 : c0;
                mid[2 * stride + 1] = (mask & 0x0200) ? c1 : c0;
                mid[2 * stride + 2] = (mask & 0x0400) ? c1 : c0;
                mid[2 * stride + 3] = (mask & 0x0800) ? c1 : c0;
                mid[3 * stride] = (mask & 0x1000) ? c1 : c0;
                mid[3 * stride + 1] = (mask & 0x2000) ? c1 : c0;
                mid[3 * stride + 2] = (mask & 0x4000) ? c1 : c0;
                mid[3 * stride + 3] = (mask & 0x8000) ? c1 : c0;
                break;
            }
            case 1:
            {
                unsigned char color = *in++;
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++)
                        if (mask & (1 << (y * 4 + x)))
                            mid[y * stride + x] = color;
                break;
            }
            case 2:
            {
                unsigned char color = *in++;
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++)
                        if (!(mask & (1 << (y * 4 + x))))
                            mid[y * stride + x] = color;
                break;
            }
            case 3:
            {
                unsigned char color = *in++;
                unsigned int d = color | (color << 8) | (color << 16) | (color << 24);
                *(unsigned int *)mid = d;
                *(unsigned int *)(mid + stride) = d;
                *(unsigned int *)(mid + 2 * stride) = d;
                *(unsigned int *)(mid + 3 * stride) = d;
                break;
            }
            }
            break;
        }
        }

        // tile 3: op = ctl & 3
        switch (ctl & 3)
        {
        case 0:
        {
            break;
        }
        case 1:
        {
            unsigned char flag = *in++;
            if (flag == 0)
            {
                *(unsigned int *)(mid + 4) = *(unsigned int *)in; in += 4;
                *(unsigned int *)(mid + 4 + stride) = *(unsigned int *)in; in += 4;
                *(unsigned int *)(mid + 4 + 2 * stride) = *(unsigned int *)in; in += 4;
                *(unsigned int *)(mid + 4 + 3 * stride) = *(unsigned int *)in; in += 4;
            }
            else
            {
                int adj = ((int *)palette_ptr)[flag];
                _BYTE *ref_tile = mid + 4 + dword_476AC0 + adj;
                *(unsigned int *)(mid + 4) = *(unsigned int *)ref_tile;
                *(unsigned int *)(mid + 4 + stride) = *(unsigned int *)(ref_tile + stride);
                *(unsigned int *)(mid + 4 + 2 * stride) = *(unsigned int *)(ref_tile + 2 * stride);
                *(unsigned int *)(mid + 4 + 3 * stride) = *(unsigned int *)(ref_tile + 3 * stride);
            }
            break;
        }
        case 2:
        {
            unsigned char color = *in++;
            unsigned int d = color | (color << 8) | (color << 16) | (color << 24);
            *(unsigned int *)(mid + 4) = d;
            *(unsigned int *)(mid + 4 + stride) = d;
            *(unsigned int *)(mid + 4 + 2 * stride) = d;
            *(unsigned int *)(mid + 4 + 3 * stride) = d;
            break;
        }
        case 3:
        {
            unsigned char sel = *in++;
            unsigned short mask = vbc_pattern_masks[sel & 0x3F];
            _BYTE *t = mid + 4;
            switch (sel >> 6)
            {
            case 0:
            {
                unsigned short val = *(unsigned short *)in; in += 2;
                unsigned char c0 = val & 0xFF;
                unsigned char c1 = (val >> 8) & 0xFF;
                t[0] = (mask & 0x0001) ? c1 : c0;
                t[1] = (mask & 0x0002) ? c1 : c0;
                t[2] = (mask & 0x0004) ? c1 : c0;
                t[3] = (mask & 0x0008) ? c1 : c0;
                t[stride] = (mask & 0x0010) ? c1 : c0;
                t[stride + 1] = (mask & 0x0020) ? c1 : c0;
                t[stride + 2] = (mask & 0x0040) ? c1 : c0;
                t[stride + 3] = (mask & 0x0080) ? c1 : c0;
                t[2 * stride] = (mask & 0x0100) ? c1 : c0;
                t[2 * stride + 1] = (mask & 0x0200) ? c1 : c0;
                t[2 * stride + 2] = (mask & 0x0400) ? c1 : c0;
                t[2 * stride + 3] = (mask & 0x0800) ? c1 : c0;
                t[3 * stride] = (mask & 0x1000) ? c1 : c0;
                t[3 * stride + 1] = (mask & 0x2000) ? c1 : c0;
                t[3 * stride + 2] = (mask & 0x4000) ? c1 : c0;
                t[3 * stride + 3] = (mask & 0x8000) ? c1 : c0;
                break;
            }
            case 1:
            {
                unsigned char color = *in++;
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++)
                        if (mask & (1 << (y * 4 + x)))
                            t[y * stride + x] = color;
                break;
            }
            case 2:
            {
                unsigned char color = *in++;
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++)
                        if (!(mask & (1 << (y * 4 + x))))
                            t[y * stride + x] = color;
                break;
            }
            case 3:
            {
                unsigned char color = *in++;
                unsigned int d = color | (color << 8) | (color << 16) | (color << 24);
                *(unsigned int *)t = d;
                *(unsigned int *)(t + stride) = d;
                *(unsigned int *)(t + 2 * stride) = d;
                *(unsigned int *)(t + 3 * stride) = d;
                break;
            }
            }
            break;
        }
        }

        out = mid + 8;
        pair_cnt = v20 - 1;
        if (!pair_cnt)
        {
            out += row_adv;
            if (!--tile_rows) return in;
            pair_cnt = tile_cols >> 1;
        }
    }
}

//----- (0045D3B8) --------------------------------------------------------
// 16-bit VBC decoder
// Each control byte encodes 4 operations for 4 consecutive 4x4 tiles.
// Operations (same mapping as 8-bit):
//   0: reference copy (0 extra bytes)
//   1: raw/palette copy (1 flag byte; if 0: 32 inline bytes, else: palette-mod ref)
//   2: solid fill (2 bytes color)
//   3: pattern dispatch (1 selector byte; if bit6|bit7: 2 byte operand, else: 4 byte operand)
_BYTE *sub_45D3B8(unsigned __int16 tile_cols, __int16 tile_rows, _BYTE *data, int front_buf, int back_buf, int offset, int width, int palette_ptr, int mode)
{
    dword_476DDC = palette_ptr;
    dword_476DD8 = 2 * offset + back_buf - front_buf;

    // Pre-copy back buffer to front buffer with global motion (matching C# ApplyFrame)
    int frame_size_16 = 2 * width * tile_rows * 4;
    int byte_offset_16 = 2 * offset;
    if (byte_offset_16 >= 0)
    {
        int copy_size = frame_size_16 - byte_offset_16;
        if (copy_size > 0)
            memcpy((void *)front_buf, (void *)(back_buf + byte_offset_16), copy_size);
    }
    else
    {
        int copy_size = frame_size_16 + byte_offset_16;
        if (copy_size > 0)
            memcpy((void *)(front_buf - byte_offset_16), (void *)back_buf, copy_size);
    }

    unsigned char *in = data;
    unsigned char *out = (unsigned char *)front_buf;
    unsigned char *ref = (unsigned char *)front_buf + dword_476DD8;
    int stride = 2 * width;
    int groups = tile_cols / 4;

    for (int row = 0; row < tile_rows; row++)
    {
        for (int g = 0; g < groups; g++)
        {
            unsigned char ctl = *in++;
            unsigned char ops[4] = {
                (unsigned char)((ctl >> 6) & 3),
                (unsigned char)((ctl >> 4) & 3),
                (unsigned char)((ctl >> 2) & 3),
                (unsigned char)((ctl >> 0) & 3)
            };

            for (int i = 0; i < 4; i++)
            {
                unsigned short *tile = (unsigned short *)out;

                switch (ops[i])
                {
                case 0:
                {
                    break;
                }
                case 1:
                {
                    unsigned char flag = *in++;
                    if (flag == 0)
                    {
                        for (int y = 0; y < 4; y++)
                            for (int x = 0; x < 4; x++)
                            {
                                tile[y * width + x] = ((unsigned short *)in)[0];
                                in += 2;
                            }
                    }
                    else
                    {
                        int adj = ((int *)palette_ptr)[flag];
                        unsigned short *ref_tile = (unsigned short *)(out + dword_476DD8 + adj);
                        for (int y = 0; y < 4; y++)
                            for (int x = 0; x < 4; x++)
                                tile[y * width + x] = ref_tile[y * width + x];
                    }
                    break;
                }
                case 2:
                {
                    unsigned short color = ((unsigned short *)in)[0];
                    in += 2;
                    for (int y = 0; y < 4; y++)
                        for (int x = 0; x < 4; x++)
                            tile[y * width + x] = color;
                    break;
                }
                case 3:
                {
                    unsigned char sel = *in++;
                    unsigned short mask = vbc_pattern_masks[sel & 0x3F];
                    switch (sel >> 6)
                    {
                    case 0:
                    {
                        unsigned short c0 = ((unsigned short *)in)[0];
                        unsigned short c1 = ((unsigned short *)in)[1];
                        in += 4;
                        for (int y = 0; y < 4; y++)
                            for (int x = 0; x < 4; x++)
                                tile[y * width + x] = (mask & (1 << (y * 4 + x))) ? c1 : c0;
                        break;
                    }
                    case 1:
                    {
                        unsigned short color = ((unsigned short *)in)[0];
                        in += 2;
                        for (int y = 0; y < 4; y++)
                            for (int x = 0; x < 4; x++)
                                if (mask & (1 << (y * 4 + x)))
                                    tile[y * width + x] = color;
                        break;
                    }
                    case 2:
                    {
                        unsigned short color = ((unsigned short *)in)[0];
                        in += 2;
                        for (int y = 0; y < 4; y++)
                            for (int x = 0; x < 4; x++)
                                if (!(mask & (1 << (y * 4 + x))))
                                    tile[y * width + x] = color;
                        break;
                    }
                    case 3:
                    {
                        unsigned short color = ((unsigned short *)in)[0];
                        in += 2;
                        for (int y = 0; y < 4; y++)
                            for (int x = 0; x < 4; x++)
                                tile[y * width + x] = color;
                        break;
                    }
                    }
                    break;
                }
                }

                out += 8;
            }
        }
        out += 3 * stride;
    }

    return in;
}

//----- (0045A110) --------------------------------------------------------
__int16 video_45A110(VideoFile *a1, VideoFileFrame *frame)
{
    VideoFileFrame *v2; // edi@1
    int v3; // edx@1
    int v4; // eax@1
    char *v5; // ebx@1
    __int16 v6; // cx@5
    char v7; // al@7
    int v8; // ecx@8
    __int16 *v9; // eax@9
    __int16 v10; // si@10
    __int16 v11; // ax@16
    int v12; // edx@16
    __int16 v13; // ax@19
    __int16 v14; // cx@24

    v2 = frame;
    v3 = 0;
    LOWORD_HEXRAYS(v4) = frame->field_4;
    a1->header.num_sound_bytes = 0;
    a1->field_330 = v4;
    a1->header.field_24 = 0;
    LOWORD_HEXRAYS(v4) = frame->field_4;
    v5 = (char *)&frame->field_6;
    if (v4 & 1)
    {
        v3 = *(_WORD *)v5 + frame->field_8 * a1->header.width;
        v5 = (char *)&frame->field_A;
    }
    if (v4 & 4)
    {
        a1->header.ptr_20 = (__int16 *)(v5 + 4);
        a1->header.num_sound_bytes = *(_DWORD *)v5 - 4;
        v4 = *(_DWORD *)v5;
        v5 += *(_DWORD *)v5;
    }
    v6 = frame->field_4;
    if (v6 & 8)
    {
        if ((a1->header._0__first_4_bits_eq_bpp & 0xF) == 2)
        {
            v7 = 0;
            if (v6 & 0x10)
            {
                memcpy(word_476DE0, &v5[*(_DWORD *)v5 + 4], 0x200u);
                v8 = dword_476FE0;
                if (dword_476FE0)
                {
                    v9 = word_476DE0;
                    do
                    {
                        v10 = *v9;
                        ++v9;
                        *(v9 - 1) = *(_WORD *)(v8 + 2 * (v10 & 0x7FFF));
                    } while ((int)v9 < (int)&dword_476FE0);
                }
                v2 = frame;
                v7 = 1;
            }
            else if (dword_476FE0)
            {
                v7 = 2;
            }
            sub_45D3B8(
                a1->header.width / 4,
                a1->header.height / 4,
                v5 + 4,
                *((int *)&a1->frame_front_buffer + a1->field_332),
                *((int *)&a1->frame_front_buffer + (a1->field_332 ^ 1)),
                v3,
                a1->header.width,
                (int)a1->_344_prolly_palette,
                v7);
        }
        else
        {
            sub_45A48E(
                a1->header.width / 4,
                a1->header.height / 4,
                v5 + 4,
                *((int *)&a1->frame_front_buffer + a1->field_332),
                *((int *)&a1->frame_front_buffer + (a1->field_332 ^ 1)),
                v3,
                a1->header.width,
                (int)a1->_344_prolly_palette);
        }
        v11 = a1->field_332;
        v12 = *((_DWORD *)&a1->frame_front_buffer + v11);
        a1->field_332 = v11 ^ 1;
        a1->header.field_10 = v12;
        v4 = *(_DWORD *)v5;
        v5 += *(_DWORD *)v5;
    }
    if (v2->field_4 & 0x10)
    {
        if (a1->header._0__first_4_bits_eq_bpp == 1)
        {
            *(_WORD *)&a1->gap2C[2] = (unsigned __int8)v5[4];
            v13 = (unsigned __int8)v5[5];
            *(_WORD *)&a1->gap2C[0] = v13;
            if (!v13)
                *(_WORD *)&a1->gap2C[0] = 256;
            v4 = 3 * *(_WORD *)&a1->gap2C[0];
            memcpy((char *)&a1->header.field_10 + 2 * (*(_WORD *)&a1->gap2C[2] + 16) + *(_WORD *)&a1->gap2C[2], v5 + 6, v4);
            v2 = frame;
        }
        v5 += *(_DWORD *)v5;
    }
    if (v2->field_4 & 0x20)
    {
        v14 = *(_WORD *)v5;
        v5 += 2;
        a1->header.field_E = v14;
    }
    if (v2->field_4 & 0x40)
    {
        a1->header.field_24 = *(_DWORD *)v5 - 4;
        a1->header.field_28 = (int)(v5 + 4);
    }
    return v4;
}


//----- (00422AA0) --------------------------------------------------------
// id:
//     0  ->  MelbourneHouse & Intro
//     1  ->  Mission Briefing
int VIDEO_Play(int id)
{
    int v1; // esi@1
    char **v2; // edi@5
               //char *v3; // eax@16
    int result; // eax@17
                //char *v5; // eax@18
    DataHunk *v6; // eax@18
    DataHunk *v7; // ebp@18
    DataMapdItem *v8; // ecx@23
    stru1_draw_params *v9; // esi@25
    stru1_draw_params *v10; // edi@25
    RenderString *v11; // ebx@25
    KeyboardInput keyboard_input; // [sp+10h] [bp-1B8h]@9
    char a1[80]; // [sp+24h] [bp-1A4h]@7
    char v14[80]; // [sp+74h] [bp-154h]@7
                  //char v15[260]; // [sp+C4h] [bp-104h]@16
    v1 = id;
    VIDEO_IsAllocated();
    stru1_408480_reset_animation();
    stru1_set_animation(0, 0, 0x80000000, 0);
    RENDER_SetViewportAndClear();
    if (!v1)
    {
        draw_list_alloc();
        gRenderer->ClearTarget(64, 64, 64);
        sprintf(a1, (const char *)aSFmvMh_fmv_vbc, app_root_dir);
        if (VIDEO_ReadAndAllocDrawJob(a1, 0, 0, 100) == 1 && !VIDEO_IsVideoInvalid())
        {
            while (1)
            {
                input_update_keyboard();
                input_get_keyboard_state(&keyboard_input);
                if (keyboard_input.just_pressed_keys_mask)
                    break;
                VIDEO_DoFrame();
                draw_list_update_and_draw();
                TimedMessagePump();
                if (VIDEO_IsVideoInvalid())
                    goto LABEL_37;
            }
            VIDEO_free();
        }
    LABEL_37:
        stru1_set_animation(0, 0, 0x80000000, 0);
        sprintf(a1, (const char *)aSFmvS, app_root_dir, intro_vbc);
        if (VIDEO_ReadAndAllocDrawJob(a1, 0, 0, 100) != 1 || VIDEO_IsVideoInvalid())
            goto LABEL_43;
        while (1)
        {
            input_update_keyboard();
            input_get_keyboard_state(&keyboard_input);
            if (keyboard_input.just_pressed_keys_mask)
                goto LABEL_42;
            VIDEO_DoFrame();
            draw_list_update_and_draw();
            TimedMessagePump();
            if (VIDEO_IsVideoInvalid())
                goto LABEL_43;
        }
    }
    if (v1 != 1)
    {
        if (v1 != 2 || _47A18C_probably_play_outro_movie != 1)
            goto LABEL_44;
        _47A18C_probably_play_outro_movie = 0;
        v2 = (char **)aSurvout_vbc;
        if (current_level_idx != 14)
            v2 = (char **)aEvolvout_vbc;
        strcpy(v14, (const char *)v2);
        draw_list_alloc();
        sprintf(a1, (const char *)aSFmvS, app_root_dir, v14);
        if (VIDEO_ReadAndAllocDrawJob(a1, 0, 0, 100) != 1 || VIDEO_IsVideoInvalid())
        {
        LABEL_43:
            draw_list_free();
        LABEL_44:
            VIDEO_free();
            return 1;
        }
        while (1)
        {
            input_update_keyboard();
            input_get_keyboard_state(&keyboard_input);
            if (keyboard_input.just_pressed_keys_mask)
                break;
            VIDEO_DoFrame();
            draw_list_update_and_draw();
            TimedMessagePump();
            if (VIDEO_IsVideoInvalid())
                goto LABEL_43;
        }
    LABEL_42:
        VIDEO_free();
        goto LABEL_43;
    }
    if (current_level_idx >= LEVEL_SURV_16 && current_level_idx <= LEVEL_MUTE_25 || is_game_loading())
        goto LABEL_44;
    if (!sprites_lvl)
    {
        sprites_lvl = LVL_LoadLevel("SPRITES.LVL");
        if (!sprites_lvl)
            return 0;
    }

    v6 = LVL_LoadLevel(fmv_lvl);
    v7 = v6;
    if (!v6)
        return 0;
    result = LVL_SubstHunk(v6, sprites_lvl, (const char *)MOBD);
    if (result)
    {
        result = LVL_RunLevel(v7);
        if (result)
        {
            render_default_stru1->field_8 &= 0xBFFFFFFF;
            render_default_stru1->clip_z = render_width;
            render_default_stru1->clip_y = 0;
            render_default_stru1->clip_w = render_height;
            _47A010_mapd_item_being_drawn[0] = MAPD_Draw(MAPD_MAP, 0, 0);
            v8 = &LVL_FindMapd()->items[0];
            _40E400_set_palette(v8->GetPalette());
            render_copy_palette(&_477990_video_palette, GetSysPalette());
            sprite_47A400.pstru7 = array_466028;
            cplc_select(0);
            cplc_406320();
            sprintf(v14, (const char *)aSFmvS, app_root_dir, levels[current_level_idx].vbc_filename);
            v9 = render_create_stru1(0, 38, 31, 320, 240);
            v10 = render_create_stru1(0, 240, 313, 160, 128);
            v11 = render_string_create(0, currently_running_lvl_mobd[26].items, 400, 40, 25, 28, 90, 8, 8);
            VIDEO_ReadAndAllocDrawJob_2(v14, v9, v10, v11, 320, 240, 100);
            if (!VIDEO_IsVideoInvalid())
            {
                while (1)
                {
                    input_update_keyboard();
                    input_get_keyboard_state(&keyboard_input);
                    if (keyboard_input.just_pressed_keys_mask)
                        break;
                    _4393F0_call_mapd();
                    VIDEO_DoFrame();
                    draw_list_update_and_draw();
                    TimedMessagePump();
                    if (VIDEO_IsVideoInvalid())
                        goto LABEL_30;
                }
                VIDEO_free();
            }
        LABEL_30:
            render_string_list_remove(v11);
            render_remove_stru1(v10);
            render_remove_stru1(v9);
            bitmap_list_remove(_47A010_mapd_item_being_drawn[0]);
            LVL_Deinit();
            free(v7);
            goto LABEL_44;
        }
    }
    return result;
}