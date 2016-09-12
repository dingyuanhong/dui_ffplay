#include "AudioSampleDraw.h"
#include "SDLDraw.h"

int Get_I_Start(ImageRenderInfor *iri,AudioSampleSpikes *pSampleSpikes,
	struct AudioParams audio_tgt,int channels,int64_t audio_callback_time,
	AudioSampleArray *s,int audio_write_buf_size,bool bShowWAVE)
{
	int i_start, x, n;
    int h = 0;
    int rdft_bits, nb_freq;

	for (rdft_bits = 1; (1 << rdft_bits) < 2 * iri->height; rdft_bits++);
    nb_freq = 1 << (rdft_bits - 1);
	int data_used= bShowWAVE ? iri->width : (2*nb_freq);
    n = 2 * channels;

    int delay = audio_write_buf_size;
    delay /= n;
    /* to be more precise, we take into account the time spent since
        the last buffer computation */
    if (audio_callback_time) {
        int64_t time_diff = av_gettime() - audio_callback_time;
        delay -= (time_diff * audio_tgt.freq) / 1000000;
    }
    delay += 2 * data_used;
    if (delay < data_used)
        delay = data_used;

    i_start= x = compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
    if (bShowWAVE) {
        h = INT_MIN;
        for (int i = 0; i < 1000; i += channels) {
            int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
            int a = s->sample_array[idx];
            int b = s->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
            int c = s->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
            int d = s->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
            int score = a - d;
            if (h < score && (b ^ c) < 0) {
                h = score;
                i_start = idx;
            }
        }
    }
	pSampleSpikes->i_start = i_start;
	pSampleSpikes->h = h;
    return i_start;
}

int AudioSampleWAVE(ImageRenderInfor *iri,AudioSampleSpikes *pSampleSpikes,int channels,AudioSampleArray *s,int audio_write_buf_size)
{
	int i, i_start, x, y1, y, ys, nb_display_channels;
    int ch, h, h2, bgcolor, fgcolor;
	nb_display_channels = channels;
	i_start = pSampleSpikes->i_start;
	h = pSampleSpikes->h;

	bgcolor = SDL_MapRGB(iri->screen->format, 0x00, 0x00, 0x00);

	fill_rectangle(iri->screen,
            iri->xleft, iri->ytop, iri->width, iri->height,
            bgcolor, 0);

    fgcolor = SDL_MapRGB(iri->screen->format, 0xff, 0xff, 0xff);

    /* total height for one channel */
    h = iri->height / nb_display_channels;
    /* graph height / 2 */
    h2 = (h * 9) / 20;
    for (ch = 0; ch < nb_display_channels; ch++) {
        i = i_start + ch;
        y1 = iri->ytop + ch * h + (h / 2); /* position of center line */
        for (x = 0; x < iri->width; x++) {
            y = (s->sample_array[i] * h2) >> 15;
            if (y < 0) {
                y = -y;
                ys = y1 - y;
            } else {
                ys = y1;
            }
            fill_rectangle(iri->screen,
                            iri->xleft + x, ys, 1, y,
                            fgcolor, 0);
            i += channels;
            if (i >= SAMPLE_ARRAY_SIZE)
                i -= SAMPLE_ARRAY_SIZE;
        }
    }

    fgcolor = SDL_MapRGB(iri->screen->format, 0x00, 0x00, 0xff);

    for (ch = 1; ch < nb_display_channels; ch++) {
        y = iri->ytop + ch * h;
        fill_rectangle(iri->screen,
                        iri->xleft, y, iri->width, 1,
                        fgcolor, 0);
    }
    SDL_UpdateRect(iri->screen, iri->xleft, iri->ytop, iri->width, iri->height);

	return 0;
}

int AudioSampleFFTS(ImageRenderInfor *iri,AudioSampleSpikes *pSampleSpikes,FFTSRenderInfor *fri,int channels,AudioSampleArray *s,int audio_write_buf_size)
{
	int i, i_start, x, y1,y, ys, n, nb_display_channels;
    int ch, bgcolor, fgcolor;
    int rdft_bits, nb_freq;
	nb_display_channels = channels;
	i_start = pSampleSpikes->i_start;

	for (rdft_bits = 1; (1 << rdft_bits) < 2 * iri->height; rdft_bits++);
    nb_freq = 1 << (rdft_bits - 1);

	bgcolor = SDL_MapRGB(iri->screen->format, 0x00, 0x00, 0x00);

	nb_display_channels= FFMIN(nb_display_channels, 2);
    if (rdft_bits != fri->rdft_bits) {
        av_rdft_end(fri->rdft);
        av_free(fri->rdft_data);
        fri->rdft = av_rdft_init(rdft_bits, DFT_R2C);
        fri->rdft_bits = rdft_bits;
        fri->rdft_data = (FFTSample*)av_malloc(4 * nb_freq * sizeof(*fri->rdft_data));
    }
    {
        FFTSample *data[2];
        for (ch = 0; ch < nb_display_channels; ch++) {
            data[ch] = fri->rdft_data + 2 * nb_freq * ch;
            i = i_start + ch;
            for (x = 0; x < 2 * nb_freq; x++) {
                double w = (x-nb_freq) * (1.0 / nb_freq);
                data[ch][x] = s->sample_array[i] * (1.0 - w * w);
                i += channels;
                if (i >= SAMPLE_ARRAY_SIZE)
                    i -= SAMPLE_ARRAY_SIZE;
            }
            av_rdft_calc(fri->rdft, data[ch]);
        }
        /* Least efficient way to do this, we should of course
            * directly access it but it is more than fast enough. */
        for (y = 0; y < iri->height; y++) {
            double w = 1 / sqrt((double)nb_freq);
            int a = sqrt(w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] + data[0][2 * y + 1] * data[0][2 * y + 1]));
            int b = (nb_display_channels == 2 ) ? sqrt(w * sqrt(data[1][2 * y + 0] * data[1][2 * y + 0]
                    + data[1][2 * y + 1] * data[1][2 * y + 1])) : a;
            a = FFMIN(a, 255);
            b = FFMIN(b, 255);
            fgcolor = SDL_MapRGB(iri->screen->format, a, b, (a + b) / 2);

            fill_rectangle(iri->screen,
                        fri->xpos, iri->height-y, 1, 1,
                        fgcolor, 0);
        }
    }
    SDL_UpdateRect(iri->screen, fri->xpos, iri->ytop, 1, iri->height);
	return 0;
}

/* copy samples for viewing in editor window */
void update_sample_display(AudioSampleArray *asa, short *samples, int samples_size)
{
    int size, len;
	if(asa == NULL) return;
    size = samples_size / sizeof(short);
    while (size > 0) {
        len = SAMPLE_ARRAY_SIZE - asa->sample_array_index;
        if (len > size)
            len = size;
        memcpy(asa->sample_array + asa->sample_array_index, samples, len * sizeof(short));
        samples += len;
        asa->sample_array_index += len;
        if (asa->sample_array_index >= SAMPLE_ARRAY_SIZE)
            asa->sample_array_index = 0;
        size -= len;
    }
}